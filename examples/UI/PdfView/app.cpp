#include "app.h"

PdfViewerApp::PdfViewerApp()
{
}

void PdfViewerApp::onStart()
{
	auto window = New<Window>();
	window->setTitle("PdfViewer");
	window->setFrame(30, 30, 550, 700);
	window->setOnClose([](Window* window, UIEvent* ev) {
		UIApp::quit();
	});

	auto pdf = New<PdfView>();
	pdf->setWidthFilling(1, UIUpdateMode::Init);
	pdf->setHeightFilling(1, UIUpdateMode::Init);
	window->addView(pdf, UIUpdateMode::Init);

	window->show();
	setMainWindow(window);

	pdf->openFile("D:\\Temp\\1.pdf");

	//File::writeAllBytes("D:\\Temp\\2.dat", pdf->getDocument()->getPage(0)->getContentStream());
}
