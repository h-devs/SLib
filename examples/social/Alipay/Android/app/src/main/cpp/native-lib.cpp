#include <slib/platform.h>

#include "../../../../../src/cpp/app.h"

JNIEXPORT jint JNI_OnLoad(JavaVM* jvm, void* reserved)
{
	Android::initialize(jvm);
	AlipayApp::main();
	return JNI_VERSION_1_4;
}
