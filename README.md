#LinuxVk
This repo stores random experiments with Vulkan on Linux.
## Installation
For Manjaro:

I found the only thing I needed to add to my standard makefile was `pkg-config --cflags vulkan` and `pkg-config --libs vulkan`.

While writing things, I noticed that if you use optimus manager and try and run things on the intel GPU while in NVIDIA mode, the app crashes.  This is known behavior and is documented [here](https://github.com/Askannz/optimus-manager/wiki/FAQ,-common-issues,-troubleshooting).

## Things to try

I read a blog that claimed that allocating read-only data as non-device local is fine because the GPU caches it all.  Test this out with a variable amount of vertices (also can indirectly discover what the cache size is!)