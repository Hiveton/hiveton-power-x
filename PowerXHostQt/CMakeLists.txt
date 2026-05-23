cmake_minimum_required(VERSION 3.21)

project(PowerXHostQt VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

qt_add_executable(PowerXHostQt
    src/main.cpp
    src/DashboardWindow.cpp
    src/DashboardWindow.h
    src/ChartWidget.cpp
    src/ChartWidget.h
    src/DataStore.cpp
    src/DataStore.h
    src/PowerModels.h
    src/MockUsbTransport.cpp
    src/MockUsbTransport.h
    src/RecordDialog.cpp
    src/RecordDialog.h
)

target_link_libraries(PowerXHostQt PRIVATE Qt6::Widgets Qt6::Core Qt6::Gui)

if(APPLE)
    set_target_properties(PowerXHostQt PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.hiveton.powerx.host"
        MACOSX_BUNDLE_BUNDLE_NAME "PowerX Host"
    )
endif()
