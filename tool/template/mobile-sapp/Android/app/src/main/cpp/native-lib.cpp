#include <slib/platform.h>

#include "../../../../../src/cpp/app.h"

JNIEXPORT jint JNI_OnLoad(JavaVM* jvm, void* reserved)
{
	Android::initialize(jvm);
	SLIB_TEMPLATE_APP_NAMEApp::main();
	return JNI_VERSION_1_4;
}
