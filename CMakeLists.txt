cmake_minimum_required(VERSION 3.20)

project(novadesk_runner LANGUAGES C CXX)
option(NOVADESK_ENABLE_LTO "Enable LTO for MinGW MinSizeRel builds" OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE MinSizeRel CACHE STRING "Build type" FORCE)
endif()

set(APP_SRC_DIR "${CMAKE_SOURCE_DIR}/src/apps/novadesk")
set(THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/src/third_party")
set(QUICKJS_DIR "${THIRD_PARTY_DIR}/quick-js")

file(GLOB_RECURSE NOVADESK_APP_SOURCES CONFIGURE_DEPENDS
    "${APP_SRC_DIR}/*.cpp"
)

add_executable(novadesk WIN32
    ${NOVADESK_APP_SOURCES}
    "${APP_SRC_DIR}/Novadesk.rc"
    "${QUICKJS_DIR}/quickjs.c"
    "${QUICKJS_DIR}/libregexp.c"
    "${QUICKJS_DIR}/libunicode.c"
    "${QUICKJS_DIR}/dtoa.c"
)

target_compile_definitions(novadesk PRIVATE UNICODE _UNICODE)

target_include_directories(novadesk PRIVATE
    "${APP_SRC_DIR}"
    "${APP_SRC_DIR}/domain"
    "${APP_SRC_DIR}/shared"
    "${APP_SRC_DIR}/render"
    "${APP_SRC_DIR}/scripting/quickjs"
    "${APP_SRC_DIR}/scripting/quickjs/parser"
    "${QUICKJS_DIR}"
)

target_link_libraries(novadesk PRIVATE
    user32
    gdi32
    opengl32
    winmm
    powrprof
    shell32
    comctl32
    d2d1
    dwrite
    windowscodecs
    shlwapi
    iphlpapi
    pdh
    wininet
    version
)

if(MINGW)
    # Statically link MinGW runtimes so dist does not require
    # libgcc/libstdc++/libwinpthread DLLs next to novadesk.exe.
    target_link_options(novadesk PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()

if(MINGW)
    # Release-size profile: MinSizeRel + LTO + gc-sections + strip.
    target_compile_options(novadesk PRIVATE
        $<$<CONFIG:MinSizeRel>:-Os -ffunction-sections -fdata-sections -s>
    )
    target_link_options(novadesk PRIVATE
        $<$<CONFIG:MinSizeRel>:-Wl,--gc-sections -s>
    )
    if(NOVADESK_ENABLE_LTO)
        target_compile_options(novadesk PRIVATE
            $<$<CONFIG:MinSizeRel>:-flto>
        )
        target_link_options(novadesk PRIVATE
            $<$<CONFIG:MinSizeRel>:-flto>
        )
    endif()
endif()
