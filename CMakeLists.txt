# Set the minimum CMake version required to build this project.
cmake_minimum_required(VERSION 3.16)

# The name of the project.
project(MissionControl)

# Add the allwpilib subdirectory to pull in wpigui, imgui, et al.
add_subdirectory(allwpilib)

# Add our main executable.
add_executable(MissionControl 
    src/Main.cc src/Plot.cc src/MyHttpConnection.cpp src/WebSocketHandlers.cpp)

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
target_link_libraries(MissionControl PRIVATE libglassnt libglass ntcore wpimath wpiutil wpigui imgui)
