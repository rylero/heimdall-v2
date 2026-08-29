#include "threat_processor.h"

ThreatProcessor::ThreatProcessor(Config config, ThreatOutput& output)
    : config_(std::move(config)),
      output_(output),
      pose_estimator_(config_.pose_cameras),
      pose_buffer_(config_.pose_buffer_capacity)
{}

void ThreatProcessor::push_pose(const TimestampedPose& p) {
    pose_buffer_.push(p);
}

void ThreatProcessor::process(const std::vector<Detection>& dets) {
    const bool healthy = !config_.healthy_flag
                       || config_.healthy_flag->load(std::memory_order_relaxed);

    ThreatFrame frame;
    frame.healthy = healthy;

    if (dets.empty()) {
        output_.send_threat_frame(frame);
        return;
    }

    const uint64_t capture_ns = dets.front().capture_monotonic_ns;
    frame.timestamp_ns = dets.front().timestamp_ns;

    const RobotPose pose       = pose_buffer_.closest(capture_ns);
    const auto      field_dets = pose_estimator_.project(dets, pose);
    frame = select_threats(field_dets, pose, config_.threat);
    frame.timestamp_ns = dets.front().timestamp_ns;
    frame.healthy      = healthy;
    output_.send_threat_frame(frame);
}
