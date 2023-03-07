cmake_minimum_required(VERSION 3.18)
project(pmix_example)

set(CMAKE_CXX_STANDARD 17)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

find_package(PMIx REQUIRED)
add_executable(main main.cpp)
target_link_libraries(main PRIVATE PMIx::PMIx)

add_executable(client client.c)
target_link_libraries(client PRIVATE PMIx::PMIx)