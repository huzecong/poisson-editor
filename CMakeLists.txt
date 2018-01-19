cmake_minimum_required(VERSION 3.1)
project(PoissonEditor)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        main.cpp
        utils.h
        bitmatrix.h
        bitmatrix.cpp
        mainwindow.h
        mainwindow.cpp
        imagewindow.h
        imagewindow.cpp
        imagescene.h
        imagescene.cpp
        imagemagic.h
        poissonfusion.cpp
        smartfill.cpp)

# Add the path to the Qt installation/files
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/usr/local/opt/qt/")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

#set(CXX_FLAGS "-DQT_NO_CLIPBOARD")

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${CXX_FLAGS})

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Instruct CMake to run tools automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_AUTOMOC_MOC_OPTIONS ${CXX_FLAGS})

# Add the app icon
if (APPLE)
    set(ICON_NAME icon.icns)
    set(ICON_FILE icon/${ICON_NAME})

    if (EXISTS ${ICON_FILE})
        set_source_files_properties(${ICON_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

        set(OS_BUNDLE MACOSX_BUNDLE)
        set(MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME})
        set(MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
        set(MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION})
        set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
        set(MACOSX_BUNDLE_COPYRIGHT ${COPYRIGHT})
        set(MACOSX_BUNDLE_GUI_IDENTIFIER ${IDENTIFIER})
        set(MACOSX_BUNDLE_ICON_FILE ${ICON_NAME})

        set(META_FILES ${ICON_FILE})
    else ()
        set(META_FILES)
    endif ()
elseif (WIN32)
    # who cares
    set(OS_BUNDLE WIN32)

    set(META_FILES)
endif ()

set(QT_COMPONENTS Core Widgets Gui)
find_package(Qt5 COMPONENTS ${QT_COMPONENTS} REQUIRED)

qt5_add_resources(RESOURCE_FILES graphics.qrc)

find_package(Eigen3 REQUIRED)
find_package(OpenCV REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR} ${OpenCV_INCLUDE_DIR})

add_executable(${PROJECT_NAME} ${OS_BUNDLE} ${SOURCE_FILES} ${META_FILES} ${RESOURCE_FILES})

qt5_use_modules(${PROJECT_NAME} ${QT_COMPONENTS})

target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBRARIES})
