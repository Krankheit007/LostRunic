/** @file LRSaveProvider.h @brief World-resolving providers for V2 save chunks. */
#pragma once

#include "CoreMinimal.h"
#include "Save/LRSaveV2Types.h"

class UGameInstance;

class ILRSaveProvider
{
public:
	virtual ~ILRSaveProvider() = default;
	virtual FName GetProviderId() const = 0;
	virtual bool IsRequired() const { return true; }
	virtual bool Capture(UGameInstance& gameInstance, FLRSaveDataV2& data, FString& outError) const = 0;
	virtual bool Restore(UGameInstance& gameInstance, const FLRSaveDataV2& data, FString& outError) const = 0;
	virtual bool ResetForNewGame(UGameInstance& gameInstance, FString& outError) const { return true; }
};

namespace LRSaveProviders
{
	LOSTRUNIC_API void CreateRequired(TArray<TUniquePtr<ILRSaveProvider>>& outProviders);
	LOSTRUNIC_API bool CaptureAll(const TArray<TUniquePtr<ILRSaveProvider>>& providers,
		UGameInstance& gameInstance, FLRSaveDataV2& data, FString& outError);
	LOSTRUNIC_API bool RestoreNonPlayer(const TArray<TUniquePtr<ILRSaveProvider>>& providers,
		UGameInstance& gameInstance, const FLRSaveDataV2& data, FString& outError);
	LOSTRUNIC_API bool RestorePlayer(const TArray<TUniquePtr<ILRSaveProvider>>& providers,
		UGameInstance& gameInstance, const FLRSaveDataV2& data, FString& outError);
	LOSTRUNIC_API bool ResetForNewGame(const TArray<TUniquePtr<ILRSaveProvider>>& providers,
		UGameInstance& gameInstance, FString& outError);
}
