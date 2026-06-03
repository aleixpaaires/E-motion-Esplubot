import { ARTISTS } from './artEngine.js'

const PLACEHOLDER_VIDEO_BASE = '/painters/placeholders'

const PAINTER_SCRIPT_DATA = {
  kandinsky: {
    avatar: '/logo-esplubot.png',
    questions: [
      'Si tu emoción fuera un color o una forma, ¿cuál sería ahora?',
      '¿Qué ritmo te gustaría que tuviera este dibujo?',
    ],
  },
  pollock: {
    avatar: '/logo-esplubot.png',
    questions: [
      '¿Qué energía quieres soltar en la pintura?',
      '¿Tu emoción se mueve rápido, lento o con saltos?',
    ],
  },
  rothko: {
    avatar: '/logo-esplubot.png',
    questions: [
      '¿Qué color profundo describe mejor cómo estás?',
      '¿Tu emoción ocupa mucho espacio o está quieta en un rincón?',
    ],
  },
  'alma-thomas': {
    avatar: '/logo-esplubot.png',
    questions: [
      '¿Qué color alegre o tranquilo quieres repetir en el cuadro?',
      '¿Tu emoción parece luz, mosaico o naturaleza?',
    ],
  },
  'de-kooning': {
    avatar: '/logo-esplubot.png',
    questions: [
      '¿Tu emoción sale como una curva suave o como un gesto intenso?',
      '¿Qué quieres transformar con este dibujo?',
    ],
  },
}

const RESPONSE_TEXT = {
  joyful: 'Voy a responder con movimiento abierto, color luminoso y trazos vivos.',
  calm: 'Voy a bajar la velocidad y dejar respirar el color con trazos suaves.',
  sad: 'Voy a usar capas más profundas y recorridos lentos para cuidar esa emoción.',
  nervous: 'Voy a convertir esa tensión en líneas controladas y cambios de dirección.',
  tired: 'Voy a pintar con menos presión, pausas y una paleta descansada.',
  confused: 'Voy a organizar esa mezcla con formas que encuentren un camino.',
  neutral: 'Voy a crear una composición equilibrada para empezar desde calma.',
}

export function getPainterProfiles() {
  return ARTISTS.map((artist) => buildPainterProfile(artist))
}

export function getPainterProfile(painterId) {
  return getPainterProfiles().find((painter) => painter.id === painterId) || getPainterProfiles()[0]
}

export function getPainterVideo(painterId, emotion = 'neutral') {
  const painter = getPainterProfile(painterId)
  return painter.videos.find((video) => video.emotion === emotion) || painter.videos.find((video) => video.emotion === 'neutral')
}

function buildPainterProfile(artist) {
  const data = PAINTER_SCRIPT_DATA[artist.id] || PAINTER_SCRIPT_DATA.kandinsky

  return {
    id: artist.id,
    name: artist.name,
    description: artist.summary,
    style: artist.style,
    styleLabel: artist.label,
    avatar: data.avatar,
    questions: data.questions,
    videos: buildPlaceholderVideos(artist.id),
    responses: buildResponses(artist.id),
  }
}

function buildPlaceholderVideos(painterId) {
  return ['neutral', 'joyful', 'calm', 'sad', 'nervous', 'tired', 'confused'].map((emotion) => ({
    id: `${painterId}-${emotion}-placeholder`,
    emotion,
    label: emotion,
    src: `${PLACEHOLDER_VIDEO_BASE}/${painterId}/${emotion}.mp4`,
    poster: '/logo-esplubot.png',
    provider: 'heygen-placeholder',
    ready: false,
  }))
}

function buildResponses(painterId) {
  return Object.entries(RESPONSE_TEXT).map(([emotion, text]) => ({
    id: `${painterId}-response-${emotion}`,
    emotion,
    text,
    videoId: `${painterId}-${emotion}-placeholder`,
  }))
}
