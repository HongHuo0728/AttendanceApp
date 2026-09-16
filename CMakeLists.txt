cmake_minimum_required(VERSION 3.20)
project(AttendanceApp VERSION 2.0.3 LANGUAGES CXX)
enable_language(RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(AttendanceApp WIN32
    src/main.cpp
    resources/app.rc
)

target_include_directories(AttendanceApp PRIVATE include)
target_compile_definitions(AttendanceApp PRIVATE UNICODE _UNICODE)
if (MSVC)
    target_compile_options(AttendanceApp PRIVATE /utf-8 /W4 /Gy /Gw /Os)
    set_property(TARGET AttendanceApp PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    target_link_options(AttendanceApp PRIVATE /MANIFEST:NO /LTCG /OPT:REF /OPT:ICF /INCREMENTAL:NO)
endif()

target_link_libraries(AttendanceApp PRIVATE user32 gdi32 comdlg32 comctl32 shell32 ole32 uxtheme dwmapi msimg32)
