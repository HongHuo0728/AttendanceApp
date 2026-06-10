cmake_minimum_required(VERSION 3.20)
project(AttendanceApp LANGUAGES CXX)
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
    target_compile_options(AttendanceApp PRIVATE /utf-8)
endif()

target_link_libraries(AttendanceApp PRIVATE user32 gdi32 comdlg32 comctl32 shell32 ole32 uxtheme dwmapi msimg32)
