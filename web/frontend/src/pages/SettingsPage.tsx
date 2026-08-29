import { useEffect, useState } from 'react'
import './Page.css'

export default function SettingsPage() {
  const [settings, setSettings] = useState<any>(null)
  const [draft, setDraft] = useState<any>(null)
  const [saving, setSaving] = useState(false)
  const [msg, setMsg] = useState('')

  useEffect(() => {
    fetch('/api/settings')
      .then(r => r.json())
      .then(d => { setSettings(d); setDraft(d) })
  }, [])

  if (!draft) return <div className="page-loading">Loading…</div>

  const setNested = (path: string[], value: any) => {
    setDraft((prev: any) => {
      const next = structuredClone(prev)
      let node = next
      for (let i = 0; i < path.length - 1; i++) {
        if (node[path[i]] == null || typeof node[path[i]] !== 'object')
          node[path[i]] = {}
        node = node[path[i]]
      }
      node[path[path.length - 1]] = value
      return next
    })
  }

  const save = async () => {
    setSaving(true)
    const res = await fetch('/api/settings', {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(draft),
    })
    setSaving(false)
    setMsg(res.ok ? 'Saved. Restart required.' : 'Save failed.')
  }

  const t = draft.threat ?? {}
  const n = draft.nt ?? {}

  return (
    <div>
      <div className="page-header"><h1>Settings</h1></div>

      <div className="settings-card">
        <div className="settings-section-title">Inference</div>
        <div className="settings-field">
          <label>Infer config path</label>
          <input type="text" value={draft.infer_config ?? ''}
            onChange={e => setNested(['infer_config'], e.target.value)} />
        </div>
        <div className="settings-field" style={{ marginTop: 10 }}>
          <label>Cameras directory</label>
          <input type="text" value={draft.cameras_dir ?? ''}
            onChange={e => setNested(['cameras_dir'], e.target.value)} />
        </div>

        <hr />
        <div className="settings-section-title">Threat</div>
        <div className="settings-grid">
          {[
            ['min_confidence', 'Min confidence', 0, 1, 0.01],
            ['merge_radius',   'Merge radius (m)', 0, 2, 0.05],
            ['min_range',      'Min range (m)',    0, 4, 0.05],
            ['max_range',      'Max range (m)',    0, 20, 0.5],
          ].map(([key, label, min, max, step]) => (
            <div className="settings-field" key={key as string}>
              <label>{label as string}</label>
              <div className="settings-num-row">
                <input type="range" min={min} max={max} step={step}
                  value={t[key as string] ?? 0}
                  onChange={e => setNested(['threat', key as string], Number(e.target.value))} />
                <input type="number" min={min} max={max} step={step}
                  style={{ width: 80 }}
                  value={t[key as string] ?? 0}
                  onChange={e => setNested(['threat', key as string], Number(e.target.value))} />
              </div>
            </div>
          ))}
        </div>
        <div className="settings-field" style={{ marginTop: 12 }}>
          <label>Class IDs (comma-separated, empty = all classes)</label>
          <input type="text"
            value={(t.class_ids ?? []).join(', ')}
            onChange={e => setNested(
              ['threat', 'class_ids'],
              e.target.value.split(',').map(s => s.trim()).filter(Boolean).map(Number)
            )}
          />
        </div>

        <hr />
        <div className="settings-section-title">NetworkTables</div>
        <div className="settings-grid">
          <div className="settings-field">
            <label>Team</label>
            <input type="number" value={n.team ?? 6238}
              onChange={e => setNested(['nt', 'team'], Number(e.target.value))} />
          </div>
          <div className="settings-field">
            <label>Port</label>
            <input type="number" value={n.port ?? 5810}
              onChange={e => setNested(['nt', 'port'], Number(e.target.value))} />
          </div>
          <div className="settings-field">
            <label>Server (empty = 10.TE.AM.2)</label>
            <input type="text" value={n.server ?? ''}
              onChange={e => setNested(['nt', 'server'], e.target.value)} />
          </div>
          <div className="settings-field">
            <label>Identity</label>
            <input type="text" value={n.identity ?? ''}
              onChange={e => setNested(['nt', 'identity'], e.target.value)} />
          </div>
          <div className="settings-field">
            <label>Table</label>
            <input type="text" value={n.table ?? ''}
              onChange={e => setNested(['nt', 'table'], e.target.value)} />
          </div>
        </div>

        <div className="card-actions" style={{ marginTop: 20 }}>
          {msg && <span style={{ color: 'var(--muted)', fontSize: 12, flex: 1 }}>{msg}</span>}
          <button className="btn-ghost"
            onClick={() => { setDraft(structuredClone(settings)); setMsg('') }}>
            Revert
          </button>
          <button className="btn-primary" onClick={save} disabled={saving}>
            {saving ? 'Saving…' : 'Save'}
          </button>
        </div>
      </div>
    </div>
  )
}
