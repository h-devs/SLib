#pragma once

#include <slib.h>

#include <zxing_scanner.h>

using namespace slib;

class QRCodeApp : public MobileApp
{
	SLIB_APPLICATION(QRCodeApp)
public:
	QRCodeApp();

protected:
	void onStart() override;

private:
	Ref<ImageView> m_imgEncoded;
	Ref<EditView> m_editEncoding;
	Ref<ZXingScanner> m_QRCodeScanner;
	Ref<EditView> m_editDecoded;

};
