import { useEffect, useRef } from 'react'
import * as THREE from 'three'
import { OrbitControls } from 'three/addons/controls/OrbitControls.js'
import './CameraMount.css'

interface Props {
  extrinsics: {
    tx?: number
    ty?: number
    tz?: number
    yaw?: number
    pitch?: number
    roll?: number
  }
  width?: number
  height?: number
  fx?: number
  fy?: number
}

// WPILib robot frame (X forward, Y left, Z up) → three.js (Y up):
// three.x = robot.y, three.y = robot.z, three.z = robot.x
function to3(x: number, y: number, z: number) {
  return new THREE.Vector3(y, z, x)
}

export default function CameraMount({ extrinsics, width = 640, height = 480, fx, fy }: Props) {
  const hostRef = useRef<HTMLDivElement>(null)

  const tx = Number(extrinsics.tx) || 0
  const ty = Number(extrinsics.ty) || 0
  const tz = Number(extrinsics.tz) || 0
  const yaw = Number(extrinsics.yaw) || 0
  const pitch = Number(extrinsics.pitch) || 0
  const roll = Number(extrinsics.roll) || 0

  useEffect(() => {
    const host = hostRef.current
    if (!host) return

    const scene = new THREE.Scene()
    scene.background = new THREE.Color(0x0a0c12)

    const renderer = new THREE.WebGLRenderer({ antialias: true })
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2))
    host.appendChild(renderer.domElement)

    const camera = new THREE.PerspectiveCamera(50, 1, 0.05, 20)
    camera.position.copy(to3(-1.6, -1.8, 1.1))
    camera.up.set(0, 1, 0)

    const controls = new OrbitControls(camera, renderer.domElement)
    controls.target.copy(to3(0, 0, 0.15))
    controls.enableDamping = true
    controls.maxDistance = 6
    controls.minDistance = 0.4

    scene.add(new THREE.AmbientLight(0xffffff, 0.55))
    const key = new THREE.DirectionalLight(0xffffff, 0.7)
    key.position.copy(to3(-1, 1, 2))
    scene.add(key)

    const grid = new THREE.GridHelper(3, 12, 0x2a2d3a, 0x1a1d27)
    scene.add(grid)

    // Robot chassis ~30x30" bumper, 12 cm tall, sitting on z=0.
    const bumper = new THREE.Mesh(
      new THREE.BoxGeometry(0.76, 0.12, 0.76),
      new THREE.MeshStandardMaterial({ color: 0x1e3a5f, metalness: 0.2, roughness: 0.7 })
    )
    bumper.position.copy(to3(0, 0, 0.06))
    scene.add(bumper)

    const top = new THREE.Mesh(
      new THREE.BoxGeometry(0.55, 0.08, 0.55),
      new THREE.MeshStandardMaterial({ color: 0x2a4a73, metalness: 0.15, roughness: 0.8 })
    )
    top.position.copy(to3(0, 0, 0.16))
    scene.add(top)

    // Frame axes at robot origin.
    const axis = (dir: THREE.Vector3, color: number) => {
      const g = new THREE.BufferGeometry().setFromPoints([new THREE.Vector3(0, 0, 0), dir])
      return new THREE.Line(g, new THREE.LineBasicMaterial({ color }))
    }
    scene.add(axis(to3(0.45, 0, 0), 0xef4444)) // +X forward
    scene.add(axis(to3(0, 0.45, 0), 0x22c55e)) // +Y left
    scene.add(axis(to3(0, 0, 0.45), 0x3b82f6)) // +Z up

    const camGroup = new THREE.Group()
    scene.add(camGroup)

    const body = new THREE.Mesh(
      new THREE.BoxGeometry(0.045, 0.04, 0.06),
      new THREE.MeshStandardMaterial({ color: 0xe2e8f0, metalness: 0.4, roughness: 0.4 })
    )
    camGroup.add(body)

    const lens = new THREE.Mesh(
      new THREE.CylinderGeometry(0.012, 0.016, 0.02, 16),
      new THREE.MeshStandardMaterial({ color: 0x111827 })
    )
    lens.rotation.x = Math.PI / 2
    lens.position.z = -0.04
    camGroup.add(lens)

    let helper: THREE.CameraHelper | null = null
    const vizCam = new THREE.PerspectiveCamera(60, 4 / 3, 0.04, 1.4)
    camGroup.add(vizCam)
    helper = new THREE.CameraHelper(vizCam)
    scene.add(helper)

    const layout = () => {
      const w = host.clientWidth || 320
      const h = Math.max(220, Math.round(w * 0.62))
      renderer.setSize(w, h)
      camera.aspect = w / h
      camera.updateProjectionMatrix()
    }
    layout()
    const ro = new ResizeObserver(layout)
    ro.observe(host)

    let raf = 0
    const tick = () => {
      controls.update()
      helper?.update()
      renderer.render(scene, camera)
      raf = requestAnimationFrame(tick)
    }
    tick()

    const applyMount = () => {
      camGroup.position.copy(to3(tx, ty, tz))

      // Optical axis in robot frame (yaw CCW from +X, pitch down).
      const cp = Math.cos(pitch)
      const look = to3(
        Math.cos(yaw) * cp,
        Math.sin(yaw) * cp,
        -Math.sin(pitch)
      )
      const worldLook = camGroup.position.clone().add(look)
      const up = to3(0, 0, 1)
      camGroup.up.copy(up)
      camGroup.lookAt(worldLook)
      camGroup.rotateZ(roll)

      const fovy = fy && height
        ? THREE.MathUtils.radToDeg(2 * Math.atan((height / 2) / fy))
        : 55
      vizCam.fov = fovy
      vizCam.aspect = (width && height) ? width / height : 4 / 3
      vizCam.updateProjectionMatrix()
      helper?.update()
    }
    applyMount()

    return () => {
      cancelAnimationFrame(raf)
      ro.disconnect()
      controls.dispose()
      renderer.dispose()
      host.removeChild(renderer.domElement)
    }
  }, [tx, ty, tz, yaw, pitch, roll, width, height, fx, fy])

  return (
    <div className="camera-mount">
      <div className="section-label">Camera on robot</div>
      <div ref={hostRef} className="camera-mount-canvas" />
      <div className="camera-mount-legend">
        <span><i style={{ background: '#ef4444' }} /> +X forward</span>
        <span><i style={{ background: '#22c55e' }} /> +Y left</span>
        <span><i style={{ background: '#3b82f6' }} /> +Z up</span>
        <span className="camera-mount-hint">drag to orbit</span>
      </div>
    </div>
  )
}
