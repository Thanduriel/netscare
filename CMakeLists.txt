cmake_minimum_required (VERSION 3.1)
project (netscare)
set (CMAKE_CXX_STANDARD 17)

file(GLOB SOURCE_FILES
    "src/*.hpp"
    "src/*.cpp"
)

include(FindDirectX.cmake)

add_compile_options("/I%(WindowsSDK_IncludePath)")

include_directories(${DirectX_D3D11_INCLUDE_DIR})
message(STATUS ${DirectX_D3D11_2_INCLUDE_DIR})

include_directories(dependencies/DirectXTK/Inc)
file(GLOB DIRECTXTK_SOURCES
    "dependencies/DirectXTK/Src/*.h"
    "dependencies/DirectXTK/Src/*.cpp"
)

list(REMOVE_ITEM DIRECTXTK_SOURCES "${CMAKE_SOURCE_DIR}/dependencies/DirectXTK/Src/XboxDDSTextureLoader.cpp")

add_library(directXTK STATIC ${DIRECTXTK_SOURCES})

add_library(netscare SHARED ${SOURCE_FILES})
target_link_libraries(netscare d3d11)
target_link_libraries(netscare directXTK)

add_executable(injector injector/main.cpp src/PipeServer.cpp src/Gui.cpp)
target_link_libraries(injector netscare)
set_property(TARGET injector PROPERTY WIN32_EXECUTABLE true)

add_executable(demo WIN32 demo/X11FullScreen.cpp)
