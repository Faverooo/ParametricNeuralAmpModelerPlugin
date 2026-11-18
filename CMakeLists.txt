cmake_minimum_required(VERSION 3.22)
project(ParametricNAM VERSION 0.1.0 LANGUAGES C CXX)
set(PNAM_RELEASE_TAG "" CACHE STRING "Release tag used for update notifications")
set(PNAM_BUILD_VERSION "${PROJECT_VERSION}")
if(PNAM_RELEASE_TAG MATCHES "^v([0-9]+[.][0-9]+[.][0-9]+)([-+][0-9A-Za-z.+-]+)?$")
    set(PNAM_BUILD_VERSION "${CMAKE_MATCH_1}")
endif()


set(PNAM_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../ParametricNeuralAmpModelerCore" CACHE PATH "Local PNAM Core checkout")
set(PNAM_EIGEN_DIR "${PNAM_CORE_DIR}/Dependencies/eigen" CACHE PATH "Eigen source directory")
set(PNAM_JUCE_DIR "" CACHE PATH "Optional local JUCE source directory")
option(PNAM_BUILD_VST3 "Build the ParametricNAM VST3" ON)

if(NOT EXISTS "${PNAM_CORE_DIR}/NAM/pnam.cpp")
    message(FATAL_ERROR "Set PNAM_CORE_DIR to a Core checkout with PNAM support")
endif()
if(NOT EXISTS "${PNAM_EIGEN_DIR}/Eigen/Core")
    message(FATAL_ERROR "Initialize the Core Eigen submodule or set PNAM_EIGEN_DIR")
endif()
file(GLOB core_sources CONFIGURE_DEPENDS "${PNAM_CORE_DIR}/NAM/*.cpp" "${PNAM_CORE_DIR}/NAM/*/*.cpp")
add_library(pnam_core STATIC ${core_sources})
set_target_properties(pnam_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(pnam_core PUBLIC cxx_std_20)
target_compile_definitions(pnam_core PUBLIC NAM_SAMPLE_FLOAT)
if(WIN32)
    target_compile_definitions(pnam_core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
target_include_directories(pnam_core PUBLIC "${PNAM_CORE_DIR}")
target_include_directories(pnam_core SYSTEM PUBLIC "${PNAM_EIGEN_DIR}" "${PNAM_CORE_DIR}/Dependencies/nlohmann")

if(PNAM_BUILD_VST3)
    if(PNAM_JUCE_DIR)
        add_subdirectory("${PNAM_JUCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/juce")
    else()
        find_package(JUCE 8 CONFIG QUIET)
        if(NOT TARGET juce::juce_audio_processors)
            include(FetchContent)
            FetchContent_Declare(JUCE
                GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
                GIT_TAG 8.0.6
                GIT_SHALLOW TRUE)
            FetchContent_MakeAvailable(JUCE)
        endif()
    endif()

    juce_add_plugin(ParametricNAM
        VERSION "${PNAM_BUILD_VERSION}"
        NEEDS_CURL TRUE
        COMPANY_NAME "Faverooo"
        BUNDLE_ID "com.faverooo.parametricnam"
        PLUGIN_MANUFACTURER_CODE Fvro
        PLUGIN_CODE Pnam
        FORMATS VST3
        PRODUCT_NAME "ParametricNAM"
        VST3_CATEGORIES Fx Distortion)
    target_sources(ParametricNAM PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(ParametricNAM PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=1 PNAM_BUILD_VERSION="${PNAM_BUILD_VERSION}" JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(ParametricNAM PRIVATE pnam_core juce::juce_audio_processors)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(test_level_match tests/level_match.cpp)
    target_include_directories(test_level_match PRIVATE Source)
    target_compile_features(test_level_match PRIVATE cxx_std_20)
    add_test(NAME level_match COMMAND test_level_match)
endif()
