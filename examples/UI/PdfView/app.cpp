#include "app.h"

PdfViewerApp::PdfViewerApp()
{
}

void PdfViewerApp::onStart()
{
	auto window = New<Window>();
	window->setTitle("PdfViewer");
	window->setFrame(30, 30, 1000, 600);
	window->setResizable();
	window->setMinimizeButtonEnabled();
	window->setMaximizeButtonEnabled();

	auto pdf = New<PdfView>();
	pdf->setWidthFilling(1, UIUpdateMode::Init);
	pdf->setHeightFilling(1, UIUpdateMode::Init);
	window->addView(pdf, UIUpdateMode::Init);

	auto menu = Menu::create();
	if (menu.isNotNull()) {
		auto file = Menu::createPopup();
		if (file.isNotNull()) {
			MenuItemParam item;
			item.text = "&Open";
			auto weakWindow = ToWeakRef(window);
			item.action = [weakWindow, pdf]() {
				FileDialog dlg;
				dlg.type = FileDialogType::OpenFile;
				dlg.parent = weakWindow;
				dlg.title = "Open PDF Document";
				dlg.addFilter("PDF Documents", "*.pdf");
				if (dlg.run() == DialogResult::OK) {
					if (!(pdf->openFile(dlg.selectedPath))) {
						UI::alert("Failed to open file!");
					}
				}
			};
			file->addMenuItem(item);
		}
		menu->addSubmenu(file, "&File");
		window->setMenu(menu);
		UIApp::setMenu(menu);
	}

	window->show();
	setMainWindow(window);
}
