// Copyright LostRunic. All Rights Reserved.
#include "Narrative/LRDialogueSpeakerRegistry.h"

#include "Internationalization/Text.h"

const FLRDialogueSpeakerDefinition* ULRDialogueSpeakerRegistry::Find(const FName SpeakerId) const
{
	return Speakers.FindByPredicate([SpeakerId](const FLRDialogueSpeakerDefinition& Definition)
	{
		return Definition.SpeakerId == SpeakerId;
	});
}

bool ULRDialogueSpeakerRegistry::Validate(FString& OutError) const
{
	TSet<FName> SeenIds;
	for (const FLRDialogueSpeakerDefinition& Speaker : Speakers)
	{
		if (Speaker.SpeakerId.IsNone() || SeenIds.Contains(Speaker.SpeakerId))
		{
			OutError = TEXT("Speaker registry contains an empty or duplicate SpeakerId.");
			return false;
		}
		FName TableId;
		FString TableKey;
		if (!FTextInspector::GetTableIdAndKey(Speaker.DisplayName, TableId, TableKey)
			|| TableId.IsNone() || TableKey.IsEmpty())
		{
			OutError = FString::Printf(TEXT("SpeakerId=%s DisplayName must reference ST_DialogueSpeakers."), *Speaker.SpeakerId.ToString());
			return false;
		}
		SeenIds.Add(Speaker.SpeakerId);
	}
	return true;
}
