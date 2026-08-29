import { useState, useEffect, useRef } from 'react'
import CameraMount from './CameraMount'
import './CameraCard.css'

interface Props {
  camera: any
  onSaved: () => void
}

const LIVE_CONTROLS = [
  { key: 'exposure_absolute',    label: 'Exposure',   min: 1,   max: 5000 },
  { key: 'gain',                 label: 'Gain',       min: 0,   max: 255  },
  { key: 'brightness',           label: 'Brightness', min: -64, max: 64   },
  { key: 'contrast',             label: 'Contrast',   min: 0,   max: 95   },
]

async function playWhep(video: HTMLVideoElement, url: string): Promise<RTCPeerConnection> {
  const pc = new RTCPeerConnection({ iceServers: [] })
  pc.addTransceiver('video', { direction: 'recvonly' })
  pc.addTransceiver('audio', { direction: 'recvonly' })
  pc.ontrack = (ev) => {
    if (ev.streams[0]) video.srcObject = ev.streams[0]
  }

  const offer = await pc.createOffer()
  await pc.setLocalDescription(offer)
  await new Promise<void>((resolve) => {
    if (pc.iceGatheringState === 'complete') { resolve(); return }
    const t = window.setTimeout(() => resolve(), 2000)
    pc.addEventListener('icegatheringstatechange', () => {
      if (pc.iceGatheringState === 'complete') {
        window.clearTimeout(t)
        resolve()
      }
    })
  })

  const res = await fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/sdp' },
    body: pc.localDescription?.sdp ?? offer.sdp,
  })
  if (!res.ok) {
    pc.close()
    throw new Error(`WHEP ${res.status}`)
  }
  await pc.setRemoteDescription({ type: 'answer', sdp: await res.text() })
  void video.play().catch(() => {})
  return pc
}

function VideoPreview() {
  const videoRef = useRef<HTMLVideoElement>(null)
  const [err, setErr] = useState('')
  const host = window.location.hostname
  const whepUrl = `http://${host}:8889/live/ds-test/whep`
  const rtspUrl = `rtsp://${host}:8554/live/ds-test`

  useEffect(() => {
    const el = videoRef.current
    if (!el) return
    let pc: RTCPeerConnection | null = null
    let cancelled = false
    setErr('')
    playWhep(el, whepUrl)
      .then(conn => { if (cancelled) conn.close(); else pc = conn })
      .catch(e => { if (!cancelled) setErr(String(e)) })
    return () => {
      cancelled = true
      pc?.close()
      el.srcObject = null
    }
  }, [whepUrl])

  return (
    <div className="video-preview">
      <video
        ref={videoRef}
        autoPlay muted playsInline
        style={{ width: '100%', borderRadius: 6, background: '#000', maxHeight: 200 }}
      />
      {err && <div className="video-error">Preview failed ({err}). Use {rtspUrl}</div>}
      <div className="video-label">{rtspUrl}</div>
    </div>
  )
}

export default function CameraCard({ camera, onSaved }: Props) {
  const [open, setOpen] = useState(false)
  const [draft, setDraft] = useState<any>(() => structuredClone(camera))
  const [liveCtrl, setLiveCtrl] = useState<Record<string, number>>({})
  const [saving, setSaving] = useState(false)
  const [msg, setMsg] = useState('')
  const name = camera._name

  useEffect(() => { setDraft(structuredClone(camera)) }, [camera])

  const set = (path: string[], value: any) => {
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
    try {
      const res = await fetch(`/api/cameras/${name}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(draft),
      })
      if (res.ok) { setMsg('Saved. Restart required for pipeline changes.'); onSaved() }
      else setMsg('Save failed.')
    } finally { setSaving(false) }
  }

  const applyV4l2 = async (ctrl: string, value: number) => {
    await fetch(`/api/cameras/${name}/v4l2`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ [ctrl]: value }),
    })
  }

  const extr = draft.extrinsics ?? {}

  return (
    <div className="camera-card">
      <div className="camera-card-header" onClick={() => setOpen(o => !o)}>
        <div className="camera-card-title">
          <span className="chevron">{open ? '▼' : '▶'}</span>
          <span className="cam-name">{name}</span>
          {draft.device && <span className="cam-device">{draft.device}</span>}
        </div>
        <span className="badge badge-green">
          <span className="dot" />
          Detection
        </span>
      </div>

      {open && (
        <div className="camera-card-body">
          <div className="preview-row">
            <VideoPreview />
            <CameraMount
              extrinsics={extr}
              width={draft.width}
              height={draft.height}
              fx={draft.intrinsics?.fx}
              fy={draft.intrinsics?.fy}
            />
          </div>

          <div className="section-label">Live V4L2 Controls</div>
          <div className="live-controls">
            {LIVE_CONTROLS.map(ctrl => (
              <div key={ctrl.key} className="ctrl-row">
                <label>{ctrl.label}</label>
                <div className="ctrl-slider-row">
                  <input
                    type="range"
                    min={ctrl.min} max={ctrl.max}
                    value={liveCtrl[ctrl.key] ?? Math.floor((ctrl.min + ctrl.max) / 2)}
                    onChange={e => {
                      const v = Number(e.target.value)
                      setLiveCtrl(p => ({ ...p, [ctrl.key]: v }))
                    }}
                    onMouseUp={e => applyV4l2(ctrl.key, Number((e.target as HTMLInputElement).value))}
                  />
                  <span className="ctrl-value">
                    {liveCtrl[ctrl.key] ?? '—'}
                  </span>
                </div>
              </div>
            ))}
          </div>
          <hr />

          <div className="section-label">
            Pipeline Config <span className="restart-badge">⚠ requires restart</span>
          </div>
          <div className="form-grid">
            <div className="form-field">
              <label>Device</label>
              <input type="text" value={draft.device ?? ''} onChange={e => set(['device'], e.target.value)} />
            </div>
            <div className="form-field-row">
              <div className="form-field">
                <label>Width</label>
                <input type="number" value={draft.width ?? ''} onChange={e => set(['width'], Number(e.target.value))} />
              </div>
              <div className="form-field">
                <label>Height</label>
                <input type="number" value={draft.height ?? ''} onChange={e => set(['height'], Number(e.target.value))} />
              </div>
              <div className="form-field">
                <label>FPS</label>
                <input type="number" value={draft.fps ?? ''} onChange={e => set(['fps'], Number(e.target.value))} />
              </div>
            </div>
            <div className="form-field-row">
              <div className="form-field">
                <label>Rotation</label>
                <select value={draft.rotation ?? 0} onChange={e => set(['rotation'], Number(e.target.value))}>
                  <option value={0}>0°</option>
                  <option value={90}>90° CW</option>
                  <option value={180}>180°</option>
                  <option value={270}>270° CW</option>
                </select>
              </div>
              <div className="form-field">
                <label>Type</label>
                <select value={draft.type ?? 'usb'} onChange={e => set(['type'], e.target.value)}>
                  <option value="usb">USB</option>
                  <option value="csi">CSI</option>
                  <option value="test">Test pattern</option>
                </select>
              </div>
              <label className="checkbox-label" style={{ alignSelf: 'end', paddingBottom: 6 }}>
                <input type="checkbox" checked={!!draft.hw_decode} onChange={e => set(['hw_decode'], e.target.checked)} />
                HW Decode
              </label>
            </div>
          </div>

          {draft.intrinsics && (
            <>
              <div className="section-label" style={{ marginTop: 12 }}>Intrinsics (calibration)</div>
              <div className="form-grid intrinsics">
                {['fx','fy','cx','cy'].map(k => (
                  <div className="form-field" key={k}>
                    <label>{k}</label>
                    <input type="number" step="0.001"
                      value={draft.intrinsics[k] ?? ''}
                      onChange={e => set(['intrinsics', k], Number(e.target.value))} />
                  </div>
                ))}
                {draft.intrinsics.distortion && ['k1','k2','p1','p2','k3'].map(k => (
                  <div className="form-field" key={k}>
                    <label>{k}</label>
                    <input type="number" step="0.000001"
                      value={draft.intrinsics.distortion[k] ?? ''}
                      onChange={e => set(['intrinsics', 'distortion', k], Number(e.target.value))} />
                  </div>
                ))}
              </div>
            </>
          )}

          {draft.extrinsics && (
            <>
              <div className="section-label" style={{ marginTop: 12 }}>Extrinsics (robot frame, m / rad)</div>
              <div className="form-grid intrinsics">
                {['tx','ty','tz','yaw','pitch','roll'].map(k => (
                  <div className="form-field" key={k}>
                    <label>{k}</label>
                    <input type="number" step="0.0001"
                      value={extr[k] ?? ''}
                      onChange={e => set(['extrinsics', k], Number(e.target.value))} />
                  </div>
                ))}
              </div>
            </>
          )}

          <div className="card-actions">
            {msg && <span className="save-msg">{msg}</span>}
            <button className="btn-ghost" onClick={() => { setDraft(structuredClone(camera)); setMsg('') }}>
              Revert
            </button>
            <button className="btn-primary" onClick={save} disabled={saving}>
              {saving ? 'Saving…' : 'Save'}
            </button>
          </div>
        </div>
      )}
    </div>
  )
}
