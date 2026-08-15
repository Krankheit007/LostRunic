/** @file LRSaveCatalogStore.h @brief Catalog A/B persistence and interrupted transaction recovery. */
#pragma once

#include "CoreMinimal.h"
#include "Save/LRSaveV2Types.h"

class ULRSaveCatalog;
class ULRSavePayload;

struct LOSTRUNIC_API FLRCatalogRecoveryResult
{
	bool bCatalogAvailable = false;
	bool bCatalogWasReset = false;
	bool bCatalogChanged = false;
	FString Diagnostic;
};

class LOSTRUNIC_API FLRSaveCatalogStore
{
public:
	static ULRSaveCatalog* LoadBestCatalog(UObject* outer, FLRCatalogRecoveryResult& outResult);
	static bool CommitCatalog(ULRSaveCatalog& catalog, FString& outError);
	static bool RecoverPendingOperation(ULRSaveCatalog& catalog, FString& outError);
	static ULRSavePayload* LoadAndValidatePayload(UObject* outer, const FLRSaveSlotMetadata& metadata,
		ELRSaveSlotHealth& outHealth, ELRSaveResultCode& outCode, FString& outError);
	static FString MakePayloadKey(const FLRSaveSlotId& slotId, int64 saveSequence);
	static bool IsDeterministicHealth(ELRSaveSlotHealth health);

private:
	static ULRSaveCatalog* TryLoadCatalog(UObject* outer, const FString& slotName, FString& outError);
	static bool SaveCatalogAndVerify(ULRSaveCatalog& catalog, const FString& slotName, FString& outError);
	static bool DoesPayloadExist(const FLRSaveSlotMetadata& metadata);
};
