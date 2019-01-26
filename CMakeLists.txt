cmake_minimum_required (VERSION 3.1)
project (netscare)

set (CMAKE_CXX_STANDARD 17)
add_compile_options( /std:c++17 )

file(GLOB SOURCE_FILES
    "src/*.hpp"
    "src/*.cpp"
)

file(GLOB SERVER_FILES
    "server/*.hpp"
    "server/*.cpp"
)

file(GLOB INJECTOR_FILES
	"injector/*.hpp"
	"injector/*.cpp"
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

file(GLOB DIRECTXTex_SOURCE
	"dependencies/DirectXTex/DirectXTex/*.h"
	"dependencies/DirectXTex/DirectXTex/*.cpp"
)

list(REMOVE_ITEM DIRECTXTK_SOURCES "${CMAKE_SOURCE_DIR}/dependencies/DirectXTK/Src/XboxDDSTextureLoader.cpp")

add_library(directXTK STATIC ${DIRECTXTK_SOURCES})
add_library(directXTex STATIC ${DIRECTXTex_SOURCE})

add_library(netscare SHARED ${SOURCE_FILES})
target_link_libraries(netscare d3d11)
target_link_libraries(netscare directXTK)

add_executable(server ${SERVER_FILES})

add_executable(injector ${INJECTOR_FILES} src/PipeServer.hpp src/PipeServer.cpp src/utils.cpp src/utils.hpp server/Messages.hpp server/Types.hpp server/Types.cpp server/Asn.hpp server/Asn.cpp)
target_link_libraries(injector netscare)
target_link_libraries(injector directXTex)
set_property(TARGET injector PROPERTY WIN32_EXECUTABLE true)

add_executable(demo WIN32 demo/X11FullScreen.cpp)
