import 'dotenv/config'
import { DEFAULT_DEVICE_ID, createTopicMap, normalizeDeviceId } from '../../src/lib/mqttContract.js'

export function loadBridgeConfig(env = process.env) {
  const deviceId = normalizeDeviceId(env.MQTT_DEVICE_ID || env.MOODCAM_DEVICE_ID || DEFAULT_DEVICE_ID)
  return {
    deviceId,
    mqttUrl: env.MQTT_URL || env.MQTT_BROKER_URL || 'wss://broker.hivemq.com:8884/mqtt',
    mqttUsername: env.MQTT_USERNAME || '',
    mqttPassword: env.MQTT_PASSWORD || '',
    openaiApiKey: env.OPENAI_API_KEY || '',
    openaiModel: env.OPENAI_DECISION_MODEL || 'gpt-4.1-mini',
    topics: createTopicMap(deviceId),
  }
}

export function buildMqttOptions(config) {
  const options = {
    clean: true,
    reconnectPeriod: 5000,
    connectTimeout: 10000,
    clientId: `moodcam-ai-bridge-${config.deviceId}-${Math.random().toString(16).slice(2)}`,
    will: {
      topic: config.topics.systemError,
      payload: JSON.stringify({
        type: 'bridge_offline',
        device_id: config.deviceId,
        message: 'AI Bridge desconectado inesperadamente.',
        timestamp: Date.now(),
      }),
      qos: 1,
      retain: false,
    },
  }

  if (config.mqttUsername) options.username = config.mqttUsername
  if (config.mqttPassword) options.password = config.mqttPassword
  return options
}
