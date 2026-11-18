/*
 * fs24_pilot - Landing rating for MSFS 2024
 * Copyright (c) 2026 thWelly
 *
 * Licensed under the MIT License.
 * See the LICENSE file in the project root for full license text.
 */

// Composition root of the standalone module: the only place that knows every
// ring. It creates the adapters, wires them into the application use cases and
// owns their lifetime. No logic lives here.

#include <MSFS/MSFS.h>

#include <cstdio>

#include "domain/grade.hpp"

#ifndef FS24_PILOT_VERSION
#error "FS24_PILOT_VERSION must be defined by the build"
#endif

extern "C" MSFS_CALLBACK void module_init(void)
{
    std::fprintf(stdout, "[fs24-pilot] %s ready (%zu grades)\n", FS24_PILOT_VERSION, fs24::domain::GRADE_COUNT);
}

extern "C" MSFS_CALLBACK void module_deinit(void) { std::fprintf(stdout, "[fs24-pilot] stopped\n"); }
