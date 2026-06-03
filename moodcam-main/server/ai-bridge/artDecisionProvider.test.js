import test from 'node:test'
import assert from 'node:assert/strict'
import { decideArtPlan } from './providers/artDecisionProvider.js'
import { DEFAULT_ROBOT_CALIBRATION } from '../../src/lib/voiceEngine.js'

test('usa fallback local cuando OpenAI no esta configurado', async () => {
  const result = await decideArtPlan({
    config: {
      openaiApiKey: '',
      openaiModel: 'gpt-4.1-mini',
    },
    sessionSummary: {
      session_id: 's1',
      device_id: 'device1',
      artist_id: 'kandinsky',
      combined_emotions: [
        { emotion: 'happy', label: 'Alegría', percentage: 70 },
        { emotion: 'surprise', label: 'Sorpresa', percentage: 30 },
      ],
      calibration: DEFAULT_ROBOT_CALIBRATION,
      mobility: 80,
      conversation_mode: 'none',
    },
  })

  assert.equal(result.source, 'local_fallback')
  assert.equal(result.plan.session_id, 's1')
  assert.equal(result.plan.validated_by, 'artEngine')
  assert.ok(result.plan.robot_commands.some((command) => command.type === 'stroke'))
})
