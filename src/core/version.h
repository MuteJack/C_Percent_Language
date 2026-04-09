// version.h — C% project metadata (shared across all executables)
#pragma once

#define CPCT_LANG_NAME "C%"
#define CPCT_VERSION "0.0.1"
#define CPCT_NIGHTLY true

// CPCT_GIT_HASH and CPCT_BUILD_TIME are injected by build system
#ifndef CPCT_GIT_HASH
#define CPCT_GIT_HASH "unknown"
#endif
#ifndef CPCT_BUILD_TIME
#define CPCT_BUILD_TIME "unknown"
#endif
