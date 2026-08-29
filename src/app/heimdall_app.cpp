#include "heimdall_app.h"
#include <chrono>
#include <cstdio>

static ThreatProcessor::Config make_processor_config(const AppConfig& c,
                                                     const std::atomic<bool>* health) {
    return {
        .pose_cameras         = c.pose_cameras,
        .threat               = c.threat,
        .pose_buffer_capacity = PoseBuffer::N,
        .healthy_flag         = health,
    };
}

HeimdallApp::HeimdallApp(Config config)
    : config_(std::move(config)),
      comm_(config_.nt),
      pipeline_(config_.pipeline_cameras, config_.infer_config_path,
                [this](const std::vector<Detection>& d){ enqueue_detections(d); }),
      processor_(make_processor_config(config_, &pipeline_healthy_), comm_)
{}

HeimdallApp::~HeimdallApp() { stop(); }

void HeimdallApp::enqueue_detections(const std::vector<Detection>& dets) {
    std::lock_guard lock(det_mutex_);
    while (static_cast<int>(det_queue_.size()) >= kMaxDetQueue)
        det_queue_.pop();
    det_queue_.push(dets);
    det_cv_.notify_one();
}

void HeimdallApp::det_worker_loop() {
    while (true) {
        std::vector<Detection> dets;
        {
            std::unique_lock lock(det_mutex_);
            det_cv_.wait(lock, [this]{ return !det_queue_.empty() || !running_; });
            if (!running_ && det_queue_.empty()) break;
            dets = std::move(det_queue_.front());
            det_queue_.pop();
        }
        processor_.process(dets);
    }
}

void HeimdallApp::pose_recv_loop() {
    while (running_) {
        if (auto p = comm_.try_recv_pose())
            processor_.push_pose(*p);
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
}

void HeimdallApp::watchdog_loop() {
    using namespace std::chrono;
    constexpr auto kPoll         = milliseconds(500);
    constexpr auto kStallTimeout = seconds(3);

    uint64_t last_count = pipeline_frame_count();
    auto     last_progress = steady_clock::now();
    bool     restarted = false;

    while (running_) {
        std::this_thread::sleep_for(kPoll);
        const uint64_t count = pipeline_frame_count();
        const auto now = steady_clock::now();

        if (count != last_count) {
            last_count    = count;
            last_progress = now;
            if (!pipeline_healthy_.exchange(true))
                std::printf("[watchdog] pipeline recovered — healthy\n");
            restarted = false;
            continue;
        }

        if (count > 0 && now - last_progress >= kStallTimeout) {
            if (pipeline_healthy_.exchange(false))
                std::fprintf(stderr, "[watchdog] pipeline stalled — no frames for %llds, healthy=false\n",
                             (long long)duration_cast<seconds>(now - last_progress).count());
            if (!restarted) {
                pipeline_.restart();
                restarted = true;
            }
        }
    }
}

void HeimdallApp::run() {
    running_ = true;
    det_worker_thread_ = std::thread([this]{ det_worker_loop(); });
    pose_recv_thread_  = std::thread([this]{ pose_recv_loop(); });
    watchdog_thread_   = std::thread([this]{ watchdog_loop(); });
    pipeline_.run();
}

void HeimdallApp::stop() {
    if (stopped_.exchange(true)) return;
    running_ = false;
    det_cv_.notify_all();
    pipeline_.stop();
    if (det_worker_thread_.joinable()) det_worker_thread_.join();
    if (pose_recv_thread_.joinable())  pose_recv_thread_.join();
    if (watchdog_thread_.joinable())   watchdog_thread_.join();
}
