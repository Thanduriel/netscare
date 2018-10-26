cmake_minimum_required (VERSION 3.1)
project (netscare)
set (CMAKE_CXX_STANDARD 17)

file(GLOB SOURCE_FILES
    "src/*.hpp"
    "src/*.cpp"
)

include(FindDirectX.cmake)

include_directories(${DirectX_XYZ_INCLUDE_DIR})

add_executable(netscare ${SOURCE_FILES})

add_executable(demo WIN32 demo/X11FullScreen.cpp)

# detours
include_directories("dependencies/detours/src")
#file(GLOB SOURCE_FILES_DETOURS "dependencies/detours/src/*.cpp")

add_library(detours STATIC dependencies/detours/src/detours.cpp 
	dependencies/detours/src/modules.cpp 
	dependencies/detours/src/disasm.cpp 
	dependencies/detours/src/image.cpp 
	dependencies/detours/src/creatwth.cpp 
	dependencies/detours/src/disolx86.cpp 
	dependencies/detours/src/disolx64.cpp 
	dependencies/detours/src/disolia64.cpp)
target_link_libraries (netscare detours)