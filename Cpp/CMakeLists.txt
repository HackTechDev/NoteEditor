cmake_minimum_required(VERSION 3.16)
project(NoteEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(NoteEditor
    main.cpp
    MainWindow.cpp
    MainWindow.h
    Editor.cpp
    Editor.h
    Highlighters.cpp
    Highlighters.h
    FindReplaceDialog.cpp
    FindReplaceDialog.h
    DraftsBrowser.cpp
    DraftsBrowser.h
    TrashDialog.cpp
    TrashDialog.h
    VersionHistoryDialog.cpp
    VersionHistoryDialog.h
    Session.cpp
    Session.h
)

target_link_libraries(NoteEditor PRIVATE Qt6::Widgets)
