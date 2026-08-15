/** @file LRMainMenuUITypes.h @brief 主菜单只读 ViewModel。 */
#pragma once

#include "CoreMinimal.h"
#include "Save/LRSaveV2Types.h"

#include "LRMainMenuUITypes.generated.h"

USTRUCT(BlueprintType, meta = (DisplayName = "Lost Runic Main Menu View Model"))
struct LOSTRUNIC_API FLRMainMenuViewModel
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Main Menu") bool bCanNewGame = false;
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu") bool bCanContinue = false;
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu") bool bCanLoad = false;
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu") bool bCanOptions = false;
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu") bool bCanExit = true;
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu") bool bIsBusy = false;
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu") ELRSaveCatalogState CatalogState = ELRSaveCatalogState::Initializing;
	UPROPERTY(BlueprintReadOnly, Category = "Main Menu") FText StatusMessage;
};
