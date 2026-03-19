cmake_minimum_required(VERSION 3.22)

project(
  DAW
  VERSION 0.1.0
  DESCRIPTION "An open-source digital audio workstation"
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET
      "10.15"
      CACHE STRING "Minimum macOS version")
endif()

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)

FetchContent_Declare(
  JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.6
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# Only add the modules subdirectory to avoid conflicting with our JUCE
FetchContent_Declare(
  tracktion_engine
  GIT_REPOSITORY https://github.com/Tracktion/tracktion_engine.git
  GIT_TAG v3.2.0
  GIT_SHALLOW TRUE)
FetchContent_GetProperties(tracktion_engine)
if(NOT tracktion_engine_POPULATED)
  FetchContent_Populate(tracktion_engine)
  add_subdirectory(${tracktion_engine_SOURCE_DIR}/modules
                   ${tracktion_engine_BINARY_DIR}/modules)
endif()

juce_add_gui_app(
  DAW
  PRODUCT_NAME
  "DAW"
  COMPANY_NAME
  "DAW Project"
  BUNDLE_ID
  "org.dawproject.daw"
  MICROPHONE_PERMISSION_ENABLED
  TRUE
  MICROPHONE_PERMISSION_TEXT
  "DAW needs microphone access for audio recording."
  PLUGINHOST_AU
  TRUE
  NEEDS_MIDI_INPUT
  TRUE
  NEEDS_MIDI_OUTPUT
  TRUE)

juce_generate_juce_header(DAW)

target_sources(DAW PRIVATE src/Main.cpp src/MainComponent.cpp
                           src/MainComponent.h)

target_compile_definitions(
  DAW
  PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_USE_CURL=0
    JUCE_WEB_BROWSER=0
    JUCE_STRICT_REFCOUNTEDPOINTER=1
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:DAW,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:DAW,JUCE_VERSION>"
    TRACKTION_ENABLE_TIMESTRETCH_SOUNDTOUCH=1)

target_link_libraries(
  DAW
  PRIVATE tracktion::tracktion_engine
          tracktion::tracktion_core
          tracktion::tracktion_graph
          juce::juce_audio_devices
          juce::juce_audio_formats
          juce::juce_audio_processors
          juce::juce_audio_utils
          juce::juce_dsp
          juce::juce_gui_basics
          juce::juce_gui_extra
          juce::juce_recommended_config_flags
          juce::juce_recommended_warning_flags)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(DAW PRIVATE "-latomic")
endif()

option(DAW_BUILD_TESTS "Build tests" ON)

if(DAW_BUILD_TESTS)
  enable_testing()

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(Catch2)

  add_executable(DAWTests tests/main_test.cpp)

  target_compile_features(DAWTests PRIVATE cxx_std_20)

  target_link_libraries(DAWTests PRIVATE Catch2::Catch2WithMain)

  include(CTest)
  include(Catch)
  catch_discover_tests(DAWTests)
endif()
