#pragma once
#include "app/pose_buffer.h"
#include "app/threat_output.h"
#include "pipeline/detection.h"
#include "pose/pose_estimator.h"
#include "threat/threat.h"
#include <atomic>
#include <cstdint>
#include <vector>

// Hardware-free post-detection pipeline: pose lookup → projection → threat select → output.
class ThreatProcessor {
public:
    struct Config {
        std::vector<CameraParams> pose_cameras;
        ThreatConfig              threat;
        size_t                    pose_buffer_capacity = PoseBuffer::N;
        const std::atomic<bool>*  healthy_flag = nullptr;
    };

    ThreatProcessor(Config config, ThreatOutput& output);

    void push_pose(const TimestampedPose& p);
    void process(const std::vector<Detection>& dets);

private:
    Config         config_;
    ThreatOutput&  output_;
    PoseEstimator  pose_estimator_;
    PoseBuffer     pose_buffer_;
};
