#pragma once
#include "pose/camera_params.h"
#include "pose/field_detection.h"
#include <cstdint>
#include <vector>

// One other robot (or configured class) after projection into field and robot frames.
struct Threat {
    int   class_id   = 0;
    float field_x    = 0;
    float field_y    = 0;
    float robot_x    = 0;  // robot-relative meters: +X forward, +Y left (WPILib)
    float robot_y    = 0;
    float range      = 0;  // hypot(robot_x, robot_y)
    float confidence = 0;
};

struct ThreatConfig {
    float            min_confidence = 0.4f;
    // Empty = every class is a threat. Set to the detector's robot class id(s)
    // when the model also sees game pieces.
    std::vector<int> class_ids;
    // Field-space merge radius (meters). Multi-camera duplicates of the same
    // object collapse to the higher-confidence detection. 0 = disabled.
    float            merge_radius   = 0.4f;
    // Ignore detections sitting on top of us (self / bumper noise).
    float            min_range      = 0.3f;
    // Ignore far projections (horizon junk, sky intersections that still hit z=0).
    float            max_range      = 8.0f;
};

// Per-frame output published to the robot: the list of other robots in view.
struct ThreatFrame {
    uint64_t            timestamp_ns = 0;
    bool                healthy      = true;
    std::vector<Threat> threats;
};

// Pure: field detections + our pose → threat list.
// No tracking, no Kalman, no history, no flee vector.
ThreatFrame select_threats(
    const std::vector<FieldDetection>& field_dets,
    const RobotPose&                   pose,
    const ThreatConfig&                cfg
);
