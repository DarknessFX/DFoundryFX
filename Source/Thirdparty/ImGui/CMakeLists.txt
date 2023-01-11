cmake_minimum_required(VERSION 3.13)

project(ImGui)

include_directories(imgui/)

set(HEADER_FILES
    imgui/imgui.h
    imgui/imconfig.h
    imgui/imgui_internal.h
    imgui/imstb_textedit.h
    imgui/imstb_rectpack.h
    imgui/imstb_truetype.h
    imgui/implot.h
    imgui/implot_internal.h)

set(SOURCE_FILES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/implot.cpp
    imgui/implot_items.cpp
    imgui/imgui_demo.cpp
    imgui/implot_demo.cpp)

add_library(ImGui STATIC ${HEADER_FILES} ${SOURCE_FILES})

set_target_properties(ImGui
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/../Binaries/")