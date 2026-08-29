include(Catch)

if(TARGET heimdall_pipeline)
    add_executable(heimdall_pipeline_tests
        test_camera_source.cpp
        test_pipeline_chain.cpp
    )
    target_link_libraries(heimdall_pipeline_tests
        heimdall_pipeline
        Catch2::Catch2WithMain
    )
    catch_discover_tests(heimdall_pipeline_tests)

    add_executable(heimdall_parser_tests
        test_yolo_parser.cpp
    )
    target_link_libraries(heimdall_parser_tests
        yolo_parser
        Catch2::Catch2WithMain
    )
    catch_discover_tests(heimdall_parser_tests)
endif()

add_executable(heimdall_pose_tests
    test_pose_estimator.cpp
)
target_link_libraries(heimdall_pose_tests
    heimdall_pose
    Catch2::Catch2WithMain
)
catch_discover_tests(heimdall_pose_tests)

add_executable(heimdall_threat_tests
    test_threat.cpp
)
target_link_libraries(heimdall_threat_tests
    heimdall_threat
    Catch2::Catch2WithMain
)
catch_discover_tests(heimdall_threat_tests)

add_executable(heimdall_pose_buffer_tests
    test_pose_buffer.cpp
)
target_include_directories(heimdall_pose_buffer_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(heimdall_pose_buffer_tests
    Catch2::Catch2WithMain
)
catch_discover_tests(heimdall_pose_buffer_tests)

add_executable(heimdall_nt_tests
    test_nt_comm.cpp
)
target_link_libraries(heimdall_nt_tests
    heimdall_comm
    Catch2::Catch2WithMain
)
catch_discover_tests(heimdall_nt_tests)
