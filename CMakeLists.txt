cmake_minimum_required(VERSION 3.21)
project(Pomodoro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(Pomodoro
    src/main.cpp
    src/core/pomodoro.cpp
    src/core/lang.cpp
    src/data/taskdata.cpp
    src/data/notedata.cpp
    src/data/daily_task_data.cpp
    src/widgets/mainwindow.cpp
    src/widgets/pomodoro_widget.cpp
    src/widgets/taskwidget.cpp
    src/widgets/notewidget.cpp
    src/widgets/daily_task_widget.cpp
)

target_include_directories(Pomodoro PRIVATE
    src
    src/core
    src/data
    src/widgets
)
target_link_libraries(Pomodoro PRIVATE Qt6::Widgets)

if(WIN32)
    set_target_properties(Pomodoro PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
