cmake_minimum_required(VERSION 3.20)

project(th07 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TH_EXTERNAL_ASSETS "Do not bundle assets (on platforms that bundle them) and use the user/executable directories instead" OFF)

set(SOURCES
    src/AnmManager.cpp
    src/AnmVm.cpp
    src/AsciiManager.cpp
    src/BombData.cpp
    src/BulletManager.cpp
    src/Chain.cpp
    src/Controller.cpp
    src/EclManager.cpp
    src/EffectManager.cpp
    src/Ending.cpp
    src/EnemyEclInstr.cpp
    src/EnemyManager.cpp
    src/FileSystem.cpp
    src/GameErrorContext.cpp
    src/GameManager.cpp
    src/GameWindow.cpp
    src/Gui.cpp
    src/ItemManager.cpp
    src/MainMenu.cpp
    src/MidiOutput.cpp
    src/MusicRoom.cpp
    src/Player.cpp
    src/ReplayManager.cpp
    src/ResultScreen.cpp
    src/Rng.cpp
    src/ScreenEffect.cpp
    src/SoundPlayer.cpp
    src/Stage.cpp
    src/Supervisor.cpp
    src/TextHelper.cpp
    src/Touch.cpp
    src/main.cpp
    src/graphics/Gles.cpp
    src/pbg4/Lzss.cpp
    src/pbg4/Pbg4Archive.cpp
    src/pbg4/Pbg4File.cpp
    src/sj2utf8/sj2utf8.cpp
    src/utils.cpp
)

if(NINTENDO_SWITCH)
    list(APPEND SOURCES platform/switch/PlatformSwitch.cpp platform/switch/SwitchAudio.cpp)
endif()

if(ANDROID)
    add_library(main SHARED ${SOURCES})
    set(TH_EXEC_NAME "main")
else()
    add_executable(th07 ${SOURCES})
    set(TH_EXEC_NAME "th07")
endif()

target_include_directories(${TH_EXEC_NAME} PRIVATE src src/pbg4)

if(TH_EXTERNAL_ASSETS)
    target_compile_definitions(${TH_EXEC_NAME} PRIVATE TH_EXTERNAL_ASSETS)
endif()

if(EMSCRIPTEN)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

    target_compile_options(th07 PRIVATE
        "-O2"
    )
    target_link_options(th07 PRIVATE
        "--preload-file"
        "${CMAKE_SOURCE_DIR}/assets@/"
        "-sALLOW_MEMORY_GROWTH"
        "-sMAX_WEBGL_VERSION=2"
        "--use-preload-cache"
        "-O2"
        "-sINVOKE_RUN=0"
        "-sEXPORTED_RUNTIME_METHODS=['callMain']"
        "-lidbfs.js"
        "--shell-file"
        "${CMAKE_SOURCE_DIR}/resources/shell.html"
    )
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

if(NOT ANDROID AND NOT EMSCRIPTEN AND NOT APPLE AND NOT NINTENDO_SWITCH)
    find_package(OpenGL COMPONENTS GLES3 REQUIRED)
endif()

if(APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)

    set_source_files_properties(src/SoundPlayer.cpp PROPERTIES LANGUAGE OBJCXX)
    set(APP_RESOURCES
        "${CMAKE_SOURCE_DIR}/assets/th07.dat"
        "${CMAKE_SOURCE_DIR}/assets/thbgm.dat"
        "${CMAKE_SOURCE_DIR}/assets/msgothic.ttc"
    )

if(NOT TH_EXTERNAL_ASSETS)
    target_sources(${TH_EXEC_NAME} PRIVATE ${APP_RESOURCES})
    set_source_files_properties(${APP_RESOURCES} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )
endif()

if (IOS OR NOT TH_EXTERNAL_ASSETS)
    set_target_properties(${TH_EXEC_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "東方妖々夢"
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.zun.th07"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
    )
endif()

    if(IOS)
        set(BUILD_SHARED_LIBS OFF)
        set_target_properties(${TH_EXEC_NAME} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/iOS/Info.plist
        )
        target_link_libraries(${TH_EXEC_NAME} PRIVATE
            "-framework OpenGLES"
            "-framework AudioToolbox"
            "-framework AVFoundation"
            "-framework CoreAudio"
            "-framework CoreGraphics"
            "-framework QuartzCore"
            "-framework UIKit"
            "-framework Foundation"
        )
    else()
        target_compile_definitions(${TH_EXEC_NAME} PRIVATE USING_GL)
        set_target_properties(${TH_EXEC_NAME} PROPERTIES
            XCODE_ATTRIBUTE_INFOPLIST_KEY_CFBundleDisplayName "東方妖々夢　～ Perfect Cherry Blossom"
        )
        find_package(OpenGL REQUIRED)
        target_link_libraries(${TH_EXEC_NAME} PRIVATE
            OpenGL::GL
            "-framework AudioToolbox"
            "-framework CoreAudio"
            "-framework CoreVideo"
            "-framework Cocoa"
            "-framework IOKit"
            "-framework ForceFeedback"
            "-framework Carbon"
        )
    endif()
endif()

if(EMSCRIPTEN)
    # emscripten has no sdl3_image port, so it has to use a vendored sdl
    set(USE_VENDORED_SDL ON)
else()
    find_package(SDL3 QUIET)
    find_package(SDL3_ttf QUIET)
    find_package(SDL3_image QUIET)

    if(NOT SDL3_FOUND OR NOT SDL3_ttf_FOUND OR NOT SDL3_image_FOUND)
        message(STATUS "system SDL3 libraries not found. using vendored sdl3")
        set(USE_VENDORED_SDL ON)
    else()
        set(USE_VENDORED_SDL OFF)
    endif()
endif()

if(USE_VENDORED_SDL)
    set(SDLIMAGE_BACKEND_STB ON)
    set(SDLIMAGE_BACKEND_IMAGEIO OFF)

    set(SDLIMAGE_ANI OFF)
    set(SDLIMAGE_AVIF OFF)
    set(SDLIMAGE_BMP OFF)
    set(SDLIMAGE_GIF OFF)
    set(SDLIMAGE_JPG ON) # really, we only need jpg
    set(SDLIMAGE_JXL OFF)
    set(SDLIMAGE_LBM OFF)
    set(SDLIMAGE_PCX OFF)
    set(SDLIMAGE_PNG OFF)
    set(SDLIMAGE_PNM OFF)
    set(SDLIMAGE_QOI OFF)
    set(SDLIMAGE_SVG OFF)
    set(SDLIMAGE_TGA OFF)
    set(SDLIMAGE_TIF OFF)
    set(SDLIMAGE_WEBP OFF)
    set(SDLIMAGE_XCF OFF)
    set(SDLIMAGE_XPM OFF)
    set(SDLIMAGE_XV OFF)

    set(SDLTTF_VENDORED ON)
    set(SDLTTF_HARFBUZZ OFF)

    add_subdirectory(vendored/SDL EXCLUDE_FROM_ALL)
    add_subdirectory(vendored/SDL_image EXCLUDE_FROM_ALL)
    add_subdirectory(vendored/SDL_ttf EXCLUDE_FROM_ALL)
endif()

target_link_libraries(${TH_EXEC_NAME} PRIVATE
    SDL3::SDL3
    SDL3_image::SDL3_image
    SDL3_ttf::SDL3_ttf
)

if(ANDROID)
    target_link_libraries(${TH_EXEC_NAME} PRIVATE GLESv3)
elseif(NINTENDO_SWITCH)
    # Horizon: SDL3 (patched switch backend) + Mesa/nouveau GLES via EGL.
    target_compile_definitions(${TH_EXEC_NAME} PRIVATE TH_EXTERNAL_ASSETS)
    if(TH_SWITCH_NXLINK)
        target_compile_definitions(${TH_EXEC_NAME} PRIVATE TH_SWITCH_NXLINK)
    endif()
    target_include_directories(${TH_EXEC_NAME} PRIVATE
        ${DEVKITPRO}/portlibs/switch/include
    )
    target_link_directories(${TH_EXEC_NAME} PRIVATE
        ${DEVKITPRO}/portlibs/switch/lib
    )
    target_link_libraries(${TH_EXEC_NAME} PRIVATE
        GLESv2 EGL glapi drm_nouveau
        nx
    )
    # Default (all languages): romanised title. The original Japanese title is
    # patched into the Japanese slot only, so it shows up exclusively on
    # consoles whose system language is set to 日本語.
    nx_generate_nacp(
        OUTPUT ${CMAKE_BINARY_DIR}/th07.base.nacp
        NAME "Touhou 7: Perfect Cherry Blossom"
        AUTHOR "ZUN / Team Shanghai Alice"
        VERSION "1.00b-r5"
    )

    find_program(TH_PYTHON3 NAMES python3 python REQUIRED)
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/th07.nacp
        COMMAND ${TH_PYTHON3} "${CMAKE_SOURCE_DIR}/scripts/nacp_lang.py"
                ${CMAKE_BINARY_DIR}/th07.base.nacp ${CMAKE_BINARY_DIR}/th07.nacp
                --lang Japanese
                --name "東方妖々夢　～ Perfect Cherry Blossom"
                --author "上海アリス幻樂団"
                --fill-empty-name "Touhou 7: Perfect Cherry Blossom"
                --fill-empty-author "ZUN / Team Shanghai Alice"
        DEPENDS ${CMAKE_BINARY_DIR}/th07.base.nacp "${CMAKE_SOURCE_DIR}/scripts/nacp_lang.py"
        COMMENT "Patching Japanese title into NACP"
        VERBATIM
    )
    if(EXISTS "${CMAKE_SOURCE_DIR}/platform/switch/icon.jpg")
        nx_create_nro(${TH_EXEC_NAME}
            OUTPUT ${CMAKE_BINARY_DIR}/th07.nro
            NACP ${CMAKE_BINARY_DIR}/th07.nacp
            ICON "${CMAKE_SOURCE_DIR}/platform/switch/icon.jpg"
        )
    else()
        nx_create_nro(${TH_EXEC_NAME}
            OUTPUT ${CMAKE_BINARY_DIR}/th07.nro
            NACP ${CMAKE_BINARY_DIR}/th07.nacp
        )
    endif()
elseif(NOT EMSCRIPTEN AND NOT APPLE)
    target_link_libraries(${TH_EXEC_NAME} PRIVATE OpenGL::GLES3)
endif()
