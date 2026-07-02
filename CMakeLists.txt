cmake_minimum_required(VERSION 3.21)
project(Pomodoro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(Pomodoro
    src/main.cpp
    src/pomodoro.cpp
    src/pomodoro_widget.cpp
    src/mainwindow.cpp
    src/taskdata.cpp
    src/taskwidget.cpp
    src/notedata.cpp
    src/notewidget.cpp
    src/lang.cpp
)

target_include_directories(Pomodoro PRIVATE src)
target_link_libraries(Pomodoro PRIVATE Qt6::Widgets)

if(WIN32)
    set_target_properties(Pomodoro PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
