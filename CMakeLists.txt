# Set the minimum CMake version required to build this project.
cmake_minimum_required(VERSION 3.16)
set (CMAKE_CXX_STANDARD 17)

# The name of the project.
project(MissionControl)

# Add the allwpilib subdirectory to pull in wpigui, imgui, et al.
add_subdirectory(allwpilib)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Add our main executable.
add_executable(MissionControl 
    # src/Main_test.cc)
    src/WebSocketTest.cpp src/WebSocketClientTest.cpp src/WebSocketServerTest.cpp)
    

add_executable(MVP_Server src/MVP_Server.cpp) 
target_link_libraries(MVP_Server PRIVATE wpiutil  wpinet GTest::gtest_main)

# Set include directories.
target_include_directories(MissionControl PRIVATE include)

# Add target link libraries for executable.
if(WIN32)
    set_target_properties(MissionControl PROPERTIES LINK_FLAGS "Gdi32.lib Shell32.lib d3d11.lib d3dcompiler.lib")
elseif(APPLE)
    set_target_properties(MissionControl PROPERTIES LINK_FLAGS "-framework Metal -framework QuartzCore")
elseif(UNIX)
    set_target_properties(MissionControl PROPERTIES LINK_FLAGS "-lX11")
endif()
target_link_libraries(MissionControl PRIVATE libglassnt libglass ntcore wpimath wpiutil wpigui imgui GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(MVP_Server)
