#pragma once
#include "comm/nt_comm.h"
#include "pipeline/camera_source.h"
#include "pose/camera_params.h"
#include "threat/threat.h"
#include <string>
#include <vector>

struct AppConfig {
    std::vector<CameraConfig> pipeline_cameras;
    std::vector<CameraParams> pose_cameras;
    std::string               infer_config_path = "config/infer_yolo26n.txt";
    ThreatConfig              threat;
    NtComm::Config            nt;
};
