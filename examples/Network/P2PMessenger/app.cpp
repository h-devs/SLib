#include "app.h"

P2PMsgApp::P2PMsgApp()
{
}

void P2PMsgApp::onStart()
{
	String name = UI::prompt("P2PMsg", "Please input the name.", System::getUserName());
	if (name.isEmpty()) {
		UI::quitApp();
		return;
	}

	P2PSocketParam param;

	param.onReceiveBroadcast = [this](P2PSocket*, P2PNodeId& nodeId, P2PMessage& input) {
		String name = input.getJson()["name"].getString();
		if (name.isNotEmpty()) {
			String old = m_nodeNames.getValue(nodeId);
			if (old != name) {
				m_nodeNames.put(nodeId, name);
				if (old.isNull()) {
					m_lstNames->addItem(nodeId.toString(), name);
				} else {
					sl_int64 index = m_lstNames->findItemByValue(nodeId.toString());
					if (index >= 0) {
						m_lstNames->setItemTitle(index, name);
					}
				}
			}
		}
	};

	param.onReceiveMessage = [this](P2PSocket*, P2PNodeId& nodeId, P2PMessage& input, P2PResponse& output) {
		String name = m_nodeNames.getValue(nodeId);
		if (name.isNotEmpty()) {
			addToBoard("From " + name, input.getString());
			output.setString("OK");
		}
	};

	auto p2p = P2PSocket::open(param);
	if (p2p.isNull()) {
		UI::alert(String::format("P2PSocket Error: %s", param.errorText));
		UIApp::quit();
		return;
	}
	m_p2p = p2p;

	m_timerBroadcast = Timer::start([p2p, name](Timer*) {
		Json msg;
		msg.putItem("name", name);
		p2p->sendBroadcast(msg);
	}, 3000);

	m_lstNames = New<LabelList>();
	m_lstNames->setWidthFilling(0.5f, UIUpdateMode::Init);
	m_lstNames->setHeightFilling(1, UIUpdateMode::Init);
	m_lstNames->setBorder(sl_true, UIUpdateMode::Init);

	m_txtInput = New<EditView>();
	m_txtInput->setWidthFilling(1, UIUpdateMode::Init);
	m_txtInput->setHeightWrapping(UIUpdateMode::Init);

	m_txtBoard = New<EditView>();
	m_txtBoard->setMultiLine(MultiLineMode::Multiple, UIUpdateMode::Init);
	m_txtBoard->setScrolling(sl_true, sl_true, UIUpdateMode::Init);
	m_txtBoard->setGravity(Alignment::TopLeft, UIUpdateMode::Init);
	m_txtBoard->setMarginTop(5, UIUpdateMode::Init);
	m_txtBoard->setWidthFilling(1, UIUpdateMode::Init);
	m_txtBoard->setHeightFilling(1, UIUpdateMode::Init);

	auto btnSend = New<Button>();
	btnSend->setText(" Send ", UIUpdateMode::Init);
	btnSend->setWidthWrapping(UIUpdateMode::Init);
	btnSend->setHeightWrapping(UIUpdateMode::Init);
	btnSend->setMarginLeft(10, UIUpdateMode::Init);
	btnSend->setCreatingNativeWidget();
	btnSend->setOkOnClick();
	btnSend->setDefaultButton();

	auto layout1 = New<HorizontalLinearLayout>();
	layout1->addChild(m_txtInput, UIUpdateMode::Init);
	layout1->addChild(btnSend, UIUpdateMode::Init);
	layout1->setWidthFilling(1, UIUpdateMode::Init);
	layout1->setHeightWrapping(UIUpdateMode::Init);
	auto layout2 = New<LinearLayout>();
	layout2->addChild(layout1, UIUpdateMode::Init);
	layout2->addChild(m_txtBoard, UIUpdateMode::Init);
	layout2->setWidthFilling(1, UIUpdateMode::Init);
	layout2->setHeightFilling(1, UIUpdateMode::Init);
	layout2->setMarginLeft(5, UIUpdateMode::Init);
	auto layout3 = New<HorizontalLinearLayout>();
	layout3->addChild(m_lstNames, UIUpdateMode::Init);
	layout3->addChild(layout2, UIUpdateMode::Init);
	layout3->setWidthFilling(1, UIUpdateMode::Init);
	layout3->setHeightFilling(1, UIUpdateMode::Init);
	layout3->setMargin(5, UIUpdateMode::Init);
	layout3->setFontSize(16, UIUpdateMode::Init);

	auto window = New<Window>();
	window->setTitle(String::format("P2PMsg (Port=%d)", param.boundPort));
	window->setFrame(100, 100, 600, 400);
	window->setCenterScreen();
	window->setOnClose([](Window* window, UIEvent* ev) {
		UIApp::quit();
	});
	window->addView(layout3, UIUpdateMode::Init);
	window->show();
	setMainWindow(window);

	window->setOnOK([this](Window*, UIEvent*) {
		P2PNodeId nodeId(m_lstNames->getSelectedValue());
		if (nodeId.isZero()) {
			UI::alert("Please select a node!");
			return;
		}
		String msg = m_txtInput->getText();
		if (msg.isEmpty()) {
			m_txtInput->setFocus();
			return;
		}
		String name = m_lstNames->getSelectedTitle();
		m_p2p->sendMessage(nodeId, msg, [this, name, msg](P2PResponse& response) {
			if (response.getString() == "OK") {
				addToBoard("To " + name, msg);
			} else {
				addToBoard("To " + name, msg + " (Failed)");
			}
		});
		m_txtInput->setText(sl_null);
	});
}

void P2PMsgApp::onExit()
{
	if (m_p2p) {
		m_p2p->close();
	}
}

void P2PMsgApp::addToBoard(const StringParam& title, const StringParam& content)
{
	String text = String::format("%s: %s\r\n", title, content);
	m_txtBoard->appendText(text);
}
