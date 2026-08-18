// Copyright LostRunic. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

/** Small platform-independent SHA-256 implementation for editor exchange identities. */
class FLRDialogueLocalizationHash
{
public:
	static FString Sha256(const uint8* Data, int32 NumBytes);
	static FString Sha256String(const FString& Value);
};
