# LinuxVk
This repo houses random experiments with Vulkan on Linux.
## Installation
Packages used (on Manjaro Linux):
 - assimp
 - glm
 - glfw

I found the only thing I needed to add to my standard makefile was `pkg-config --cflags vulkan` and `pkg-config --libs vulkan`.

While writing things, I noticed that if you use optimus manager and try and run things on the intel GPU while in NVIDIA mode, the app crashes.  This is known behavior and is documented [here](https://github.com/Askannz/optimus-manager/wiki/FAQ,-common-issues,-troubleshooting).
