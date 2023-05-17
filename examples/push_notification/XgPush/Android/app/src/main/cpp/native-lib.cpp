#include <slib/platform.h>

#include "../../../../../src/app.h"

JNIEXPORT jint JNI_OnLoad(JavaVM* jvm, void* reserved)
{
	Android::initialize(jvm);
	XgPushApp::main();
	return JNI_VERSION_1_4;
}
