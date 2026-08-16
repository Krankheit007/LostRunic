#include "UI/LRSaveWidgetController.h"

#include "Data/LRGameContentSet.h"

namespace
{
	FText ResolveSaveText(const ULRGameContentSet* contentSet, const FName textKey)
	{
		return contentSet ? contentSet->ResolveUIText(textKey) : FText::FromName(textKey);
	}

	FName HealthTextKey(const ELRSaveSlotHealth health)
	{
		switch (health)
		{
		case ELRSaveSlotHealth::Healthy: return TEXT("SaveHealthHealthy");
		case ELRSaveSlotHealth::MissingPayload: return TEXT("SaveHealthMissing");
		case ELRSaveSlotHealth::CorruptPayload: return TEXT("SaveHealthCorrupt");
		case ELRSaveSlotHealth::UnsupportedVersion: return TEXT("SaveHealthUnsupported");
		case ELRSaveSlotHealth::CatalogMismatch: return TEXT("SaveHealthMismatch");
		case ELRSaveSlotHealth::UnknownSlotType: return TEXT("SaveHealthUnknown");
		case ELRSaveSlotHealth::InvalidData: return TEXT("SaveHealthInvalid");
		default: return TEXT("SaveHealthInvalid");
		}
	}
}

FLRSaveUISnapshot ULRSaveWidgetController::BuildSnapshot(const TArray<FLRSaveSlotMetadata>& slots,
	const ELRSaveSelectionMode mode, const ELRSaveUIState state, const bool bManualSaveAllowed,
	const int32 maxManualSlots, const ULRGameContentSet* contentSet)
{
	FLRSaveUISnapshot result;
	result.Mode = mode;
	result.State = state;
	result.bIsBusy = state == ELRSaveUIState::Saving || state == ELRSaveUIState::Loading
		|| state == ELRSaveUIState::Deleting;
	result.Title = ResolveSaveText(contentSet, TEXT("SaveTitle"));
	result.CreateLabel = ResolveSaveText(contentSet, TEXT("SaveCreateNew"));
	int32 manualCount = 0;
	bool bHasAutomaticSlot = false;
	for (const FLRSaveSlotMetadata& metadata : slots)
	{
		FLRSaveSlotView& view = result.Slots.AddDefaulted_GetRef();
		view.SlotId = metadata.SlotId;
		view.DisplayIndex = metadata.DisplayIndex;
		view.SavedAtUtc = metadata.SavedAtUtc;
		view.PlayTimeSeconds = metadata.PlayTimeSeconds;
		view.CollectedCount = metadata.CollectedCount;
		view.TotalCollectibleCount = contentSet ? contentSet->GetTotalCollectibleCount() : 0;
		view.Health = metadata.Health;
		view.bAutomatic = metadata.SlotId.Type == ELRSaveSlotType::Auto;
		bHasAutomaticSlot |= view.bAutomatic;
		view.SlotDisplayText = view.bAutomatic
			? ResolveSaveText(contentSet, TEXT("SaveAuto")) : FText::AsNumber(metadata.DisplayIndex);
		view.HealthDisplayText = ResolveSaveText(contentSet, HealthTextKey(metadata.Health));
		view.bCanLoad = mode == ELRSaveSelectionMode::Load && state == ELRSaveUIState::Idle
			&& metadata.Health == ELRSaveSlotHealth::Healthy;
		view.bCanOverwrite = mode == ELRSaveSelectionMode::Save && state == ELRSaveUIState::Idle
			&& !view.bAutomatic && bManualSaveAllowed;
		view.bCanDelete = state == ELRSaveUIState::Idle && !view.bAutomatic;
		view.MapDisplayName = contentSet ? contentSet->GetMapDisplayName(metadata.MapId) : FText::FromName(metadata.MapId);
		manualCount += view.bAutomatic ? 0 : 1;
	}
	if (!bHasAutomaticSlot)
	{
		// 自动存档栏位固定显示为第一行：目录尚无自动存档时展示为空槽，
		// 不可加载/覆盖/删除（规则层与子系统层另有兜底）。
		FLRSaveSlotView& autoView = result.Slots.AddDefaulted_GetRef();
		autoView.SlotId.Type = ELRSaveSlotType::Auto;
		autoView.SlotId.Guid = LRSaveV2Ids::AutoSlotGuid;
		autoView.bAutomatic = true;
		autoView.bHasData = false;
		autoView.SlotDisplayText = ResolveSaveText(contentSet, TEXT("SaveAuto"));
		autoView.HealthDisplayText = ResolveSaveText(contentSet, HealthTextKey(ELRSaveSlotHealth::MissingPayload));
	}
	result.Slots.Sort([](const FLRSaveSlotView& a, const FLRSaveSlotView& b)
	{
		if (a.bAutomatic != b.bAutomatic)
		{
			return a.bAutomatic;
		}
		return a.DisplayIndex < b.DisplayIndex;
	});
	result.bCanCreateManualSlot = mode == ELRSaveSelectionMode::Save && state == ELRSaveUIState::Idle
		&& bManualSaveAllowed && manualCount < maxManualSlots;
	result.CreateDisplayIndex = manualCount + 1;
	return result;
}

FLRSaveFocusTarget ULRSaveWidgetController::ReconcileFocusTarget(const FLRSaveUISnapshot& snapshot,
	const FLRSaveFocusTarget& requestedTarget)
{
	if (requestedTarget.Kind == ELRSaveFocusTargetKind::ExistingSlot)
	{
		const FLRSaveSlotView* slot = snapshot.Slots.FindByPredicate(
			[&requestedTarget](const FLRSaveSlotView& view) { return view.SlotId == requestedTarget.SlotId; });
		if (slot && (slot->bCanLoad || slot->bCanOverwrite || slot->bCanDelete))
		{
			return requestedTarget;
		}
	}
	else if (requestedTarget.Kind == ELRSaveFocusTargetKind::CreateSlot
		&& snapshot.bCanCreateManualSlot && requestedTarget.CreateDisplayIndex == snapshot.CreateDisplayIndex)
	{
		return requestedTarget;
	}
	else if (requestedTarget.Kind == ELRSaveFocusTargetKind::Root)
	{
		return requestedTarget;
	}
	for (const FLRSaveSlotView& slot : snapshot.Slots)
	{
		if (slot.bCanLoad || slot.bCanOverwrite || slot.bCanDelete)
		{
			return FLRSaveFocusTarget::MakeExisting(slot.SlotId);
		}
	}
	return snapshot.bCanCreateManualSlot
		? FLRSaveFocusTarget::MakeCreate(snapshot.CreateDisplayIndex) : FLRSaveFocusTarget::MakeRoot();
}
