cmake_minimum_required(VERSION 3.26)
project(Sam_sampler VERSION 1.0.0 LANGUAGES C CXX)

#==============================================================================
# MANDATORY: Use centralized JUCE detection
#==============================================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake")
include(FindJUCE)

if(NOT JUCE_FOUND)
    message(FATAL_ERROR "JUCE is required but was not found. See juce_backend/cmake/FindJUCE.cmake for installation instructions.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find JUCE
set(JUCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../external/JUCE")
if(NOT EXISTS "${JUCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at ${JUCE_DIR}")
endif()

add_subdirectory(${JUCE_DIR} "${CMAKE_BINARY_DIR}/juce")

# Sam Sampler Plugin - All 7 Required Formats
juce_add_plugin("sam_sampler"
    COMPANY_NAME "SchillingerEcosystem"
    BUNDLE_ID "com.schillingerEcosystem.sam_sampler"
    PLUGIN_IS_A_SYNTH 1
    NEEDS_MIDI_INPUT 1
    FORMATS VST3 AU CLAP LV2 Standalone
    LV2URI "http://schillingerEcosystem.com/plugins/sam_sampler"
)

target_compile_definitions(sam_sampler PRIVATE
    JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1
)

target_sources(sam_sampler
    PRIVATE
        src/SamSamplerPlugin.cpp
        src/dsp/SamSamplerDSP_Pure.cpp
        include/dsp/SamSamplerDSP.h
        ../../include/dsp/LookupTables.cpp
)

target_include_directories(sam_sampler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include  # Shared headers
)

target_link_libraries(sam_sampler PRIVATE
    juce::juce_audio_plugin_client
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

#==============================================================================
# SoundFont Sample Files
#==============================================================================
# SoundFont files are included in sf2/ directory
#
# PRODUCTION SoundFonts (Real samples):
# - salamander_grand_v1.sf2 (555MB) - Complete Salamander Grand Piano
# - roland_tr808.sf2 (2.3MB) - Roland TR-808 drum machine (real samples)
#
# TEST SoundFonts (Minimal placeholder files - 9KB each):
# - roland_tr909_test.sf2 - Roland TR-909 drum machine (test)
# - roland_tr606_test.sf2 - Roland TR-606 drum machine (test)
# - roland_tr707_test.sf2 - Roland TR-707 drum machine (test)
# - roland_tr505_test.sf2 - Roland TR-505 drum machine (test)
# - roland_tr626_test.sf2 - Roland TR-626 drum machine (test)
# - roland_cr78_test.sf2 - Roland CR-78 drum machine (test)
#
# NOTE: Test SoundFonts provide basic kick/snare/hi-hat until real samples
# can be recorded or sourced. Real samples require ~200-300MB of storage.
#
# These files will be copied to the plugin bundle Resources directory
# and available at runtime for loading
#==============================================================================

# Copy SoundFont files to plugin bundle Resources
# Note: JUCE doesn't have a built-in way to copy arbitrary data files,
# so we use a custom command to copy them after build

function(copy_soundfonts_to_bundle target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/sf2"
            $<TARGET_BUNDLE_DIR:${target}>/Contents/Resources/sf2
        COMMENT "Copying SoundFont files to ${target} bundle"
    )
endfunction()

# Copy SoundFonts for each format
copy_soundfonts_to_bundle(sam_sampler_VST3)
copy_soundfonts_to_bundle(sam_sampler_AU)
copy_soundfonts_to_bundle(sam_sampler_Standalone)

# For CLAP and LV2, they have different bundle structures
if(TARGET sam_sampler_CLAP)
    add_custom_command(TARGET sam_sampler_CLAP POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/sf2"
            $<TARGET_FILE_DIR:sam_sampler_CLAP>/sam_sampler.clap/Contents/Resources/sf2
        COMMENT "Copying SoundFont files to CLAP bundle"
    )
endif()

if(TARGET sam_sampler_LV2)
    add_custom_command(TARGET sam_sampler_LV2 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/sf2"
            ${CMAKE_CURRENT_BINARY_DIR}/sam_sampler_LV2_artefacts/sam_sampler.lv2/Resources/sf2
        COMMENT "Copying SoundFont files to LV2 bundle"
    )
endif()

