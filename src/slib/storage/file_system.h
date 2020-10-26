#include "slib/storage/file_system.h"

#include "slib/core/log.h"

#define TAG "FileSystem"
#define LOG(...) SLIB_LOG(TAG, ##__VA_ARGS__)
#define LOG_DEBUG(...) SLIB_LOG_DEBUG(TAG, ##__VA_ARGS__)
#define LOG_ERROR(...) SLIB_LOG_ERROR(TAG, ##__VA_ARGS__)

#ifdef SLIB_FILE_SYSTEM_CAN_THROW
#define SLIB_USE_THROW
#endif
#include "slib/core/throw.h"