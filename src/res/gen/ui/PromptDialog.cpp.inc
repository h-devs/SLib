namespace slib
{
	namespace ui
	{
		SLIB_DEFINE_WINDOW_LAYOUT(PromptDialog)

		void PromptDialog::initialize()
		{
			this->setDialog(sl_true);
			this->setCenterScreen(sl_true);
			this->setWidthWrapping();
			this->setHeightWrapping();
			m_contentView->setFont(slib::Font::create(slib::UI::getDefaultFontFamily(), 18.0f, sl_false, sl_false, sl_false), slib::UIUpdateMode::Init);

			_linear1 = new slib::LinearLayout;
			_linear1->setWidthWrapping(slib::UIUpdateMode::Init);
			_linear1->setHeightWrapping(slib::UIUpdateMode::Init);
			_linear1->setAlignParentLeft(slib::UIUpdateMode::Init);
			_linear1->setAlignParentTop(slib::UIUpdateMode::Init);
			_linear1->setMinimumWidth(slib::UIResource::toUiPos(0.2f*slib::UIResource::getScreenWidth()), slib::UIUpdateMode::Init);
			_linear1->setMarginLeft(10, slib::UIUpdateMode::Init);
			_linear1->setMarginTop(10, slib::UIUpdateMode::Init);
			_linear1->setMarginRight(10, slib::UIUpdateMode::Init);
			_linear1->setMarginBottom(10, slib::UIUpdateMode::Init);
			m_contentView->addChild(_linear1, slib::UIUpdateMode::Init);

			label = new slib::LabelView;
			label->setWidthWrapping(slib::UIUpdateMode::Init);
			label->setHeightWrapping(slib::UIUpdateMode::Init);
			label->setAlignParentLeft(slib::UIUpdateMode::Init);
			_linear1->addChild(label, slib::UIUpdateMode::Init);

			input = new slib::EditView;
			input->setWidthFilling(1.0f, slib::UIUpdateMode::Init);
			input->setHeightWrapping(slib::UIUpdateMode::Init);
			input->setAlignParentLeft(slib::UIUpdateMode::Init);
			input->setMarginTop(10, slib::UIUpdateMode::Init);
			_linear1->addChild(input, slib::UIUpdateMode::Init);

			_linear2 = new slib::LinearLayout;
			_linear2->setOrientation(slib::LayoutOrientation::Horizontal, slib::UIUpdateMode::Init);
			_linear2->setWidthWrapping(slib::UIUpdateMode::Init);
			_linear2->setHeightWrapping(slib::UIUpdateMode::Init);
			_linear2->setAlignParentRight(slib::UIUpdateMode::Init);
			_linear2->setMarginTop(10, slib::UIUpdateMode::Init);
			_linear1->addChild(_linear2, slib::UIUpdateMode::Init);

			ok = new slib::Button;
			ok->setWidthWrapping(slib::UIUpdateMode::Init);
			ok->setHeightWrapping(slib::UIUpdateMode::Init);
			ok->setAlignParentTop(slib::UIUpdateMode::Init);
			ok->setCreatingNativeWidget(sl_true);
			ok->setOkOnClick();
			ok->setText("   OK   ", slib::UIUpdateMode::Init);
			ok->setDefaultButton(sl_true, slib::UIUpdateMode::Init);
			_linear2->addChild(ok, slib::UIUpdateMode::Init);

			cancel = new slib::Button;
			cancel->setWidthWrapping(slib::UIUpdateMode::Init);
			cancel->setHeightWrapping(slib::UIUpdateMode::Init);
			cancel->setAlignParentTop(slib::UIUpdateMode::Init);
			cancel->setMarginLeft(10, slib::UIUpdateMode::Init);
			cancel->setCreatingNativeWidget(sl_true);
			cancel->setCancelOnClick();
			cancel->setText(" Cancel ", slib::UIUpdateMode::Init);
			_linear2->addChild(cancel, slib::UIUpdateMode::Init);

			setInitialFocus(input);
		}

		void PromptDialog::layoutViews(sl_ui_len CONTENT_WIDTH, sl_ui_len CONTENT_HEIGHT)
		{
		}

	}
}
