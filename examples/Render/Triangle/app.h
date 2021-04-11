#pragma once

#include <slib.h>

using namespace slib;

class TriangleApp : public UIApp
{
	SLIB_APPLICATION(TriangleApp)
public:
	TriangleApp();
	
protected:
	void onStart() override;
	
};
