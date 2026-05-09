#include "HUDItemSlotWidget.h"

void UHUDItemSlotWidget::InitHUDSlot(const FHUDItemSlotData& SlotData)
{
	CachedSlotData = SlotData;
	BP_OnSlotDataSet(SlotData);
}
