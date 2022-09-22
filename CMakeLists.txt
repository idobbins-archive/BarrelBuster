cmake_minimum_required(VERSION 3.23)
project(BarrelBuster)

set(CMAKE_CXX_STANDARD 17)


add_executable(BarrelBuster main.cpp)


find_package(PNG REQUIRED)
target_include_directories(BarrelBuster PUBLIC ${PNG_INCLUDE_DIR})
target_link_libraries(BarrelBuster PRIVATE  ${PNG_LIBRARY})
