#include "Save/LRSavePayload.h"

bool ULRSavePayload::ValidatePayload(const FLRSaveSlotMetadata* expectedMetadata, ELRSaveSlotHealth& outHealth,
	FString& outError) const
{
	outHealth = ELRSaveSlotHealth::Healthy;
	if (SaveVersion != LatestVersion)
	{
		outHealth = ELRSaveSlotHealth::UnsupportedVersion;
		outError = FString::Printf(TEXT("Unsupported payload version %d; expected %d."), SaveVersion, LatestVersion);
		return false;
	}
	if (!SlotId.IsValid() || PayloadKey.IsEmpty() || SaveSequence <= 0 || !Data.Player.ResumeAnchor.IsValid()
		|| Data.Player.CurrentMapId.IsNone() || Data.Statistics.DeathCount < 0 || Data.Statistics.PlayTimeSeconds < 0.0)
	{
		outHealth = ELRSaveSlotHealth::InvalidData;
		outError = TEXT("Payload contains invalid required V2 data.");
		return false;
	}
	if (expectedMetadata && (SlotId != expectedMetadata->SlotId || PayloadKey != expectedMetadata->PayloadKey
		|| SaveSequence != expectedMetadata->SaveSequence))
	{
		outHealth = ELRSaveSlotHealth::CatalogMismatch;
		outError = TEXT("Payload identity does not match its catalog entry.");
		return false;
	}
	return true;
}
