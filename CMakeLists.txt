cmake_minimum_required(VERSION 3.21...3.30)

set(CMAKE_EXECUTABLE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


include(nuget.cmake)

# install nuget dependencies
# agility sdk
get_nuget_package(PACKAGE Microsoft.Direct3D.D3D12 VERSION 1.613.3)
# compiler
get_nuget_package(PACKAGE Microsoft.Direct3D.DXC VERSION 1.8.2403.18)



# set a default CXX standard for the tools and targets that do not specify them.
# If commented, the latest supported standard for your compiler is automatically set.
set(CMAKE_CXX_STANDARD 23)

add_compile_options(/W4 /WX)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Ox /DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Ox")


project(GImS VERSION 0.0.1 DESCRIPTION "" LANGUAGES CXX C)
add_subdirectory(./gimslib)
add_subdirectory(./Assignments)
add_subdirectory(./Tutorials)

# set the startup project for the "play" button in MSVC
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT T00GLM)

