# Pantomir - Game Engine with Vulkan

### Pre-requisites

1. Install [Visual Studio 2022](https://visualstudio.microsoft.com/vs/community/) with workloads:
   ```Desktop development with C++``` and ```Game development with C++```.
   * If you have VS2022 installed, just add a system environment variable via Windows **Edit the system environment variables** ```VCPKG_ROOT```. For example:
      ```
      C:\Program Files\Microsoft Visual Studio\2022\Community\VC\vcpkg
      ```
   * [Ninja](https://github.com/ninja-build/ninja/wiki/Pre-built-Ninja-packages) should already come with the bundled visual studio 2022 toolchain in CLion. If you want to manually do it, set PATH to ```ninja.exe``` location.
2. Install [Vulkan SDK](https://vulkan.lunarg.com/) associated with your OS. Just tick every optional box, you have
   enough storage. Check if it's in PATH.

### Building Pantomir - CLion and VSCode

1. CMakePresets does a lot of the work for you. You can open this project up in CLion or VSCode with the Cmake
   extension.
2. Make sure to set the **working directory** to the root of the project, so it can touch the Assets folder.
3. Enable the profiles I've made from CMakePresets.json for CLion.

### Notes and Concerns

* We use vcpkg for faster initial build times / rebuild times.