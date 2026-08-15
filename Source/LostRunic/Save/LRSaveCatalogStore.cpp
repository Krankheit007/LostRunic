#include "Save/LRSaveCatalogStore.h"

#include "Core/LRLog.h"
#include "Kismet/GameplayStatics.h"
#include "Save/LRSaveCatalog.h"
#include "Save/LRSavePayload.h"

namespace
{
	constexpr int32 SaveUserIndex = 0;

	void CommitPendingWriteInMemory(ULRSaveCatalog& catalog)
	{
		const FLRSaveSlotMetadata target = catalog.PendingOperation.TargetMetadata;
		if (FLRSaveSlotMetadata* existing = catalog.FindSlot(target.SlotId))
		{
			*existing = target;
		}
		else
		{
			catalog.Slots.Add(target);
		}
		catalog.PendingOperation = FLRCatalogPendingOperation();
		catalog.SortSlots();
	}

	void CommitPendingDeleteInMemory(ULRSaveCatalog& catalog)
	{
		const FLRSaveSlotId targetId = catalog.PendingOperation.TargetMetadata.SlotId;
		catalog.Slots.RemoveAll([&targetId](const FLRSaveSlotMetadata& slot) { return slot.SlotId == targetId; });
		catalog.PendingOperation = FLRCatalogPendingOperation();
	}
}

ULRSaveCatalog* FLRSaveCatalogStore::LoadBestCatalog(UObject* outer, FLRCatalogRecoveryResult& outResult)
{
	FString errorA;
	FString errorB;
	ULRSaveCatalog* catalogA = TryLoadCatalog(outer, LRSaveCatalogNames::A(), errorA);
	ULRSaveCatalog* catalogB = TryLoadCatalog(outer, LRSaveCatalogNames::B(), errorB);
	ULRSaveCatalog* catalog = catalogA && catalogB
		? (catalogA->Generation >= catalogB->Generation ? catalogA : catalogB)
		: (catalogA ? catalogA : catalogB);
	if (!catalog)
	{
		catalog = NewObject<ULRSaveCatalog>(outer);
		outResult.bCatalogAvailable = true;
		outResult.bCatalogWasReset = true;
		outResult.Diagnostic = FString::Printf(TEXT("No valid V2 catalog. A='%s' B='%s'. Starting empty."),
			*errorA, *errorB);
		return catalog;
	}

	outResult.bCatalogAvailable = true;
	if (catalog->PendingOperation.IsSet())
	{
		FString recoveryError;
		outResult.bCatalogChanged = RecoverPendingOperation(*catalog, recoveryError);
		outResult.Diagnostic = recoveryError;
	}
	return catalog;
}

bool FLRSaveCatalogStore::CommitCatalog(ULRSaveCatalog& catalog, FString& outError)
{
	FString validationError;
	if (!catalog.Validate(validationError))
	{
		outError = FString::Printf(TEXT("Catalog commit rejected: %s"), *validationError);
		return false;
	}
	++catalog.Generation;
	const FString& target = (catalog.Generation % 2) == 0 ? LRSaveCatalogNames::A() : LRSaveCatalogNames::B();
	return SaveCatalogAndVerify(catalog, target, outError);
}

bool FLRSaveCatalogStore::RecoverPendingOperation(ULRSaveCatalog& catalog, FString& outError)
{
	if (!catalog.PendingOperation.IsSet())
	{
		return false;
	}
	if (catalog.PendingOperation.Type == ELRCatalogPendingType::Write)
	{
		ELRSaveSlotHealth health;
		ELRSaveResultCode code;
		FString payloadError;
		ULRSavePayload* payload = LoadAndValidatePayload(&catalog, catalog.PendingOperation.TargetMetadata,
			health, code, payloadError);
		if (payload)
		{
			CommitPendingWriteInMemory(catalog);
			if (!CommitCatalog(catalog, outError))
			{
				return false;
			}
			outError = TEXT("Recovered interrupted catalog write by committing the validated payload.");
			return true;
		}

		const FString invalidPayload = catalog.PendingOperation.TargetMetadata.PayloadKey;
		catalog.PendingOperation = FLRCatalogPendingOperation();
		if (!CommitCatalog(catalog, outError))
		{
			return false;
		}
		if (UGameplayStatics::DoesSaveGameExist(invalidPayload, SaveUserIndex))
		{
			UGameplayStatics::DeleteGameInSlot(invalidPayload, SaveUserIndex);
		}
		outError = FString::Printf(TEXT("Rolled back interrupted catalog write: %s"), *payloadError);
		return true;
	}

	if (catalog.PendingOperation.Type == ELRCatalogPendingType::Delete)
	{
		const FString payloadKey = catalog.PendingOperation.TargetMetadata.PayloadKey;
		if (UGameplayStatics::DoesSaveGameExist(payloadKey, SaveUserIndex)
			&& !UGameplayStatics::DeleteGameInSlot(payloadKey, SaveUserIndex))
		{
			outError = FString::Printf(TEXT("Pending delete could not remove payload '%s'."), *payloadKey);
			return false;
		}
		CommitPendingDeleteInMemory(catalog);
		if (!CommitCatalog(catalog, outError))
		{
			return false;
		}
		outError = TEXT("Recovered interrupted catalog delete.");
		return true;
	}

	outError = TEXT("Catalog contains an unknown pending operation.");
	return false;
}

ULRSavePayload* FLRSaveCatalogStore::LoadAndValidatePayload(UObject* outer, const FLRSaveSlotMetadata& metadata,
	ELRSaveSlotHealth& outHealth, ELRSaveResultCode& outCode, FString& outError)
{
	outHealth = ELRSaveSlotHealth::Healthy;
	outCode = ELRSaveResultCode::ReadFailed;
	if (!DoesPayloadExist(metadata))
	{
		outHealth = ELRSaveSlotHealth::MissingPayload;
		outCode = ELRSaveResultCode::MissingPayload;
		outError = FString::Printf(TEXT("Payload '%s' does not exist."), *metadata.PayloadKey);
		return nullptr;
	}
	USaveGame* raw = UGameplayStatics::LoadGameFromSlot(metadata.PayloadKey, SaveUserIndex);
	ULRSavePayload* payload = Cast<ULRSavePayload>(raw);
	if (!payload)
	{
		// LoadGameFromSlot can fail for a temporary platform I/O error. Do not persist CorruptPayload here.
		outCode = ELRSaveResultCode::ReadFailed;
		outError = FString::Printf(TEXT("Payload '%s' could not be read."), *metadata.PayloadKey);
		return nullptr;
	}
	if (!payload->ValidatePayload(&metadata, outHealth, outError))
	{
		outCode = outHealth == ELRSaveSlotHealth::UnsupportedVersion ? ELRSaveResultCode::UnsupportedVersion
			: (outHealth == ELRSaveSlotHealth::CatalogMismatch ? ELRSaveResultCode::CatalogMismatch
				: ELRSaveResultCode::InvalidData);
		return nullptr;
	}
	return payload;
}

FString FLRSaveCatalogStore::MakePayloadKey(const FLRSaveSlotId& slotId, const int64 saveSequence)
{
	return FString::Printf(TEXT("LostRunic_V2_Payload_%s_%lld"),
		*slotId.Guid.ToString(EGuidFormats::Digits), saveSequence);
}

bool FLRSaveCatalogStore::IsDeterministicHealth(const ELRSaveSlotHealth health)
{
	return health != ELRSaveSlotHealth::Healthy && health != ELRSaveSlotHealth::CorruptPayload;
}

ULRSaveCatalog* FLRSaveCatalogStore::TryLoadCatalog(UObject* outer, const FString& slotName, FString& outError)
{
	if (!UGameplayStatics::DoesSaveGameExist(slotName, SaveUserIndex))
	{
		outError = TEXT("missing");
		return nullptr;
	}
	ULRSaveCatalog* catalog = Cast<ULRSaveCatalog>(UGameplayStatics::LoadGameFromSlot(slotName, SaveUserIndex));
	if (!catalog || !catalog->Validate(outError))
	{
		if (outError.IsEmpty())
		{
			outError = TEXT("unreadable");
		}
		return nullptr;
	}
	if (catalog->GetOuter() != outer)
	{
		catalog->Rename(nullptr, outer);
	}
	return catalog;
}

bool FLRSaveCatalogStore::SaveCatalogAndVerify(ULRSaveCatalog& catalog, const FString& slotName, FString& outError)
{
	if (!UGameplayStatics::SaveGameToSlot(&catalog, slotName, SaveUserIndex))
	{
		outError = FString::Printf(TEXT("Failed to write catalog '%s'."), *slotName);
		return false;
	}
	ULRSaveCatalog* verification = Cast<ULRSaveCatalog>(UGameplayStatics::LoadGameFromSlot(slotName, SaveUserIndex));
	FString validationError;
	if (!verification || verification->Generation != catalog.Generation || !verification->Validate(validationError))
	{
		outError = FString::Printf(TEXT("Catalog '%s' failed read-back verification: %s"), *slotName, *validationError);
		return false;
	}
	return true;
}

bool FLRSaveCatalogStore::DoesPayloadExist(const FLRSaveSlotMetadata& metadata)
{
	return !metadata.PayloadKey.IsEmpty() && UGameplayStatics::DoesSaveGameExist(metadata.PayloadKey, SaveUserIndex);
}
