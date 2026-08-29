#include "config/app_config_loader.h"
#include "config/camera_config_loader.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <vector>

using json = nlohmann::json;

AppConfig load_app_config(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("cannot open app config: " + path);

    json j = json::parse(f, nullptr, /*exceptions=*/true, /*ignore_comments=*/true);

    std::string cameras_dir = "config/cameras";
    if (j.contains("cameras_dir")) cameras_dir = j.at("cameras_dir").get<std::string>();
    auto cameras = load_camera_configs(cameras_dir);

    AppConfig cfg;
    cfg.pipeline_cameras = std::move(cameras.pipeline_cameras);
    cfg.pose_cameras     = std::move(cameras.pose_cameras);

    if (j.contains("infer_config"))
        cfg.infer_config_path = j.at("infer_config").get<std::string>();

    if (j.contains("threat")) {
        const auto& t = j.at("threat");
        if (t.contains("min_confidence"))
            cfg.threat.min_confidence = t.at("min_confidence").get<float>();
        if (t.contains("merge_radius"))
            cfg.threat.merge_radius = t.at("merge_radius").get<float>();
        if (t.contains("min_range"))
            cfg.threat.min_range = t.at("min_range").get<float>();
        if (t.contains("max_range"))
            cfg.threat.max_range = t.at("max_range").get<float>();
        if (t.contains("class_ids"))
            cfg.threat.class_ids = t.at("class_ids").get<std::vector<int>>();

        if (cfg.threat.min_confidence < 0.f || cfg.threat.min_confidence > 1.f)
            throw std::runtime_error("threat.min_confidence must be in [0, 1]");
        if (cfg.threat.merge_radius < 0.f)
            throw std::runtime_error("threat.merge_radius must be >= 0");
        if (cfg.threat.min_range < 0.f)
            throw std::runtime_error("threat.min_range must be >= 0");
        if (cfg.threat.max_range < 0.f)
            throw std::runtime_error("threat.max_range must be >= 0");
    }

    if (j.contains("nt")) {
        const auto& n = j.at("nt");
        if (n.contains("team"))     cfg.nt.team     = n.at("team").get<int>();
        if (n.contains("server"))   cfg.nt.server   = n.at("server").get<std::string>();
        if (n.contains("port"))     cfg.nt.port     = n.at("port").get<int>();
        if (n.contains("identity")) cfg.nt.identity = n.at("identity").get<std::string>();
        if (n.contains("table"))    cfg.nt.table    = n.at("table").get<std::string>();
        if (cfg.nt.port <= 0)
            throw std::runtime_error("nt.port must be > 0");
        if (cfg.nt.server.empty() && cfg.nt.team <= 0)
            throw std::runtime_error("nt.team must be > 0 when nt.server is empty");
    }

    return cfg;
}
