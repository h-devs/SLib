namespace slib
{
	namespace ui
	{
		SLIB_DEFINE_WINDOW_LAYOUT(TakePhotoFromCameraDialog)

		void TakePhotoFromCameraDialog::initialize()
		{
			this->setBackgroundColor(slib::Color(0, 0, 0, 255));
			this->setCenterScreen(sl_true);
			this->setTitle(string::take_photo::get());
			this->setClientWidth(500);
			this->setClientHeight(500);

		}

		void TakePhotoFromCameraDialog::layoutViews(sl_ui_len CONTENT_WIDTH, sl_ui_len CONTENT_HEIGHT)
		{
		}

	}
}
