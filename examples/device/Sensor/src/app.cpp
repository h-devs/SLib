#include "app.h"

SensorApp::SensorApp()
{
}

void SensorApp::onStart()
{
	getContentView()->setBackgroundColor(Color::White);

	auto label = New<LabelView>();
	label->setWidthWrapping(UIUpdateMode::Init);
	label->setHeightWrapping(UIUpdateMode::Init);
	label->setCenterInParent(UIUpdateMode::Init);
	label->setFont(Font::create("Arial", UI::dpToPixel(20)), UIUpdateMode::Init);
	label->setMultiLine(MultiLineMode::Multiple);
	addViewToContent(label);

	auto readData = [this, label]() {
		SensorParam param;
		param.flagUseLocation = sl_true;
		param.onLocationChanged = [label](Sensor*, const GeoLocation& location) {
			label->setText(String::format("Latitude = %.4f\nLongitude = %.4f\nAltitude = %.4f", location.latitude, location.longitude, location.altitude));
		};
		m_sensor = Sensor::create(param);
	};
#ifdef SLIB_PLATFORM_IS_ANDROID
	Setting::grantPermissions(AppPermissions::AccessFineLocation, readData);
#else
	readData();
#endif

}
