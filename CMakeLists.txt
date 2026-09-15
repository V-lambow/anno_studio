cmake_minimum_required(VERSION 3.16)

project(AnnoStudio VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
find_package(jsoncpp CONFIG REQUIRED)

set(TS_FILES AnnoStudio_zh_CN.ts)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        ${TS_FILES}
)
include_directories(./halcon/include/)
include_directories(./utils/include/)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    find_package(Qt6 REQUIRED COMPONENTS Core)

    qt_add_executable(AnnoStudio
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        resource/resourse.qrc
        MyGraphics/mygraphicsscene.h MyGraphics/mygraphicsscene.cpp
        MyGraphics/mygraphicsview.h MyGraphics/mygraphicsview.cpp
        MyGraphics/painteditem.h MyGraphics/painteditem.cpp
        MyGraphics/rectitem.h MyGraphics/rectitem.cpp

        saige/include/saige_error.h saige/include/SaigeVisionAPI.h saige/include/SaigeVisionAPI_V1.h saige/include/SaigeVisionUtil.h
        saige/lib/SaigeVision.lib
        dlg/dlg_saigeinfer.ui
        dlg/dlg_saigeinfer.h dlg/dlg_saigeinfer.cpp dlg/dlg_saigeinfer.ui
        dlg/dlg_ultrainfer.ui dlg/dlg_ultrainfer.h dlg/dlg_ultrainfer.cpp
        dlg/dlg_labelstatistics.h dlg/dlg_labelstatistics.cpp
        dlg/dlg_dataset_split.h dlg/dlg_dataset_split.cpp
        halcon/lib/halconcpp.lib
        halcon/lib/halcon.lib
        halcon/lib/halconc.lib
        utils/src/saigeutils.cpp
        utils/include/base_head.hpp utils/include/saigeutils.h
        utils/src/base_source.cpp
        MyGraphics/inferpattern.h MyGraphics/inferpattern.cpp
        MyGraphics/visibilitymanager.h MyGraphics/visibilitymanager.cpp
        MyGraphics/roioverlayitem.h
        MyGraphics/canvasadjustmentwidget.h MyGraphics/canvasadjustmentwidget.cpp
        MyGraphics/undomanager.h MyGraphics/undomanager.cpp
        utils/include/label_exporter.h utils/src/label_exporter.cpp


    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET AnnoStudio APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    if(COMMAND qt_create_translation)
        qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
    endif()
else()
    if(ANDROID)
        add_library(AnnoStudio SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(AnnoStudio
            ${PROJECT_SOURCES}
        )
    endif()

    if(COMMAND qt5_create_translation)
        qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
    endif()
endif()

target_link_libraries(AnnoStudio PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/saige/lib/SaigeVision.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/halcon/lib/halconcpp.lib
)

if(WIN32)
    target_link_options(AnnoStudio PRIVATE /DELAYLOAD:SaigeVision.dll)
    target_link_libraries(AnnoStudio PRIVATE delayimp.lib)
endif()
  target_link_libraries(AnnoStudio PRIVATE JsonCpp::JsonCpp)
target_link_libraries(AnnoStudio PRIVATE Qt6::Core)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.AnnoStudio)
endif()
set_target_properties(AnnoStudio PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS AnnoStudio
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(AnnoStudio)
endif()
