# syntax=docker/dockerfile:1
# Build on the Jetson Orin Nano (linux/arm64). Do not cross-build from a laptop.
FROM nvcr.io/nvidia/deepstream-l4t:7.0-triton-multiarch

RUN apt-get update && apt-get install -y \
    cmake \
    ninja-build \
    build-essential \
    pkg-config \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    git \
    ccache \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt .
COPY src/models/bbox_parser/CMakeLists.txt src/models/bbox_parser/CMakeLists.txt
COPY tests/CMakeLists.txt tests/CMakeLists.txt
RUN mkdir -p src/pose src/threat src/comm src/pipeline src/app \
             src/models/bbox_parser src/config tools tests && \
    touch src/pose/pose_estimator.cpp \
          src/threat/threat.cpp \
          src/comm/nt_comm.cpp \
          src/pipeline/camera_source.cpp src/pipeline/probe.cpp src/pipeline/pipeline.cpp \
          src/app/heimdall_app.cpp src/app/threat_processor.cpp \
          src/main.cpp src/config/app_config_loader.cpp \
          src/models/bbox_parser/rfdetr_parser.cpp \
          src/models/bbox_parser/yolo_parser.cpp \
          src/config/camera_config_loader.cpp \
          tools/mock_robot.cpp \
          tests/test_camera_source.cpp tests/test_pipeline_chain.cpp \
          tests/test_yolo_parser.cpp \
          tests/test_pose_estimator.cpp tests/test_threat.cpp \
          tests/test_nt_comm.cpp tests/test_pose_buffer.cpp
RUN PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache

RUN --mount=type=cache,target=/root/.ccache \
    cmake --build build --parallel $(nproc) --target ntcore Catch2WithMain

COPY . .
RUN --mount=type=cache,target=/root/.ccache \
    cmake --build build --parallel $(nproc)

CMD ["/app/build/heimdall"]
