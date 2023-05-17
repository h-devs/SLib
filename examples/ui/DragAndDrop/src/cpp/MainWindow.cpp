#include "MainWindow.h"

void MainWindow::onCreate()
{
	DragItem drag;
	drag.setText("add-button");
	drag.setDraggingSize(100, 50);
	drag.setDraggingImage(drawable::button::get());
	btnDrag->setDragItem(drag);
	linearDrop->setDropTarget();
	linearDrop->setOnDragDropEvent([this](View*, UIEvent* ev) {
		if (ev->getDragItem().getText() == "add-button" || ev->getDragItem().getFiles().isNotNull()) {
			ev->setDragOperation(DragOperations::Copy);
			switch (ev->getAction()) {
				case UIAction::DragEnter:
					{
						Ref<Button> button = new Button;
						button->setWidthWrapping();
						button->setHeightWrapping();
						button->setCenterHorizontal();
						button->setPadding(5);
						button->setMargin(5);
						button->setBackgroundColor(Color(0, 128, 0, 255));
						button->setBoundRadius(4);
						button->setText("Button");
						button->setTextColor(Color::White);
						button->setId("btn_adding");
						linearDrop->addChild(button);
					}
					break;
				case UIAction::DragLeave:
					{
						Ref<View> button = linearDrop->findViewById("btn_adding");
						if (button.isNotNull()) {
							linearDrop->removeChild(button);
						}
					}
					break;
				case UIAction::Drop:
					{
						Ref<Button> button = Ref<Button>::from(linearDrop->findViewById("btn_adding"));
						if (button.isNotNull()) {
							if (ev->getDragItem().getFiles().isNotNull()) {
								button->setText(String::format("%s", Json(ev->getDragItem().getFiles())));
							} else {
								static int n = 0;
								n++;
								button->setText(String::format("Button%d", n));
							}
							button->setId(sl_null);
						}
					}
					break;
				default:
					break;
			}
		}
	});
}
