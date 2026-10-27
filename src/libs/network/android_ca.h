#pragma once

#if defined(__ANDROID__)

#include <cpr/cpr.h>

cpr::SslOptions android_ca();

#endif
