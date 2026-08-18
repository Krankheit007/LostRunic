// Copyright LostRunic. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "LRDialogueLocalizationSettings.generated.h"

/** Editor-only overrides for the reproducible project-owned localization Python environment. */
UCLASS(config=EditorPerProjectUserSettings, defaultconfig, meta=(DisplayName="LostRunic Dialogue Localization"))
class LOSTRUNICEDITOR_API ULRDialogueLocalizationSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const ULRDialogueLocalizationSettings* Get();

	UPROPERTY(EditAnywhere, Config, Category="Python", meta=(FilePathFilter="exe"))
	FFilePath PythonExecutableOverride;
};
