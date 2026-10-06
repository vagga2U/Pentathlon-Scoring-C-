cmake_minimum_required(VERSION 3.16)

project(PentathlonEventManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 QUIET COMPONENTS Widgets PrintSupport)
if (Qt6_FOUND)
    set(QT_PACKAGE Qt6)
else ()
    find_package(Qt5 REQUIRED COMPONENTS Widgets PrintSupport)
    set(QT_PACKAGE Qt5)
endif ()

add_executable(PentathlonEventManager WIN32
    src/main.cpp
    src/disciplines.h
    src/eventmodel.h
    src/eventmodel.cpp
    src/csvimporter.h
    src/csvimporter.cpp
    src/ruleset.h
    src/ruleset.cpp
    src/setuptab.h
    src/setuptab.cpp
    src/disciplinetab.h
    src/disciplinetab.cpp
    src/fencinggrouptab.h
    src/fencinggrouptab.cpp
    src/finalresultstab.h
    src/finalresultstab.cpp
    src/mainwindow.h
    src/mainwindow.cpp
)

target_link_libraries(PentathlonEventManager
    PRIVATE
        ${QT_PACKAGE}::Widgets
        ${QT_PACKAGE}::PrintSupport
)

target_include_directories(PentathlonEventManager PRIVATE src)

add_custom_command(TARGET PentathlonEventManager POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/rulesets
        $<TARGET_FILE_DIR:PentathlonEventManager>/rulesets
)
