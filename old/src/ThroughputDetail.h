#pragma once
#include "includes.h"

#include "TestDetailDialog.h"

class ThroughputDetail : public TestDetailDialog
{
public:
	ThroughputDetail(wxWindow *parent, int row = 0);
	~ThroughputDetail(void);

	void RefreshList(int irow);
};
