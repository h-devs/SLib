#include "app.h"

QRCodeExampleApp::QRCodeExampleApp()
{
}

void QRCodeExampleApp::onStart()
{
	setAvailableScreenOrientationsLandscape();
#ifdef SLIB_PLATFORM_IS_DESKTOP
	getMainWindow()->setSize(800, 450);
	getMainWindow()->setCenterScreen(sl_true);
#endif

	Ref<HorizontalLinearLayout> linear1 = new HorizontalLinearLayout;
	linear1->setWidthFilling();
	linear1->setHeightFilling();
	linear1->setBackgroundColor(Color::Gray);

	Ref<LinearLayout> linear2 = new LinearLayout;
	linear2->setWidthFilling();
	linear2->setHeightFilling();
	linear1->addChild(linear2);

	m_imgEncoded = new ImageView;
	m_imgEncoded->setWidthFilling();
	m_imgEncoded->setHeightWeight(0.7f);
	m_imgEncoded->setMargin(2);
	m_imgEncoded->setPadding(30);
	m_imgEncoded->setBackgroundColor(Color::White);
	m_imgEncoded->setScaleMode(ScaleMode::Contain);
	linear2->addChild(m_imgEncoded);

	m_editEncoding = new TextArea;
	m_editEncoding->setHintText("Input any text here to encode into QR Code");
	m_editEncoding->setOnChange([this](EditView*, String& text) {
		ZXingGenerateParam param;
		param.format = ZXingFormat::QR_CODE;
		param.width = 512;
		param.height = 512;
		param.text = text;
		m_imgEncoded->setSource(ZXing::generate(param));
	});
	m_editEncoding->setWidthFilling();
	m_editEncoding->setHeightFilling();
	m_editEncoding->setMargin(2);
	m_editEncoding->setBackgroundColor(Color::White);
	linear2->addChild(m_editEncoding);

	Ref<LinearLayout> linear3 = new LinearLayout;
	linear3->setWidthFilling();
	linear3->setHeightFilling();
	linear1->addChild(linear3);

	m_QRCodeScanner = new QRCodeScanner;
	m_QRCodeScanner->setOnDetect([this](QRCodeScanner* scanner, String code) {
		m_editDecoded->setText(String::format("[%s] %s", Time::now(), code));
	});
	m_QRCodeScanner->setWidthFilling();
	m_QRCodeScanner->setHeightWeight(0.7f);
	m_QRCodeScanner->setMargin(2);
	linear3->addChild(m_QRCodeScanner);

	m_editDecoded = new TextArea;
	m_editDecoded->setReadOnly(sl_true);
	m_editDecoded->setWidthFilling();
	m_editDecoded->setHeightFilling();
	m_editDecoded->setMargin(2);
	m_editDecoded->setBackgroundColor(Color::White);
	linear3->addChild(m_editDecoded);

	addViewToContent(linear1);

	m_QRCodeScanner->start();
}
