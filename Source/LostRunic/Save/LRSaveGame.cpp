#include "Save/LRSaveGame.h"

bool ULRSaveGame::MigrateToLatest(FString& outError)
{
	if (SaveVersion < 0 || SaveVersion > LatestVersion)
	{
		outError = FString::Printf(TEXT("Unsupported save version %d."), SaveVersion);
		return false;
	}
	if (SaveVersion == 0 && !ResumeAnchor.IsValid() && !LegacyMapId.IsNone())
	{
		ResumeAnchor.MapId = LegacyMapId;
		ResumeAnchor.AnchorId = TEXT("Migrated");
		ResumeAnchor.Location = LegacyLocation;
		ResumeAnchor.Rotation = LegacyRotation;
	}
	SaveVersion = LatestVersion;
	return true;
}
