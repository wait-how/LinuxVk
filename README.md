# LinuxVk
A base of code for vulkan work on linux.
## Installation
Make sure to use the `--recurse-submodules` option when cloning.  Run `make dbg` for a debug build or `make opt` for a release build.

Build dependencies:
 - pkg-config
 - some recent version of clang++ (g++ should work, but is not tested)

Compilation dependencies:
 - vulkan-headers
 - glm
 - assimp
 - glfw

Runtime dependencies:
 - a vulkan-capable gpu and driver
 - vulkan-validation-layers (if debugging)
 - assimp
 - glfw

Optional dependancies:
 - vulkan-tools (for the very useful vulkaninfo command)

