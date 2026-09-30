cmake_minimum_required(VERSION 3.21)
project(Minesweeper VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.7 REQUIRED COMPONENTS Core Gui Widgets)

add_library(minesweeper_core
    src/core/game.cpp
    src/core/storage.cpp
    src/core/theme.cpp
    src/core/theme_source.cpp
)
target_include_directories(minesweeper_core PUBLIC src)
target_link_libraries(minesweeper_core PUBLIC Qt6::Core Qt6::Gui)
add_library(minesweeper_ui
    src/ui/board.cpp
    src/ui/modal_backdrop.cpp
    src/ui/theme_watcher.cpp
    src/ui/window.cpp
)
target_link_libraries(minesweeper_ui PUBLIC minesweeper_core Qt6::Widgets)
add_executable(minesweeper src/main.cpp src/cli.cpp)
target_link_libraries(minesweeper PRIVATE minesweeper_ui)
foreach(target minesweeper_core minesweeper_ui minesweeper)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
endforeach()

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(core_tests tests/core_tests.cpp)
    target_link_libraries(core_tests PRIVATE minesweeper_core Qt6::Test)
    add_test(NAME core COMMAND core_tests)
    add_executable(ui_tests tests/ui_tests.cpp)
    target_link_libraries(ui_tests PRIVATE minesweeper_ui Qt6::Test)
    add_test(NAME ui COMMAND ui_tests)
    set_tests_properties(ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_test(NAME cli COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_test.sh $<TARGET_FILE:minesweeper>)
    add_test(NAME install COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/install_test.sh
        $<TARGET_FILE_DIR:minesweeper> ${CMAKE_CURRENT_SOURCE_DIR}/scripts/install-desktop.sh)
endif()

include(GNUInstallDirs)
install(TARGETS minesweeper RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES assets/io.github.minesweeper.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/io.github.minesweeper.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
