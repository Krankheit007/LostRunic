#include "Camera/LRCutawayTargetComponent.h"

#include "LostRunic.h"
#include "Data/LRPresentationTuning.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"

void ULRCutawayTargetComponent::ValidateMaterials() const
{
	if (!Tuning) return;
	for (UPrimitiveComponent* primitive : AffectedPrimitives)
	{
		const UMeshComponent* mesh = Cast<UMeshComponent>(primitive);
		if (!mesh) continue;
		const TArray<FName> slotNames = mesh->GetMaterialSlotNames();
		for (int32 slotIndex = 0; slotIndex < mesh->GetNumMaterials(); ++slotIndex)
		{
			const FName slotName = slotNames.IsValidIndex(slotIndex) ? slotNames[slotIndex] : NAME_None;
			if (PersistentMaterialSlots.Contains(slotName)) continue;
			UMaterialInterface* material = mesh->GetMaterial(slotIndex);
			if (!material || material->GetBlendMode() != BLEND_Masked)
			{
				UE_LOG(LogLostRunicCutaway, Warning, TEXT("%s slot %s must resolve to Masked for cutaway"),
					*GetNameSafe(primitive), *slotName.ToString());
				continue;
			}
			const UMaterial* baseMaterial = material->GetBaseMaterial();
			bool bApproved = false;
			for (const TSoftObjectPtr<UMaterialInterface>& approvedReference : Tuning->ApprovedCutawayMasterMaterials)
			{
				UMaterialInterface* approved = approvedReference.LoadSynchronous();
				bApproved |= approved && approved->GetBaseMaterial() == baseMaterial;
			}
			if (!bApproved)
			{
				UE_LOG(LogLostRunicCutaway, Warning, TEXT("%s slot %s uses unapproved cutaway master %s"),
					*GetNameSafe(primitive), *slotName.ToString(), *GetNameSafe(baseMaterial));
			}
		}
	}
}
