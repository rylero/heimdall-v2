# Heimdall v2

Jetson **Orin Nano** vision for one job: **if another robot is in view, tell ours to drive the other way.**

Build and run **on the Jetson**. DeepStream is the proven v1 stack. This repo drops JPDAF, ZMQ, and protobuf. The rio already runs NetworkTables; Heimdall is just another NT client.

```
Camera(s) → DeepStream (nvinfer) → detections
                                      ↓
                                PoseEstimator  ← NT /heimdall/pose/{x,y,heading}
                                      ↓
                                select_threats
                                      ↓
                                NT /heimdall/hasThreat, fleeX, fleeY
                                      ↓
                                Robot: if hasThreat, drive (fleeX, fleeY)
```

## NetworkTables (`/heimdall`)

Robot **publishes** (odometry, every loop):

| Topic | Type |
|---|---|
| `pose/x` | double, field m |
| `pose/y` | double, field m |
| `pose/heading` | double, rad CCW from +X |

Jetson **publishes**:

| Topic | Type |
|---|---|
| `hasThreat` | bool |
| `fleeX` / `fleeY` | double, unit vector, robot frame (+X forward, +Y left) |
| `nearestRange` | double, meters |
| `healthy` | bool |
| `threatCount` | int |

Copy `rio/Heimdall.java` into the robot project (WPILib only, no extra vendordep):

```java
heimdall.sendPose(pose.getX(), pose.getY(), pose.getRotation().getRadians());
if (heimdall.hasThreat()) {
    drive.drive(heimdall.fleeX() * maxSpeed, heimdall.fleeY() * maxSpeed, 0.0);
}
```

Set `nt.team` in `config/heimdall.jsonc` (default 6238 → `10.62.38.2`). Leave `nt.server` empty unless you need an override (e.g. `"127.0.0.1"` with `mock_robot`).

There is no track history. A frame with no detections means `hasThreat == false`.

## Jetson notes (Orin Nano)

- **No NVENC.** Debug preview encodes with `x264enc`. Detection is upstream of a leaky queue so software encode cannot stall nvinfer.
- **One NvJPEG unit.** USB cameras use `jpegdec` (`hw_decode: false` in the camera JSONC).
- **DeepStream 7.0 / L4T** image: `nvcr.io/nvidia/deepstream-l4t:7.0-triton-multiarch`.
- Copy engines and labels from the v1 `models/` tree into this repo's `models/`.

## Config

`config/heimdall.jsonc` — threat thresholds, NT team/server, infer config path.  
`config/cameras/*.jsonc` — one file per camera.

`threat.class_ids` is empty by default (every detector class is a threat). Set it when the model also sees game pieces.

`nvinfer` `batch-size` must match the camera count.

## Run on the Jetson

```bash
cd docker
docker compose build && docker compose up
```

First image build compiles ntcore inside the container. Optional max clocks:

```bash
sudo nvpmodel -m 0 && sudo jetson_clocks
```

Debug preview (MediaMTX): `rtsp://<jetson-ip>:8554/live/ds-test`.
