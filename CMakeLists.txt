cmake_minimum_required(VERSION 3.21)
project(FluxTransfer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    add_compile_options(/W4 /permissive- /MP)
    add_compile_options(/utf-8)
    add_link_options(/SUBSYSTEM:WINDOWS)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Test)

set(SOURCES
    src/main.cpp
    src/core/FileCopyEngine.cpp
    src/core/TransferManager.cpp
    src/core/TransferTask.cpp
    src/core/DriveManager.cpp
    src/core/SizeScanner.cpp
    src/ui/MainWindow.cpp
    src/ui/SourceChecklist.cpp
    src/ui/DestinationPicker.cpp
    src/ui/TransferQueueWidget.cpp
    src/ui/ConflictDialog.cpp
    src/common/Theme.cpp
)

set(HEADERS
    src/core/FileCopyEngine.h
    src/core/TransferManager.h
    src/core/TransferTask.h
    src/core/DriveManager.h
    src/core/SizeScanner.h
    src/ui/MainWindow.h
    src/ui/SourceChecklist.h
    src/ui/DestinationPicker.h
    src/ui/TransferQueueWidget.h
    src/ui/ConflictDialog.h
    src/common/Theme.h
    src/common/Utils.h
)

add_executable(FluxTransfer ${SOURCES} ${HEADERS})

target_include_directories(FluxTransfer PRIVATE src)

target_link_libraries(FluxTransfer PRIVATE
    Qt6::Core
    Qt6::Widgets
)

set_target_properties(FluxTransfer PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

if(WIN32)
    set_target_properties(FluxTransfer PROPERTIES
        LINK_FLAGS "/ENTRY:mainCRTStartup"
    )

    get_filename_component(_QT_BIN_DIR "${Qt6_DIR}/../../../bin" ABSOLUTE)
    find_program(WINDEPLOYQT_EXE
        NAMES windeployqt6.exe windeployqt.exe
        HINTS "${_QT_BIN_DIR}"
    )

    set(DEPLOY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dist")

    add_custom_target(deploy
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPLOY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:FluxTransfer>" "${DEPLOY_DIR}"
        COMMAND "${WINDEPLOYQT_EXE}"
                --release
                --no-translations
                --no-system-d3d-compiler
                --no-opengl-sw
                --dir "${DEPLOY_DIR}"
                "${DEPLOY_DIR}/$<TARGET_FILE_NAME:FluxTransfer>"
        DEPENDS FluxTransfer
        COMMENT "Deploying Qt runtime into dist/"
        VERBATIM
    )
endif()

if(BUILD_TESTING)
    enable_testing()

    add_executable(FluxTransferTests
        tests/test_copy_verify.cpp
        src/core/FileCopyEngine.cpp
        src/core/DriveManager.cpp
    )

    target_include_directories(FluxTransferTests PRIVATE src)
    target_link_libraries(FluxTransferTests PRIVATE Qt6::Core)

    set_target_properties(FluxTransferTests PROPERTIES
        WIN32_EXECUTABLE OFF
        MACOSX_BUNDLE OFF
    )

    if(MSVC)
        target_compile_options(FluxTransferTests PRIVATE /W4 /permissive- /utf-8)
        target_link_options(FluxTransferTests PRIVATE /SUBSYSTEM:CONSOLE)
    endif()

    add_test(NAME copy_verify COMMAND FluxTransferTests)

    # Core-level progress rework tests: worker throttle, rolling speed, and
    # byte/percent invariants. Excludes the UI, so it links Core + Test only.
    add_executable(FluxTransferCoreTests
        tests/test_transfer_progress.cpp
        src/core/TransferManager.cpp
        src/core/TransferTask.cpp
        src/core/FileCopyEngine.cpp
        src/core/DriveManager.cpp
    )

    target_include_directories(FluxTransferCoreTests PRIVATE src)
    target_link_libraries(FluxTransferCoreTests PRIVATE Qt6::Core Qt6::Test)

    set_target_properties(FluxTransferCoreTests PROPERTIES
        WIN32_EXECUTABLE OFF
        MACOSX_BUNDLE OFF
    )

    if(MSVC)
        target_compile_options(FluxTransferCoreTests PRIVATE /W4 /permissive- /utf-8)
        target_link_options(FluxTransferCoreTests PRIVATE /SUBSYSTEM:CONSOLE)
    endif()

    add_test(NAME transfer_progress COMMAND FluxTransferCoreTests)

    # Widget-level tests for the source checklist (offscreen platform).
    add_executable(FluxTransferUiTests
        tests/test_checklist.cpp
        src/core/DriveManager.cpp
        src/core/SizeScanner.cpp
        src/ui/SourceChecklist.cpp
        src/common/Theme.cpp
    )

    target_include_directories(FluxTransferUiTests PRIVATE src)
    target_link_libraries(FluxTransferUiTests PRIVATE Qt6::Core Qt6::Widgets Qt6::Test)

    set_target_properties(FluxTransferUiTests PROPERTIES
        WIN32_EXECUTABLE OFF
        MACOSX_BUNDLE OFF
    )

    if(MSVC)
        target_compile_options(FluxTransferUiTests PRIVATE /W4 /permissive- /utf-8)
        target_link_options(FluxTransferUiTests PRIVATE /SUBSYSTEM:CONSOLE)
    endif()

    add_test(NAME source_checklist COMMAND FluxTransferUiTests)

    # Rendered-geometry tests for the visual fixes (offscreen platform).
    add_executable(FluxTransferVisualTests
        tests/test_visual_alignment.cpp
        src/core/DriveManager.cpp
        src/core/SizeScanner.cpp
        src/ui/SourceChecklist.cpp
        src/ui/TransferQueueWidget.cpp
        src/common/Theme.cpp
    )
    target_include_directories(FluxTransferVisualTests PRIVATE src)
    target_link_libraries(FluxTransferVisualTests PRIVATE Qt6::Core Qt6::Widgets)
    set_target_properties(FluxTransferVisualTests PROPERTIES WIN32_EXECUTABLE OFF MACOSX_BUNDLE OFF)
    if(MSVC)
        target_compile_options(FluxTransferVisualTests PRIVATE /W4 /permissive- /utf-8)
        target_link_options(FluxTransferVisualTests PRIVATE /SUBSYSTEM:CONSOLE)
    endif()
    add_test(NAME visual_alignment COMMAND FluxTransferVisualTests)

    # The test binary is not a deployed app, so it has no Qt DLLs or platform
    # plugins beside it. Point Qt at its own bin/plugins dirs at run time.
    get_filename_component(_QT_ROOT "${Qt6_DIR}/../../.." ABSOLUTE)
    set_tests_properties(copy_verify source_checklist transfer_progress visual_alignment PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen;QT_PLUGIN_PATH=${_QT_ROOT}/plugins"
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_QT_ROOT}/bin"
    )
endif()
