#pragma once

#include "app.h"

class PayPage : public ui::PayPage
{
public:
	void onOpen() override;

private:
	WeChat::PaymentOrder m_order;

};
