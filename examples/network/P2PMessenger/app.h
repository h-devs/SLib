#pragma once

#include <slib.h>
#include <p2p.h>

using namespace slib;

class P2PMsgApp : public UIApp
{
	SLIB_APPLICATION(P2PMsgApp)
public:
	P2PMsgApp();

protected:
	void onStart() override;

	void onExit() override;

private:
	void addToBoard(const StringParam& title, const StringParam& content);

protected:
	Ref<P2PSocket> m_p2p;
	Ref<Timer> m_timerBroadcast;

	Ref<LabelList> m_lstNames;
	Ref<EditView> m_txtBoard;
	Ref<EditView> m_txtInput;

	CHashMap<P2PNodeId, String> m_nodeNames;

};
