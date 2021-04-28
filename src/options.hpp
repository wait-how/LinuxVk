#pragma once

namespace options {
    constexpr static unsigned int screenWidth = 3840;
	constexpr static unsigned int screenHeight = 2160;

    // graphics options
    constexpr unsigned int msaaSamples = 2;

    // dev options
    constexpr static bool verbose = false;

#ifndef NDEBUG
	constexpr static bool debug = true;
#else
	constexpr static bool debug = false;
#endif
}