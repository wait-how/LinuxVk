# LinuxVk
This repo houses random experiments with Vulkan on Linux.
## Installation
Compilation deps:
 - nproc
 - pkg-config
 - some recent version of clang++ and lld
 - vulkan headers:
   - vulkan-headers
   - vulkan-validation-layers (for debugging)
   - vulkan-tools (for the very useful vulkaninfo command)

Runtime deps (listed as arch packages):
 - a vulkan-capable driver (only intel and nvidia tested!)
 - assimp
 - glm
 - glfw

While writing things, I noticed that if you use optimus manager and try and run things on the intel GPU while in NVIDIA mode, the app crashes.  This is known behavior and is documented [here](https://github.com/Askannz/optimus-manager/wiki/FAQ,-common-issues,-troubleshooting).
