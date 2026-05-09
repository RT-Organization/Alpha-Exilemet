#include "HUDVacuumSegmentWidget.h"

void UHUDVacuumSegmentWidget::InitHUDSegment(const FHUDVacuumSegmentData& SegData)
{
	CachedSegmentData = SegData;
	BP_OnSegmentDataSet(SegData);
}
