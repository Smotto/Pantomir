## Pre-requisites
1. Install [Vulkan SDK](https://vulkan.lunarg.com/) associated with your OS. Just tick every optional box, you have enough storage.

## Building Pantomir
1. Create a build directory and generate (solution, project files, intermediate build files):
   ```bash
   mkdir build && cd build
   cmake -G "Visual Studio 17 2022" ..
   ```
2. Open ```.sln``` in Rider / VS2022 and run **PantomirEditor** in _Debug | x64_