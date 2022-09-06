# Dependencies

Linux: Run `sudo apt update`, then run `sudo apt install libgl1-mesa-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev freeglut3-dev cmake g++`. I'm not 100% sure you need g++, it's probably already installed, need to check

Mac: TODO, probably similar to Linux though?

WSL2 on Windows: [Make sure you're on WSL 2](https://linuxhint.com/check-wsl-version/), then same as above. The first run may take a moment to install. If that still isn't working, I had luck running `sudo apt install vim-gtk3` (which I guess gets some GTK deps I'm too lazy to exactly determine). To install Ubuntu on Windows, follow [these instructions](https://ubuntu.com/tutorials/install-ubuntu-on-wsl2-on-windows-10#1-overview).

Note that with WSL2, you will need to use your computer's IP address (rather than `localhost`) when connecting to the NetworkTables server. To find this, go to task manager, click on vEthernet(WSL) under Performance, and use the IPv4 addess listed there. Also, because WSL2 uses what's basically a X server that shows up on your desktop, things won't be quite as fast and I've noticed some graphcial artifacts/flickering on plots.

Windows: Need to write these instructions, since native will always be faster than WSL2, still a pain though. TLDR: follow instructions from engine-control-software to install CMake and MSVC, that should be it I think?

# Building

Run the following commands:

- `mkdir build`
- `cd build`
- `cmake ..`
- `make -j\`nproc\`` (Ubuntu/WSL), or `cmake --build . -j\`nproc\`` (Windows) (or -j4, or however many cores you can use before your computer runs out of memory). Note that this will build wpilib once, but won't have to in the future.

- `./MissionControl` to run it