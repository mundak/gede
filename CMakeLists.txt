cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

project(gede LANGUAGES CXX)

find_package(Qt5Widgets CONFIG REQUIRED)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(gd
  "src/aboutdialog.cpp"
  "src/adatagscanner.cpp"
  "src/autovarctl.cpp"
  "src/codeview.cpp"
  "src/codeviewtab.cpp"
  "src/codeviewtab.cpp"
  "src/colorbutton.cpp"
  "src/com.cpp"
  "src/consolewidget.cpp"
  "src/core.cpp"
  "src/execombobox.cpp"
  "src/gd.cpp"
  "src/gd.cpp"
  "src/gdbmiparser.cpp"
  "src/gotodialog.cpp"
  "src/ini.cpp"
  "src/locator.cpp"
  "src/log.cpp"
  "src/mainwindow.cpp"
  "src/mainwindow.cpp"
  "src/memorydialog.cpp"
  "src/memorywidget.cpp"
  "src/opendialog.cpp"
  "src/parsecharqueue.cpp"
  "src/processlistdialog.cpp"
  "src/qtutil.cpp"
  "src/rusttagscanner.cpp"
  "src/settings.cpp"
  "src/settingsdialog.cpp"
  "src/syntaxhighlighter.cpp"
  "src/syntaxhighlighterada.cpp"
  "src/syntaxhighlighterbasic.cpp"
  "src/syntaxhighlightercxx.cpp"
  "src/syntaxhighlighterfortran.cpp"
  "src/syntaxhighlightergolang.cpp"
  "src/syntaxhighlighterrust.cpp"
  "src/tabwidgetadv.cpp"
  "src/tagmanager.cpp"
  "src/tagscanner.cpp"
  "src/tree.cpp"
  "src/util.cpp"
  "src/varctl.cpp"
  "src/variableinfowindow.cpp"
  "src/watchvarctl.cpp"

  "src/aboutdialog.ui"
  "src/codeviewtab.ui"
  "src/gotodialog.ui"
  "src/mainwindow.ui"
  "src/memorydialog.ui"
  "src/opendialog.ui"
  "src/processlistdialog.ui"
  "src/settingsdialog.ui"

  "src/resource.qrc"
  )
target_link_libraries(gd PRIVATE
  Qt5::Widgets
  )
target_include_directories(gd PRIVATE
  "src/"
  )

# Temporarely disable all warning to surpress noise
target_compile_options(gd PRIVATE -Wno-everything)
