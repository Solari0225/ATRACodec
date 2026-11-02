# Include after the existing atracodec CLI target. Does not alter codec targets.
if(NOT ATRACODEC_BUILD_GUI)
    return()
endif()
if(NOT WIN32 AND NOT APPLE)
    message(STATUS "ATRACodec native GUI is available on Windows and macOS")
    return()
endif()
if(NOT TARGET atracodec)
    message(FATAL_ERROR "Include product/gui/cmake-native.cmake after the atracodec CLI target")
endif()
include_guard(GLOBAL)

set(_atracodec_gui_dir "${CMAKE_CURRENT_LIST_DIR}")
if(WIN32)
    # Both RC and its relative manifest live together in the generated directory.
    set(_atracodec_gui_resources "${CMAKE_CURRENT_BINARY_DIR}/converter-resources")
    file(MAKE_DIRECTORY "${_atracodec_gui_resources}")
    foreach(_file converter_windows.rc converter_windows.manifest)
        configure_file("${_atracodec_gui_dir}/${_file}" "${_atracodec_gui_resources}/${_file}" COPYONLY)
    endforeach()
    if(SONY_ZIG_EXECUTABLE)
        set(_atracodec_gui_resource "${_atracodec_gui_resources}/converter.res")
        add_custom_command(OUTPUT "${_atracodec_gui_resource}"
            COMMAND "${SONY_ZIG_EXECUTABLE}" rc /fo "${_atracodec_gui_resource}" converter_windows.rc
            WORKING_DIRECTORY "${_atracodec_gui_resources}"
            DEPENDS "${_atracodec_gui_dir}/converter_windows.rc"
                    "${_atracodec_gui_dir}/converter_windows.manifest"
                    "${_atracodec_gui_resources}/converter_windows.rc"
                    "${_atracodec_gui_resources}/converter_windows.manifest"
            VERBATIM)
        set_source_files_properties("${_atracodec_gui_resource}" PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)
    else()
        enable_language(RC)
        set(_atracodec_gui_resource "${_atracodec_gui_resources}/converter_windows.rc")
        set_source_files_properties("${_atracodec_gui_resource}" PROPERTIES
            OBJECT_DEPENDS "${_atracodec_gui_resources}/converter_windows.manifest")
    endif()
    add_executable(atracodec-converter WIN32
        "${_atracodec_gui_dir}/converter_windows.cpp" "${_atracodec_gui_resource}")
    target_include_directories(atracodec-converter PRIVATE "${_atracodec_gui_dir}" "${_atracodec_gui_resources}")
    target_link_libraries(atracodec-converter PRIVATE user32 gdi32 comdlg32 comctl32)
    if(MSVC)
        target_link_options(atracodec-converter PRIVATE /ENTRY:wWinMainCRTStartup)
    else()
        target_link_options(atracodec-converter PRIVATE -municode -Wl,--subsystem,windows -Wl,-e,WinMainCRTStartup)
    endif()
    # CLI and GUI inherit the project's bin directory. Copy also handles a caller
    # that deliberately gives the CLI a different runtime-output directory.
    add_custom_command(TARGET atracodec-converter POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:atracodec>" "$<TARGET_FILE_DIR:atracodec-converter>/$<TARGET_FILE_NAME:atracodec>"
        VERBATIM)
    install(TARGETS atracodec-converter RUNTIME DESTINATION bin)
elseif(APPLE)
    add_executable(atracodec-converter MACOSX_BUNDLE "${_atracodec_gui_dir}/converter_macos.cpp")
    set_target_properties(atracodec-converter PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${_atracodec_gui_dir}/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "ATRACodec Converter"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.atracodec.converter"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.5.0"
        MACOSX_BUNDLE_BUNDLE_VERSION "0.5.0"
        XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "11.0")
    # Prefer the host's real SDK. A Windows-to-Darwin Zig build without an SDK
    # uses these tiny import descriptions, which are not runtime dependencies.
    set(_atracodec_gui_stubs_default OFF)
    if(NOT CMAKE_HOST_APPLE AND (SONY_ZIG_EXECUTABLE OR CMAKE_CROSSCOMPILING))
        set(_atracodec_gui_stubs_default ON)
        if(CMAKE_OSX_SYSROOT AND EXISTS "${CMAKE_OSX_SYSROOT}/System/Library/Frameworks/AppKit.framework/AppKit.tbd")
            set(_atracodec_gui_stubs_default OFF)
        endif()
    endif()
    option(ATRACODEC_GUI_USE_APPLE_STUBS "Use GUI build-only Apple imports when cross-linking without an SDK" ${_atracodec_gui_stubs_default})
    if(ATRACODEC_GUI_USE_APPLE_STUBS)
        target_link_options(atracodec-converter PRIVATE
            "-F${_atracodec_gui_dir}/macos-stubs/System/Library/Frameworks"
            "-L${_atracodec_gui_dir}/macos-stubs/usr/lib")
    endif()
    target_link_options(atracodec-converter PRIVATE
        "SHELL:-framework AppKit" "SHELL:-framework Foundation" -lobjc
        -mmacosx-version-min=11.0)
    foreach(_language en zh-Hans zh-Hant)
        set(_strings "${_atracodec_gui_dir}/localizations/${_language}.lproj/InfoPlist.strings")
        set_source_files_properties("${_strings}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/${_language}.lproj")
        target_sources(atracodec-converter PRIVATE "${_strings}")
    endforeach()
    add_custom_command(TARGET atracodec-converter POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_DIR:atracodec-converter>/Contents/MacOS"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:atracodec>" "$<TARGET_BUNDLE_DIR:atracodec-converter>/Contents/MacOS/atracodec"
        VERBATIM)
    install(TARGETS atracodec-converter BUNDLE DESTINATION .)
endif()
target_compile_features(atracodec-converter PRIVATE cxx_std_17)
add_dependencies(atracodec-converter atracodec)
# Refresh the bundled/sibling CLI even when only that target changes. Ninja and
# Makefiles relink this small launcher and rerun its post-build copy.
set_property(TARGET atracodec-converter APPEND PROPERTY LINK_DEPENDS "$<TARGET_FILE:atracodec>")
