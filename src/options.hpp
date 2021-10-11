#pragma once

namespace options {
    constexpr unsigned int screenWidth = 3840;
	constexpr unsigned int screenHeight = 2160;

    // graphics options
    constexpr unsigned int msaaSamples = 2;
    constexpr bool rayTracing = true;

    // dev options
    constexpr unsigned int framesInFlight = 2;
    constexpr bool verbose = false;
    constexpr bool shaderDebug = false;

#ifndef NDEBUG
	constexpr bool debug = true;
#else
	constexpr bool debug = false;
#endif
}