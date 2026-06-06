#pragma once

#include <Arduino.h>

// Mapa real de servos E-motion:
// Base: GPIO 27
// Hombro: GPIO 25
// Codo: GPIO 26
// Muneca: GPIO 14
//
// Prueba fisica minima. Cada comando activa solo un servo por GPIO.
constexpr bool HARDWARE_TEST_MODE = true;
constexpr int MIN_SAFE_ANGLE = 80;
constexpr int MAX_SAFE_ANGLE = 110;
constexpr int MAX_DURATION_MS = 1000;
constexpr int TEST_SERVO_CENTER_ANGLE = 90;
constexpr int TEST_SERVO_MIN_ANGLE = MIN_SAFE_ANGLE;
constexpr int TEST_SERVO_MAX_ANGLE = MAX_SAFE_ANGLE;
constexpr int TEST_SERVO_MAX_DURATION_MS = MAX_DURATION_MS;

// Solo puede activarse si HARDWARE_TEST_MODE esta activo.
constexpr bool MOTOR_OUTPUT_ENABLED = HARDWARE_TEST_MODE;
constexpr int BASE_SERVO_PIN = 27;
constexpr int SHOULDER_SERVO_PIN = 25;
constexpr int ELBOW_SERVO_PIN = 26;
constexpr int WRIST_SERVO_PIN = 14;

// Limites iniciales conservadores. Deben calibrarse para el brazo real.
constexpr int SHOULDER_MIN_ANGLE = 25;
constexpr int SHOULDER_MAX_ANGLE = 155;
constexpr int ELBOW_MIN_ANGLE = 30;
constexpr int ELBOW_MAX_ANGLE = 150;
constexpr int WRIST_MIN_ANGLE = 30;
constexpr int WRIST_MAX_ANGLE = 150;

struct ServoPose {
  int shoulder;
  int elbow;
  int wrist;
};

void beginMotors();
bool motorsConfigured();
bool hardwareTestConfigured(int servoPin);
bool runSingleServoHardwareTest(int servoPin, const char* servoName, int angle, int durationMs);
bool moveToPoseSafe(const ServoPose& target, int speed);
bool runPoseSequence(const ServoPose poses[], size_t poseCount, int speed, int durationMs);
void returnToNeutral();
void stopMotors();
ServoPose currentPose();
