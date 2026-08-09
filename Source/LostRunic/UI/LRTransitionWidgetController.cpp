#include "UI/LRTransitionWidgetController.h"

void ULRTransitionWidgetController::SetTransitionVisible(const bool bVisible)
{
	if (bTransitionVisible == bVisible)
	{
		return;
	}
	bTransitionVisible = bVisible;
	OnTransitionVisibilityChanged.Broadcast(bTransitionVisible);
}
