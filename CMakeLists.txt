cmake_minimum_required(VERSION 3.16)

project(Kuzgun VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/UI
)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebSockets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebSockets WebEngineWidgets)
find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets WebChannel)
find_package(Qt6 REQUIRED COMPONENTS SerialPort)
set(TS_FILES Kuzgun_en_US.ts)

set(PROJECT_SOURCES
    Source/main.cpp
    Source/flightcontroller.cpp
    Source/serialManager.cpp
    Source/home.cpp

    Header/home.h
    Header/main.h
    Header/flightcontroller.h
    Header/serialManager.h

    UI/home.ui
    UI/main.ui
    UI/flightcontroller.ui

    resources.qrc
)


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)


qt_add_executable(Kuzgun
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        Header/flightcontroller.h
        Header/MapBridge.h
        Header/serialManager.h
        Header/home.h

        Source/MapBridge.cpp
        Source/home.cpp
        Source/serialManager.cpp

        UI/home.ui
        UI/flightcontroller.ui

    )
target_include_directories(Kuzgun PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Header
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
    ${CMAKE_CURRENT_BINARY_DIR}
)

# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Kuzgun1 APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(Kuzgun SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Kuzgun
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(Kuzgun PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(Kuzgun PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_link_libraries(Kuzgun PRIVATE Qt${QT_VERSION_MAJOR}::WebSockets)
target_link_libraries(Kuzgun PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineWidgets)
target_link_libraries(Kuzgun PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::WebEngineWidgets Qt${QT_VERSION_MAJOR}::WebChannel)
target_link_libraries(Kuzgun PRIVATE Qt${QT_VERSION_MAJOR}::SerialPort)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Kuzgun1)
endif()
set_target_properties(Kuzgun PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS Kuzgun
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Kuzgun)
endif()
