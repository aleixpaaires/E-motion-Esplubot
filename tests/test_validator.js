import test from 'node:test'
import assert from 'node:assert/strict'
import { loadStrokes } from '../server/load_strokes.js'
import { normalizeAndValidateForEsp32, sendToEsp32 } from '../server/send_to_esp32.js'
import { validateDecision } from '../server/validator.js'

const validDecision = {
  stroke_id: 'pollock-angry-simulated-splash-v1',
  artist: 'pollock',
  emotion: 'angry',
  base_function: 'pollockSimulatedSplash',
  speed: 54,
  intensity: 80,
  duration_ms: 1200,
  pressure: 42,
  selected_colors: ['rojo', 'negro'],
}

test('carga 35 movimientos aprobados y funciones Arduino existentes', () => {
  const catalog = loadStrokes({ forceReload: true })
  assert.equal(catalog.strokes.length, 35)
  assert.equal(catalog.byId.size, 35)
  assert.ok(catalog.approvedFunctions.has('pollockSimulatedSplash'))
})

test('acepta una decisión aprobada y devuelve solo un comando seguro', () => {
  const result = validateDecision(validDecision)

  assert.deepEqual(result, {
    valid: true,
    command: {
      base_function: 'pollockSimulatedSplash',
      speed: 54,
      intensity: 80,
      duration_ms: 1200,
      pressure: 42,
      selected_colors: ['rojo', 'negro'],
    },
  })
})

test('rechaza un stroke_id inventado sin producir comando', () => {
  const result = validateDecision({ ...validDecision, stroke_id: 'POLLOCK_RABIA_SPLASH_01' })
  assert.equal(result.valid, false)
  assert.equal('command' in result, false)
  assert.match(result.error, /stroke_id no aprobado/)
})

test('rechaza artista, emoción o función que no coincidan con la receta', () => {
  assert.match(validateDecision({ ...validDecision, artist: 'rothko' }).error, /artist no coincide/)
  assert.match(validateDecision({ ...validDecision, emotion: 'happy' }).error, /emotion no coincide/)
  assert.match(validateDecision({ ...validDecision, base_function: 'inventedAction' }).error, /base_function no coincide/)
})

test('rechaza valores globales fuera de 0 a 100', () => {
  assert.match(validateDecision({ ...validDecision, speed: 101 }).error, /speed debe estar entre 0 y 100/)
  assert.match(validateDecision({ ...validDecision, intensity: -1 }).error, /intensity debe estar entre 0 y 100/)
  assert.match(validateDecision({ ...validDecision, pressure: 101 }).error, /pressure debe estar entre 0 y 100/)
})

test('rechaza parámetros que superan los límites seguros específicos del trazo', () => {
  assert.match(validateDecision({ ...validDecision, speed: 80 }).error, /speed fuera de los límites seguros/)
  assert.match(validateDecision({ ...validDecision, intensity: 40 }).error, /intensity fuera de los límites seguros/)
  assert.match(validateDecision({ ...validDecision, duration_ms: 6000 }).error, /duration_ms fuera de los límites seguros/)
  assert.match(validateDecision({ ...validDecision, pressure: 60 }).error, /pressure fuera de los límites seguros/)
})

test('rechaza selected_colors vacío, desconocido, duplicado o no aprobado', () => {
  assert.match(validateDecision({ ...validDecision, selected_colors: [] }).error, /no puede estar vacío/)
  assert.match(validateDecision({ ...validDecision, selected_colors: ['verde'] }).error, /color desconocido/)
  assert.match(validateDecision({ ...validDecision, selected_colors: ['rojo', 'red'] }).error, /duplicados/)
  assert.match(validateDecision({ ...validDecision, selected_colors: ['azul'] }).error, /no aprobados/)
})

test('rechaza acciones y campos inesperados peligrosos', () => {
  const result = validateDecision({
    ...validDecision,
    action: 'moveServoDirectly',
    servo_angle: 180,
  })

  assert.equal(result.valid, false)
  assert.equal('command' in result, false)
  assert.match(result.error, /Campos inesperados o peligrosos/)
})

test('rechaza campos obligatorios ausentes y valores no enteros', () => {
  const { duration_ms, ...missingDuration } = validDecision
  assert.match(validateDecision(missingDuration).error, /Faltan campos obligatorios: duration_ms/)
  assert.match(validateDecision({ ...validDecision, speed: 45.5 }).error, /speed debe ser un número entero/)
})

test('normaliza un comando validado para ESP32 sin exigir stroke_id', () => {
  const result = normalizeAndValidateForEsp32({
    base_function: 'pollockSplash',
    speed: 80,
    intensity: 75,
    duration_ms: 6000,
    pressure: 40,
    selected_colors: ['rojo', 'negro'],
  })

  assert.equal(result.valid, true)
  assert.equal(result.command.base_function, 'pollockSplash')
})

test('sendToEsp32 simula por defecto y no envia comandos invalidos', async () => {
  const simulated = await sendToEsp32({
    base_function: 'pollockSplash',
    speed: 80,
    intensity: 75,
    duration_ms: 6000,
    pressure: 40,
    selected_colors: ['rojo', 'negro'],
  })
  assert.equal(simulated.valid, true)
  assert.equal(simulated.mode, 'simulation')
  assert.equal(simulated.status, 'completed')

  const rejected = await sendToEsp32({
    base_function: 'inventedAction',
    speed: 80,
    intensity: 75,
    duration_ms: 6000,
    pressure: 40,
    selected_colors: ['rojo', 'negro'],
  })
  assert.equal(rejected.valid, false)
  assert.equal('sent' in rejected, false)
})
