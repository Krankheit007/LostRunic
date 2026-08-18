// Copyright LostRunic. All Rights Reserved.
#pragma once

#include "Commandlets/Commandlet.h"

#include "LRDialogueLocalizationCommandlet.generated.h"

/** Commandlet facade for the staged SUDS -> PO -> XLSX localization workflow. */
UCLASS()
class ULRDialogueLocalizationCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	ULRDialogueLocalizationCommandlet();
	virtual int32 Main(const FString& Params) override;
};
