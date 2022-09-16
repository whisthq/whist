set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(".")

set(COMMON_FILES
        modules/congestion_controller/goog_cc/trendline_estimator.cc
        modules/congestion_controller/goog_cc/inter_arrival_delta.cc
        modules/congestion_controller/goog_cc/link_capacity_estimator.cc
        modules/remote_bitrate_estimator/aimd_rate_control.cc
        modules/remote_bitrate_estimator/bwe_defines.cc
        modules/congestion_controller/goog_cc/delay_based_bwe.cc
        modules/congestion_controller/goog_cc/send_side_bandwidth_estimation.cc
        api/transport/network_types.cc
        api/transport/field_trial_based_config.cc
        api/field_trials.cc
        common_fixes.cpp
        rtc_base/experiments/struct_parameters_parser.cc
        rtc_base/experiments/field_trial_units.cc
        rtc_base/experiments/field_trial_parser.cc
        system_wrappers/source/field_trial.cc
        rtc_base/checks.cc
        )

add_executable(a.out main.cpp ${COMMON_FILES})

add_library(whistCC STATIC
        cc_impl.cpp
        ${COMMON_FILES}
        )
