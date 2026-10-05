cmake_minimum_required(VERSION 3.16)
project(krxbasicimgui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT WIN32)
    message(FATAL_ERROR "This project only supports Windows (DirectX 11)")
endif()

find_package(Freetype REQUIRED)

set(IMGUI_SOURCES
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    imgui_freetype.cpp
    imgui_impl_dx11.cpp
    imgui_impl_win32.cpp
)

set(IMGUI_HEADERS
    imgui.h
    imgui_internal.h
    imconfig.h
    imstb_rectpack.h
    imstb_textedit.h
    imstb_truetype.h
    imgui_freetype.h
    imgui_impl_dx11.h
    imgui_impl_win32.h
)

add_executable(${PROJECT_NAME} WIN32
    main.cpp
    menu_framework.cpp
    menu_framework.h
    loader_framework.cpp
    loader_framework.h
    imgui_text_renderer.cpp
    imgui_text_renderer.h
    ${IMGUI_SOURCES}
    ${IMGUI_HEADERS}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11.lib
    d3dcompiler.lib
    dxgi.lib
    ${FREETYPE_LIBRARIES}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    UNICODE
    _UNICODE
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
    set_property(TARGET ${PROJECT_NAME} PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

message(STATUS "===========================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "FreeType Include: ${FREETYPE_INCLUDE_DIRS}")
message(STATUS "FreeType Libraries: ${FREETYPE_LIBRARIES}")
message(STATUS "===========================================")

