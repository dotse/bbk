#pragma once
#include "includes.h"

#include "TestDetailDialog.h"

class AvailabilityDetail : public TestDetailDialog
{
public:
	AvailabilityDetail(wxWindow *parent, int row = 0);
	~AvailabilityDetail(void);

	void RefreshList(int row);
};
