#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

#if __has_include("src/config.h")
#include "src/config.h"
#else
#include "src/config.example.h"
#endif

#include "src/core/brush.h"
#include "src/core/motors.h"
#include "src/core/safety.h"

// Build de prueba segura:
// - No acepta base_function.
// - No acepta stroke_id.
// - No ejecuta funciones artisticas.
// - Solo acepta type=hardware_test para mover un servo seleccionado.

constexpr int SERIAL_BAUD = 115200;
constexpr size_t MAX_COMMAND_LENGTH = 320;
constexpr unsigned long WIFI_RETRY_MS = 8000;
constexpr unsigned long MQTT_RETRY_MS = 5000;

WiFiClient wifiClient;
WiFiClientSecure secureWifiClient;
PubSubClient mqttClient;

unsigned long lastWifiAttempt = 0;
unsigned long lastMqttAttempt = 0;

void printHelp();
void printStatus();
void connectWiFi();
void connectMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void publishStatus(const char* status, const char* detail);
void publishError(const char* detail);
void handleInputCommand(const String& json, bool fromMqtt);
void handleEmotionCommand(const String& json, bool fromMqtt);
bool isForbiddenRobotCommand(const String& json);
bool parseHardwareTestCommand(const String& json, String& servoName, int& servoPin, int& angle, int& durationMs);
int pinForTestServo(const String& servoName);
String normalizeEmotionName(const String& emotion);
bool emotionPoseFor(const String& emotion, int intensity, int& wristAngle, int& shoulderAngle, int& baseAngle);
int scaledDelta(int intensity, int maxDelta);
bool runEmotionServoStep(const char* statusName, const char* servoName, int servoPin, int angle, int durationMs);
String extractStringValue(const String& json, const char* key);
int extractIntValue(const String& json, const char* key, int fallback);

void setup() {
  Serial.begin(SERIAL_BAUD);
  beginSafety();
  beginMotors();
  beginBrush();
  clearEmergencyStop();

  Serial.println("E-motion ESP32: modo prueba MQTT segura.");
  printHelp();
  printStatus();

  if (NETWORK_ENABLED) {
    if (MQTT_USE_TLS) {
      secureWifiClient.setInsecure();
      Serial.println("TLS activo en modo prueba: certificado no verificado.");
      mqttClient.setClient(secureWifiClient);
    } else {
      mqttClient.setClient(wifiClient);
    }
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    connectWiFi();
    connectMQTT();
  } else {
    Serial.println("NETWORK_ENABLED=false. Copia config.example.h a config.h y activalo para WiFi/MQTT.");
  }
}

void loop() {
  if (NETWORK_ENABLED) {
    if (WiFi.status() != WL_CONNECTED) {
      connectWiFi();
    } else if (!mqttClient.connected()) {
      connectMQTT();
    } else {
      mqttClient.loop();
    }
  }

  if (!Serial.available()) {
    delay(20);
    return;
  }

  String command = Serial.readStringUntil('\n');
  command.trim();
  if (command.length() == 0) {
    return;
  }

  if (command == "STATUS") {
    printStatus();
    return;
  }

  if (command == "STOP") {
    requestEmergencyStop();
    stopBrush();
    stopMotors();
    publishError("parada de emergencia activada");
    return;
  }

  if (command.startsWith("{")) {
    handleInputCommand(command, false);
    return;
  }

  printHelp();
}

void printHelp() {
  Serial.println("Comandos seguros:");
  Serial.println("  STATUS - muestra estado");
  Serial.println("  STOP   - parada de emergencia");
  Serial.println("  JSON/MQTT robot/test - {\"type\":\"hardware_test\",\"servo\":\"wrist\",\"angle\":105,\"duration_ms\":500}");
  Serial.println("  MQTT emotion/input - {\"type\":\"emotion_test\",\"emotion\":\"happy\",\"intensity\":80,\"duration_ms\":600}");
}

void printStatus() {
  Serial.print("WiFi: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "conectado" : "desconectado");
  Serial.print("MQTT: ");
  Serial.println(mqttClient.connected() ? "conectado" : "desconectado");
  Serial.print("NETWORK_ENABLED: ");
  Serial.println(NETWORK_ENABLED ? "true" : "false");
  Serial.print("HARDWARE_TEST_MODE: ");
  Serial.println(HARDWARE_TEST_MODE ? "true" : "false");
  Serial.print("MOTOR_OUTPUT_ENABLED: ");
  Serial.println(MOTOR_OUTPUT_ENABLED ? "true" : "false");
  Serial.print("BRUSH_OUTPUT_ENABLED: ");
  Serial.println(BRUSH_OUTPUT_ENABLED ? "true" : "false");
  Serial.print("Mapa servos: base=");
  Serial.print(BASE_SERVO_PIN);
  Serial.print(" shoulder=");
  Serial.print(SHOULDER_SERVO_PIN);
  Serial.print(" elbow=");
  Serial.print(ELBOW_SERVO_PIN);
  Serial.print(" wrist=");
  Serial.println(WRIST_SERVO_PIN);
}

void connectWiFi() {
  if (!NETWORK_ENABLED || WiFi.status() == WL_CONNECTED) {
    return;
  }
  const unsigned long now = millis();
  if (now - lastWifiAttempt < WIFI_RETRY_MS) {
    return;
  }
  lastWifiAttempt = now;

  Serial.print("Conectando WiFi a: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_RETRY_MS) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi conectado. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi no conectado. No se movera ningun servo.");
  }
}

void connectMQTT() {
  if (!NETWORK_ENABLED || WiFi.status() != WL_CONNECTED || mqttClient.connected()) {
    return;
  }
  const unsigned long now = millis();
  if (now - lastMqttAttempt < MQTT_RETRY_MS) {
    return;
  }
  lastMqttAttempt = now;

  Serial.print("Conectando MQTT a: ");
  Serial.print(MQTT_HOST);
  Serial.print(":");
  Serial.println(MQTT_PORT);

  const bool connected = mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
  if (!connected) {
    Serial.print("MQTT no conectado. Estado: ");
    Serial.println(mqttClient.state());
    return;
  }

  Serial.println("MQTT conectado.");
  if (mqttClient.subscribe(TOPIC_ROBOT_COMMAND) && mqttClient.subscribe(TOPIC_EMOTION_INPUT)) {
    Serial.print("Topic suscrito: ");
    Serial.println(TOPIC_ROBOT_COMMAND);
    Serial.print("Topic suscrito: ");
    Serial.println(TOPIC_EMOTION_INPUT);
    publishStatus("completed", "mqtt_connected");
  } else {
    publishError("no se pudo suscribir a topics mqtt");
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  const String topicName = String(topic);
  if (topicName != TOPIC_ROBOT_COMMAND && topicName != TOPIC_EMOTION_INPUT) {
    publishError("topic no permitido");
    return;
  }
  if (length == 0 || length > MAX_COMMAND_LENGTH) {
    publishError("payload vacio o demasiado largo");
    return;
  }

  String message;
  message.reserve(length + 1);
  for (unsigned int index = 0; index < length; index++) {
    message += static_cast<char>(payload[index]);
  }

  Serial.print("Comando recibido por MQTT en ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);
  if (topicName == TOPIC_EMOTION_INPUT) {
    handleEmotionCommand(message, true);
  } else {
    handleInputCommand(message, true);
  }
}

void publishStatus(const char* status, const char* detail) {
  Serial.print("{\"status\":\"");
  Serial.print(status);
  Serial.print("\",\"detail\":\"");
  Serial.print(detail);
  Serial.println("\"}");

  if (!NETWORK_ENABLED || !mqttClient.connected()) {
    return;
  }
  String payload = String("{\"status\":\"") + status + "\",\"detail\":\"" + detail + "\"}";
  mqttClient.publish(TOPIC_ROBOT_STATUS, payload.c_str());
}

void publishError(const char* detail) {
  Serial.print("{\"status\":\"error\",\"detail\":\"");
  Serial.print(detail);
  Serial.println("\"}");

  if (!NETWORK_ENABLED || !mqttClient.connected()) {
    return;
  }
  String payload = String("{\"status\":\"error\",\"detail\":\"") + detail + "\"}";
  mqttClient.publish(TOPIC_ROBOT_ERROR, payload.c_str());
}

void handleInputCommand(const String& json, bool fromMqtt) {
  if (fromMqtt && (!NETWORK_ENABLED || WiFi.status() != WL_CONNECTED || !mqttClient.connected())) {
    publishError("wifi o mqtt no conectado");
    return;
  }

  publishStatus("received", "hardware_test");

  if (isForbiddenRobotCommand(json)) {
    publishError("base_function y stroke_id no permitidos en prueba hardware");
    return;
  }

  String servoName;
  int servoPin = -1;
  int angle = TEST_SERVO_CENTER_ANGLE;
  int durationMs = 0;
  if (!parseHardwareTestCommand(json, servoName, servoPin, angle, durationMs)) {
    publishError("comando hardware_test invalido");
    return;
  }

  if (!hardwareTestConfigured(servoPin)) {
    publishError("HARDWARE_TEST_MODE false o servo no configurado");
    return;
  }

  String movingDetail = String("servo=") + servoName + " gpio=" + servoPin;
  publishStatus("moving_test_servo", movingDetail.c_str());
  if (!runSingleServoHardwareTest(servoPin, servoName.c_str(), angle, durationMs)) {
    publishError("fallo movimiento servo de prueba");
    return;
  }
  String completedDetail = String("servo=") + servoName + " detached";
  publishStatus("completed", completedDetail.c_str());
}

void handleEmotionCommand(const String& json, bool fromMqtt) {
  if (fromMqtt && (!NETWORK_ENABLED || WiFi.status() != WL_CONNECTED || !mqttClient.connected())) {
    publishError("wifi o mqtt no conectado");
    return;
  }

  publishStatus("received", "emotion_input");

  if (isForbiddenRobotCommand(json)) {
    publishError("base_function y stroke_id no permitidos en prueba emocional");
    return;
  }

  const String type = extractStringValue(json, "type");
  if (type != "emotion_test") {
    publishError("solo se acepta type=emotion_test en emotion/input");
    return;
  }

  const String emotion = normalizeEmotionName(extractStringValue(json, "emotion"));
  const int intensity = constrain(extractIntValue(json, "intensity", 0), 0, 100);
  const int durationMs = constrain(extractIntValue(json, "duration_ms", 500), 1, MAX_DURATION_MS);
  int wristAngle = TEST_SERVO_CENTER_ANGLE;
  int shoulderAngle = TEST_SERVO_CENTER_ANGLE;
  int baseAngle = TEST_SERVO_CENTER_ANGLE;
  if (!emotionPoseFor(emotion, intensity, wristAngle, shoulderAngle, baseAngle)) {
    publishError("emocion no reconocida");
    return;
  }

  String detail = String("emotion=") + emotion + " intensity=" + intensity + " sin_codo=true";
  publishStatus("moving_test_servo", detail.c_str());

  // Secuencia segura: nunca se adjuntan dos servos a la vez.
  if (!runEmotionServoStep("moving_wrist", "wrist", WRIST_SERVO_PIN, wristAngle, durationMs)) {
    publishError("fallo movimiento muneca");
    return;
  }
  if (!runEmotionServoStep("moving_shoulder", "shoulder", SHOULDER_SERVO_PIN, shoulderAngle, durationMs)) {
    publishError("fallo movimiento hombro");
    return;
  }
  if (!runEmotionServoStep("moving_base", "base", BASE_SERVO_PIN, baseAngle, durationMs)) {
    publishError("fallo movimiento base");
    return;
  }

  publishStatus("completed", detail.c_str());
}

bool isForbiddenRobotCommand(const String& json) {
  return json.indexOf("\"base_function\"") >= 0 || json.indexOf("\"stroke_id\"") >= 0;
}

bool parseHardwareTestCommand(const String& json, String& servoName, int& servoPin, int& angle, int& durationMs) {
  const String type = extractStringValue(json, "type");
  servoName = extractStringValue(json, "servo");
  if (type != "hardware_test") {
    return false;
  }
  servoName.toLowerCase();
  servoPin = pinForTestServo(servoName);
  if (servoPin < 0) {
    return false;
  }

  angle = extractIntValue(json, "angle", -1);
  durationMs = extractIntValue(json, "duration_ms", -1);
  return angle >= 0
    && durationMs > 0;
}

int pinForTestServo(const String& servoName) {
  if (servoName == "base") {
    return BASE_SERVO_PIN;
  }
  if (servoName == "shoulder") {
    return SHOULDER_SERVO_PIN;
  }
  if (servoName == "wrist") {
    return WRIST_SERVO_PIN;
  }
  return -1;
}

String normalizeEmotionName(const String& emotion) {
  String normalized = emotion;
  normalized.trim();
  normalized.toLowerCase();
  if (normalized == "alegria" || normalized == "alegre" || normalized == "joy" || normalized == "joyful") {
    return "happy";
  }
  if (normalized == "tristeza" || normalized == "triste") {
    return "sad";
  }
  if (normalized == "rabia" || normalized == "ira") {
    return "angry";
  }
  if (normalized == "calma" || normalized == "tranquilo") {
    return "calm";
  }
  if (normalized == "miedo") {
    return "fear";
  }
  if (normalized == "sorpresa" || normalized == "sorprendido") {
    return "surprise";
  }
  if (normalized == "neutro" || normalized.length() == 0) {
    return "neutral";
  }
  return normalized;
}

bool emotionPoseFor(const String& emotion, int intensity, int& wristAngle, int& shoulderAngle, int& baseAngle) {
  const int strong = scaledDelta(intensity, 20);
  const int medium = scaledDelta(intensity, 14);
  const int soft = scaledDelta(intensity, 8);
  const int tiny = scaledDelta(intensity, 4);

  if (emotion == "happy") {
    wristAngle = TEST_SERVO_CENTER_ANGLE + strong;
    shoulderAngle = TEST_SERVO_CENTER_ANGLE + medium;
    baseAngle = TEST_SERVO_CENTER_ANGLE + medium;
    return true;
  }
  if (emotion == "sad") {
    wristAngle = TEST_SERVO_CENTER_ANGLE - medium;
    shoulderAngle = TEST_SERVO_CENTER_ANGLE - soft;
    baseAngle = 90;
    return true;
  }
  if (emotion == "angry") {
    wristAngle = TEST_SERVO_CENTER_ANGLE + strong;
    shoulderAngle = TEST_SERVO_CENTER_ANGLE + strong;
    baseAngle = TEST_SERVO_CENTER_ANGLE + medium;
    return true;
  }
  if (emotion == "calm") {
    wristAngle = TEST_SERVO_CENTER_ANGLE + soft;
    shoulderAngle = TEST_SERVO_CENTER_ANGLE + soft;
    baseAngle = 90;
    return true;
  }
  if (emotion == "fear") {
    wristAngle = TEST_SERVO_CENTER_ANGLE - soft;
    shoulderAngle = TEST_SERVO_CENTER_ANGLE - soft;
    baseAngle = TEST_SERVO_CENTER_ANGLE + tiny;
    return true;
  }
  if (emotion == "surprise") {
    wristAngle = TEST_SERVO_CENTER_ANGLE + strong;
    shoulderAngle = TEST_SERVO_CENTER_ANGLE + medium;
    baseAngle = TEST_SERVO_CENTER_ANGLE + strong;
    return true;
  }
  wristAngle = TEST_SERVO_CENTER_ANGLE + tiny;
  shoulderAngle = TEST_SERVO_CENTER_ANGLE + tiny;
  baseAngle = 90;
  return emotion == "neutral";
}

int scaledDelta(int intensity, int maxDelta) {
  return constrain(map(constrain(intensity, 0, 100), 0, 100, 0, maxDelta), 0, maxDelta);
}

bool runEmotionServoStep(const char* statusName, const char* servoName, int servoPin, int angle, int durationMs) {
  String detail = String("servo=") + servoName + " gpio=" + servoPin;
  publishStatus(statusName, detail.c_str());
  return runSingleServoHardwareTest(servoPin, servoName, angle, durationMs);
}

String extractStringValue(const String& json, const char* key) {
  String pattern = String("\"") + key + "\":";
  int keyIndex = json.indexOf(pattern);
  if (keyIndex < 0) {
    return "";
  }
  int firstQuote = json.indexOf('"', keyIndex + pattern.length());
  if (firstQuote < 0) {
    return "";
  }
  int secondQuote = json.indexOf('"', firstQuote + 1);
  if (secondQuote < 0) {
    return "";
  }
  return json.substring(firstQuote + 1, secondQuote);
}

int extractIntValue(const String& json, const char* key, int fallback) {
  String pattern = String("\"") + key + "\":";
  int keyIndex = json.indexOf(pattern);
  if (keyIndex < 0) {
    return fallback;
  }
  int valueStart = keyIndex + pattern.length();
  while (valueStart < json.length() && json.charAt(valueStart) == ' ') {
    valueStart++;
  }
  int valueEnd = valueStart;
  while (valueEnd < json.length() && isDigit(json.charAt(valueEnd))) {
    valueEnd++;
  }
  if (valueEnd == valueStart) {
    return fallback;
  }
  return json.substring(valueStart, valueEnd).toInt();
}
