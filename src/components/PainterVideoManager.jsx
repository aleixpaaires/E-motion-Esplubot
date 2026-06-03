export default function PainterVideoManager({ painter, response, emotionLabel }) {
  const video = response?.video
  const question = painter.questions?.[0] || '¿Qué quieres expresar con esta pintura?'

  return (
    <div className="rounded-lg border border-zinc-800 bg-zinc-950 overflow-hidden">
      <div className="relative aspect-video bg-gradient-to-br from-zinc-900 via-zinc-800 to-stone-900 flex items-center justify-center">
        {video?.ready ? (
          <video
            src={video.src}
            poster={video.poster}
            autoPlay
            muted
            loop
            playsInline
            className="absolute inset-0 w-full h-full object-cover"
          />
        ) : (
          <div className="text-center px-6">
            <img src={painter.avatar} alt={painter.name} className="w-20 h-20 mx-auto rounded-full border border-white/10 shadow-lg" />
            <p className="mt-4 text-lg font-semibold text-white">{painter.name}</p>
            <p className="mt-1 text-sm text-zinc-400">{painter.styleLabel}</p>
            <p className="mt-4 text-xs text-zinc-500">Placeholder HeyGen: {video?.src}</p>
          </div>
        )}
        <div className="absolute left-3 top-3 rounded-md bg-black/50 px-2 py-1 text-xs text-white">
          {emotionLabel ? `Estado: ${emotionLabel}` : 'Esperando respuesta'}
        </div>
      </div>

      <div className="p-4 space-y-3">
        <div className="rounded-lg bg-zinc-900 border border-zinc-800 p-3">
          <span className="block text-[10px] uppercase tracking-wider text-zinc-500">Pregunta del pintor</span>
          <p className="text-sm text-zinc-200 mt-1">{question}</p>
        </div>
        <div className="rounded-lg bg-amber-400/10 border border-amber-400/30 p-3">
          <span className="block text-[10px] uppercase tracking-wider text-amber-200/70">Respuesta seleccionada</span>
          <p className="text-sm text-amber-50 mt-1">{response?.text || 'Esperando emoción para elegir una respuesta.'}</p>
        </div>
      </div>
    </div>
  )
}
