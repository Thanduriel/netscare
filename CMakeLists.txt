cmake_minimum_required (VERSION 3.1)
project (netscare)
set (CMAKE_CXX_STANDARD 17)

file(GLOB SOURCE_FILES
    "src/*.hpp"
    "src/*.cpp"
)

include(FindDirectX.cmake)

include_directories(${DirectX_XYZ_INCLUDE_DIR})

add_library(netscare SHARED ${SOURCE_FILES})
target_link_libraries(netscare d3d11)

add_executable(injector injector/main.cpp)
target_link_libraries(injector netscare)

add_executable(demo WIN32 demo/X11FullScreen.cpp)