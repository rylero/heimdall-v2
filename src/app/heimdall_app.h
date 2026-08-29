#pragma once
#include "app/app_config.h"
#include "app/threat_processor.h"
#include "comm/nt_comm.h"
#include "pipeline/pipeline.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class HeimdallApp {
public:
    using Config = AppConfig;

    explicit HeimdallApp(Config config);
    ~HeimdallApp();

    void run();
    void stop();

private:
    Config             config_;
    NtComm             comm_;
    DeepStreamPipeline pipeline_;
    std::atomic<bool>  pipeline_healthy_{true};
    ThreatProcessor    processor_;

    std::atomic<bool>  running_{false};
    std::atomic<bool>  stopped_{false};
    std::thread        pose_recv_thread_;
    std::thread        watchdog_thread_;

    static constexpr int               kMaxDetQueue = 8;
    std::queue<std::vector<Detection>> det_queue_;
    std::mutex                         det_mutex_;
    std::condition_variable            det_cv_;
    std::thread                        det_worker_thread_;

    void enqueue_detections(const std::vector<Detection>& dets);
    void det_worker_loop();
    void pose_recv_loop();
    void watchdog_loop();
};
