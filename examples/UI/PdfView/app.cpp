#include "app.h"

PdfViewerApp::PdfViewerApp()
{
}

void PdfViewerApp::onStart()
{
	auto window = New<Window>();
	window->setTitle("PdfViewer");
	window->setFrame(100, 100, 400, 300);
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

	auto doc = pdf->getDocument();
	if (doc) {
		Log("Test", "%s", doc->getPagesCount());
		Memory content;
		Log("Test", "%s", PdfObject(doc->getPage(2, &content)).getVariant());
		Log("Test", "%s", PdfObject(doc->encrypt).getVariant());
		Log("Test", "%s", String::fromMemory(content));
	}
}
