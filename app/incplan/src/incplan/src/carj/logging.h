/**
 * Usage:
 * Log(Level) << "message";
 * LOG(LEVEL) << "message";
 * TIMED_FUNC(obj-name)
 * TIMED_SCOPE(obj-name, block-name)
 */

//#define ELPP_DISABLE_PERFORMANCE_TRACKING
//#define ELPP_DISABLE_VERBOSE_LOGS
//#define ELPP_DISABLE_INFO_LOGS
#define ELPP_FRESH_LOG_FILE
#define ELPP_NO_DEFAULT_LOG_FILE
#define ELPP_DISABLE_DEFAULT_CRASH_HANDLING
#include "easylogging++.h"