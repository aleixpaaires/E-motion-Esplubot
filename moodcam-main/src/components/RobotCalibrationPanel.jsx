export default function RobotCalibrationPanel({ calibration, onChange }) {
  const update = (path, value) => {
    const keys = path.split('.')
    const next = structuredClone(calibration)
    let cursor = next
    for (let i = 0; i < keys.length - 1; i += 1) {
      cursor = cursor[keys[i]]
    }
    cursor[keys[keys.length - 1]] = value
    onChange(next)
  }

  const updatePaint = (index, key, value) => {
    const next = structuredClone(calibration)
    next.paints[index][key] = value
    onChange(next)
  }

  return (
    <div className="space-y-4">
      <div>
        <h2 className="text-sm font-semibold text-gray-300 uppercase tracking-wider">4. Calibrar A4</h2>
        <p className="text-xs text-gray-500 mt-1">Lienzo horizontal, pinturas, agua y reposo en milímetros del robot.</p>
      </div>

      <div className="grid grid-cols-2 gap-2">
        <NumberInput label="Origen X" value={calibration.canvas.originX} onChange={(v) => update('canvas.originX', v)} />
        <NumberInput label="Origen Y" value={calibration.canvas.originY} onChange={(v) => update('canvas.originY', v)} />
        <NumberInput label="Ancho" value={calibration.canvas.width} onChange={(v) => update('canvas.width', v)} />
        <NumberInput label="Alto" value={calibration.canvas.height} onChange={(v) => update('canvas.height', v)} />
        <NumberInput label="Margen" value={calibration.canvas.margin} onChange={(v) => update('canvas.margin', v)} />
        <NumberInput label="Z arriba" value={calibration.z.up} onChange={(v) => update('z.up', v)} />
        <NumberInput label="Z pintar" value={calibration.z.paint} onChange={(v) => update('z.paint', v)} />
        <NumberInput label="Z mojar" value={calibration.z.dip} onChange={(v) => update('z.dip', v)} />
      </div>

      <div className="grid grid-cols-2 gap-2">
        <NumberInput label="Agua X" value={calibration.water.x} onChange={(v) => update('water.x', v)} />
        <NumberInput label="Agua Y" value={calibration.water.y} onChange={(v) => update('water.y', v)} />
        <NumberInput label="Toalla X" value={calibration.towel.x} onChange={(v) => update('towel.x', v)} />
        <NumberInput label="Toalla Y" value={calibration.towel.y} onChange={(v) => update('towel.y', v)} />
        <NumberInput label="Reposo X" value={calibration.rest.x} onChange={(v) => update('rest.x', v)} />
        <NumberInput label="Reposo Y" value={calibration.rest.y} onChange={(v) => update('rest.y', v)} />
      </div>

      <div className="space-y-2">
        <span className="block text-[10px] uppercase tracking-wider text-gray-500">Pinturas</span>
        {calibration.paints.map((paint, index) => (
          <div key={paint.id} className="grid grid-cols-[1fr_64px_64px] gap-2 items-center">
            <div className="flex items-center gap-2 min-w-0">
              <span className="w-3 h-3 rounded-full border border-white/20 shrink-0" style={{ backgroundColor: paint.hex }} />
              <span className="text-xs text-gray-300 truncate">{paint.label}</span>
            </div>
            <SmallNumber value={paint.x} onChange={(v) => updatePaint(index, 'x', v)} />
            <SmallNumber value={paint.y} onChange={(v) => updatePaint(index, 'y', v)} />
          </div>
        ))}
      </div>
    </div>
  )
}

function NumberInput({ label, value, onChange }) {
  return (
    <label className="space-y-1">
      <span className="block text-[10px] uppercase tracking-wider text-gray-500">{label}</span>
      <input
        type="number"
        value={value}
        onChange={(event) => onChange(Number(event.target.value))}
        className="w-full rounded-md border border-gray-800 bg-gray-950 px-2 py-1.5 text-sm text-gray-200 focus:outline-none focus:border-amber-400"
      />
    </label>
  )
}

function SmallNumber({ value, onChange }) {
  return (
    <input
      type="number"
      value={value}
      onChange={(event) => onChange(Number(event.target.value))}
      className="w-full rounded-md border border-gray-800 bg-gray-950 px-2 py-1 text-xs text-gray-200 focus:outline-none focus:border-amber-400"
    />
  )
}
