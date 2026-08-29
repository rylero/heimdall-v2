cmake_minimum_required(VERSION 3.22)
project(heimdall VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# ---------------------------------------------------------------------------
# nlohmann/json
# ---------------------------------------------------------------------------
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

# ---------------------------------------------------------------------------
# Catch2
# ---------------------------------------------------------------------------
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

# ---------------------------------------------------------------------------
# ntcore (WPILib NetworkTables) — rio is the server, we are a client
# ---------------------------------------------------------------------------
set(WITH_JAVA               OFF CACHE BOOL "" FORCE)
set(WITH_JAVA_SOURCE        OFF CACHE BOOL "" FORCE)
set(WITH_CSCORE             OFF CACHE BOOL "" FORCE)
set(WITH_WPILIB             OFF CACHE BOOL "" FORCE)
set(WITH_WPIMATH            OFF CACHE BOOL "" FORCE)
set(WITH_GUI                OFF CACHE BOOL "" FORCE)
set(WITH_TESTS              OFF CACHE BOOL "" FORCE)
set(WITH_SIMULATION_MODULES OFF CACHE BOOL "" FORCE)
set(WITH_EXAMPLES           OFF CACHE BOOL "" FORCE)
set(WITH_BENCHMARK          OFF CACHE BOOL "" FORCE)
set(WITH_WPICAL             OFF CACHE BOOL "" FORCE)
set(WITH_PROTOBUF           OFF CACHE BOOL "" FORCE)
set(WITH_NTCORE             ON  CACHE BOOL "" FORCE)
set(WITH_DOCS               OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS       OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    allwpilib
    GIT_REPOSITORY https://github.com/wpilibsuite/allwpilib.git
    GIT_TAG        v2026.2.1
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(allwpilib)

# allwpilib rewrites the global output dirs; put *our* binaries back in the build root.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ---------------------------------------------------------------------------
# Pose estimation — no hardware dependency
# ---------------------------------------------------------------------------
add_library(heimdall_pose
    src/pose/pose_estimator.cpp
)
target_include_directories(heimdall_pose PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---------------------------------------------------------------------------
# Threat selection — no hardware dependency
# ---------------------------------------------------------------------------
add_library(heimdall_threat
    src/threat/threat.cpp
)
target_include_directories(heimdall_threat PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(heimdall_threat PUBLIC heimdall_pose)

# ---------------------------------------------------------------------------
# Comm — NetworkTables
# ---------------------------------------------------------------------------
add_library(heimdall_comm
    src/comm/nt_comm.cpp
)
target_include_directories(heimdall_comm PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(heimdall_comm
    PUBLIC heimdall_threat ntcore
)

# ---------------------------------------------------------------------------
# Core — pose lookup + projection + threat select
# ---------------------------------------------------------------------------
add_library(heimdall_core
    src/app/threat_processor.cpp
    src/config/camera_config_loader.cpp
    src/config/app_config_loader.cpp
)
target_include_directories(heimdall_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(heimdall_core
    PUBLIC heimdall_comm heimdall_pose heimdall_threat nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------------------------
# Mock robot — NT server that publishes a fake pose (bench without a rio)
# ---------------------------------------------------------------------------
add_executable(mock_robot tools/mock_robot.cpp)
target_link_libraries(mock_robot PRIVATE ntcore)
set_target_properties(mock_robot PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ---------------------------------------------------------------------------
# GStreamer / DeepStream (Jetson Docker image)
# ---------------------------------------------------------------------------
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GST gstreamer-1.0)
endif()

set(DS_ROOT "/opt/nvidia/deepstream/deepstream" CACHE PATH "DeepStream SDK root")
set(DS_INCLUDES "${DS_ROOT}/sources/includes")
set(DS_LIB_DIR  "${DS_ROOT}/lib")

find_library(NVDS_META_LIB     nvds_meta     PATHS ${DS_LIB_DIR})
find_library(NVDSGST_META_LIB  nvdsgst_meta  PATHS ${DS_LIB_DIR})

if(GST_FOUND AND NVDS_META_LIB AND NVDSGST_META_LIB)
    add_library(heimdall_pipeline
        src/pipeline/camera_source.cpp
        src/pipeline/probe.cpp
        src/pipeline/pipeline.cpp
    )
    target_include_directories(heimdall_pipeline PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${DS_INCLUDES}
        ${GST_INCLUDE_DIRS}
    )
    target_link_libraries(heimdall_pipeline
        PUBLIC
            ${GST_LIBRARIES}
            ${NVDS_META_LIB}
            ${NVDSGST_META_LIB}
    )

    add_executable(heimdall
        src/main.cpp
        src/app/heimdall_app.cpp
    )
    target_include_directories(heimdall PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_link_libraries(heimdall
        heimdall_core
        heimdall_pipeline
    )
    set_target_properties(heimdall PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

    add_subdirectory(src/models/bbox_parser)
else()
    message(STATUS "GStreamer/DeepStream not found — skipping pipeline and main executable (pose/threat tests still build).")
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
