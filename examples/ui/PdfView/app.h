#pragma once

#include <slib.h>

using namespace slib;

class PdfViewerApp : public UIApp
{
	SLIB_APPLICATION(PdfViewerApp)
public:
	PdfViewerApp();

protected:
	void onStart() override;

};
