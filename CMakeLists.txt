cmake_minimum_required(VERSION 3.16)
project(VileFile VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(wxWidgets REQUIRED core base)

add_executable(VileFile main.cpp model.cpp view.cpp)
target_link_libraries(VileFile PRIVATE ${wxWidgets_LIBRARIES})
target_include_directories(VileFile PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(VileFile PRIVATE ${wxWidgets_DEFINITIONS})

find_package(Catch2 REQUIRED)

add_executable(vilefile-tests tests/test_model.cpp tests/test_utils.cpp model.cpp view.cpp)
target_link_libraries(vilefile-tests PRIVATE Catch2::Catch2WithMain ${wxWidgets_LIBRARIES})
target_include_directories(vilefile-tests PRIVATE ${CMAKE_SOURCE_DIR} ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(vilefile-tests PRIVATE ${wxWidgets_DEFINITIONS})
