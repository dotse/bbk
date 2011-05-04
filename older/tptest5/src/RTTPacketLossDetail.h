#pragma once
#include "includes.h"

#include "TestDetailDialog.h"

class RTTPacketLossDetail : public TestDetailDialog
{
public:
	RTTPacketLossDetail(wxWindow *parent, int row = 0);
	~RTTPacketLossDetail(void);

	void RefreshList(int irow);
};
