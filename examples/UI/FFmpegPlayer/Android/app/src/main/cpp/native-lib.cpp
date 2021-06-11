#include <slib/core/platform.h>

#include "../../../../../src/app.h"

JNIEXPORT jint JNI_OnLoad(JavaVM* jvm, void* reserved)
{
	Android::initialize(jvm);
	FFmpegPlayerApp::main();
	return JNI_VERSION_1_4;
}
