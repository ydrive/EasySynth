// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "TextureStyles/TextureBackupManager.h"

#include "LandscapeProxy.h"


void UTextureBackupManager::AddAndPaint(
	AActor* Actor,
	const bool bDoAdd,
	const bool bDoPaint,
	UMaterialInstanceConstant* Material)
{
	ALandscapeProxy* LandscapeProxy = Cast<ALandscapeProxy>(Actor);
	if (LandscapeProxy != nullptr)
	{
		AddLandscapeActor(LandscapeProxy, bDoAdd, bDoPaint, Material);
		return;
	}

	AddDefaultActor(Actor, bDoAdd, bDoPaint, Material);
}

bool UTextureBackupManager::ContainsActor(AActor* Actor)
{
	ALandscapeProxy* LandscapeProxy = Cast<ALandscapeProxy>(Actor);
	if (LandscapeProxy != nullptr)
	{
		return LandscapeActorDescriptors.Contains(LandscapeProxy);
	}

	return OriginalActorDescriptors.Contains(Actor);
}

void UTextureBackupManager::RemoveActor(AActor* Actor)
{
	ALandscapeProxy* LandscapeProxy = Cast<ALandscapeProxy>(Actor);
	if (LandscapeProxy != nullptr)
	{
		LandscapeActorDescriptors.Remove(LandscapeProxy);
		return;
	}

	OriginalActorDescriptors.Remove(Actor);
}

void UTextureBackupManager::AddLandscapeActor(
	ALandscapeProxy* LandscapeProxy,
	const bool bDoAdd,
	const bool bDoPaint,
	UMaterialInstanceConstant* Material)
{
	const bool bDoRestore = (Material == nullptr);

	if (bDoRestore)
	{
		// Revert to original material
		if (bDoPaint)
		{
			LandscapeProxy->LandscapeMaterial = Cast<UMaterialInstance>(LandscapeActorDescriptors[LandscapeProxy]);
			if (LandscapeProxy->LandscapeMaterial != nullptr)
			{
				FPropertyChangedEvent PropertyChangedEvent(FindFieldChecked<FProperty>(LandscapeProxy->GetClass(), FName("LandscapeMaterial")));
				LandscapeProxy->PostEditChangeProperty(PropertyChangedEvent);
				LandscapeActorDescriptors.Remove(LandscapeProxy);
			}
			else
			{
				UE_LOG(LogEasySynth, Error, TEXT("%s: Failed cast to UMaterialInstance"), *FString(__FUNCTION__))
			}
		}
	}
	else
	{
		// Change to semantic material
		if (bDoAdd)
		{
			UMaterialInstanceConstant* MaterialInstanceConstant =
				Cast<UMaterialInstanceConstant>(LandscapeProxy->GetLandscapeMaterial());
			if (MaterialInstanceConstant != nullptr)
			{
				LandscapeActorDescriptors.Add(LandscapeProxy, MaterialInstanceConstant);
			}
			else
			{
				UE_LOG(LogEasySynth, Error, TEXT("%s: Failed cast to UMaterialInstanceConstant"), *FString(__FUNCTION__))
			}
		}
		if (bDoPaint)
		{
			UMaterialInterface* MaterialInterface = Cast<UMaterialInterface>(Material);
			if (MaterialInterface != nullptr)
			{
				LandscapeProxy->LandscapeMaterial = MaterialInterface;
				FPropertyChangedEvent PropertyChangedEvent(FindFieldChecked<FProperty>(LandscapeProxy->GetClass(), FName("LandscapeMaterial")));
				LandscapeProxy->PostEditChangeProperty(PropertyChangedEvent);
			}
			else
			{
				UE_LOG(LogEasySynth, Error, TEXT("%s: Failed cast to UMaterialInterface"), *FString(__FUNCTION__))
			}
		}
	}
}

void UTextureBackupManager::AddDefaultActor(
	AActor* Actor,
	const bool bDoAdd,
	const bool bDoPaint,
	UMaterialInstanceConstant* Material)
{
	const bool bDoRestore = (Material == nullptr);

	if (bDoAdd)
	{
		OriginalActorDescriptors.Add(Actor);
	}
	else if (!OriginalActorDescriptors.Contains(Actor))
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: Actor expected but not found in OriginalActorDescriptors"),
			*FString(__FUNCTION__))
		return;
	}

	// Get actor mesh components
	TArray<UActorComponent*> ActorComponents;
	const bool bIncludeFromChildActors = true;
	Actor->GetComponents(UPrimitiveComponent::StaticClass(), ActorComponents, bIncludeFromChildActors);

	// If no mesh components are found, ignore the actor
	if (ActorComponents.Num() == 0)
	{
		return;
	}

	// Set new materials
	for (UActorComponent* ActorComponent : ActorComponents)
	{
		// Apply to each static mesh component
		UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(ActorComponent);
		if (PrimitiveComponent == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Got null static mesh component"), *FString(__FUNCTION__))
			return;
		}

		if (bDoAdd)
		{
			OriginalActorDescriptors[Actor].Add(PrimitiveComponent);
		}
		else if (!OriginalActorDescriptors[Actor].Contains(PrimitiveComponent))
		{
			UE_LOG(LogEasySynth, Warning, TEXT("%s: PrimitiveComponent expected but not found in OriginalActorDescriptors"),
				*FString(__FUNCTION__))
			return;
		}

		// Check whether number of stored materials is correct, if they are needed
		if (!bDoAdd && bDoRestore &&
			OriginalActorDescriptors[Actor][PrimitiveComponent].Num() != PrimitiveComponent->GetNumMaterials())
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: %d instead of %d actor's mesh component materials found"),
				*FString(__FUNCTION__),
				OriginalActorDescriptors[Actor][PrimitiveComponent].Num(),
				PrimitiveComponent->GetNumMaterials())
			return;
		}

		// Store all mesh component materials
		for (int i = 0; i < PrimitiveComponent->GetNumMaterials(); i++)
		{
			if (bDoRestore)
			{
				// Revert to original material
				if (bDoPaint)
				{
					PrimitiveComponent->SetMaterial(i, OriginalActorDescriptors[Actor][PrimitiveComponent][i]);
				}
			}
			else
			{
				// Change to semantic material
				if (bDoAdd)
				{
					OriginalActorDescriptors[Actor][PrimitiveComponent].Add(PrimitiveComponent->GetMaterial(i));
				}
				if (bDoPaint)
				{
					UMaterialInterface* MaterialInterface = Cast<UMaterialInterface>(Material);
					if (MaterialInterface != nullptr)
					{
						PrimitiveComponent->SetMaterial(i, MaterialInterface);
					}
					else
					{
						UE_LOG(LogEasySynth, Error, TEXT("%s: Failed cast to UMaterialInterface"), *FString(__FUNCTION__))
					}
				}
			}
		}
	}

	if (bDoRestore)
	{
		OriginalActorDescriptors.Remove(Actor);
	}
}
