#LinuxVk
This repo stores random experiments with Vulkan on Linux.
## Installation
For Manjaro:
 - install `vulkan-headers` to get access to Vulkan header files
 - install `vulkan-validation-layers` for any debug info
 - optionally install `vulkan-tools` for the `vulkaninfo` tool, which can be used to verify that Vulkan is running properly

I found the only thing I needed to add to my standard makefile was `pkg-config --cflags vulkan` and `pkg-config --libs vulkan`.

Obviously haven't integrated shaders into my makefile, but we'll cross that bridge when we get to it.
