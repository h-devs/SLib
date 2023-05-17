#include "app.h"

#include "MainWindow.h"

DragAndDropApp::DragAndDropApp()
{
}

void DragAndDropApp::onStart()
{
	Ref<MainWindow> window = new MainWindow;
	window->create();
	setMainWindow(window);
}
