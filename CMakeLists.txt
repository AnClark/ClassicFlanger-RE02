cmake_minimum_required (VERSION 3.20)

project (ClassicFlanger_RE02
    VERSION 1.0.0
    LANGUAGES C CXX
)

add_subdirectory (dpf)

include_directories (src)

dpf_add_plugin (${PROJECT_NAME}
    TARGETS vst2 vst3 lv2 clap
    FILES_DSP
        src/ClassicFlangerPlugin.cpp
        src/core/Flanger.cpp
)
