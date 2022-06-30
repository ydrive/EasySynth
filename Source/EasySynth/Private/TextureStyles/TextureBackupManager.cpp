// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "TextureStyles/TextureBackupManager.h"

#include "LandscapeProxy.h"


void UTextureBackupManager::AddAndPaint(
	AActor* Actor,
	const bool bDoAdd,
	const bool bDoPaint,
    UMaterialInstanceConstant* Material)
{
	if (!IsValid(Actor))
	{
		return;
	}

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

	// Get actor mesh components
	TArray<UActorComponent*> ActorComponents;
	const bool bIncludeFromChildActors = true;
	Actor->GetComponents(UStaticMeshComponent::StaticClass(), ActorComponents, bIncludeFromChildActors);

	// If no mesh components are found, ignore the actor
	if (ActorComponents.Num() == 0)
	{
		return;
	}

	// Set new materials
	for (UActorComponent* ActorComponent : ActorComponents)
	{
		// Apply to each static mesh component
		UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(ActorComponent);
		if (MeshComponent == nullptr)
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: Got null static mesh component"), *FString(__FUNCTION__))
			return;
		}

		if (bDoAdd)
		{
			OriginalActorDescriptors[Actor].Add(MeshComponent);
		}

		// Check whether number of stored materials is correct, if they are needed
		if (!bDoAdd && bDoRestore &&
			OriginalActorDescriptors[Actor][MeshComponent].Num() != MeshComponent->GetNumMaterials())
		{
			UE_LOG(LogEasySynth, Error, TEXT("%s: %d instead of %d actor's mesh component materials found"),
				*FString(__FUNCTION__),
				OriginalActorDescriptors[Actor][MeshComponent].Num(),
				MeshComponent->GetNumMaterials())
			return;
		}

		// Store all mesh component materials
		for (int i = 0; i < MeshComponent->GetNumMaterials(); i++)
		{
			if (bDoRestore)
			{
				// Revert to original material
				if (bDoPaint)
				{
					MeshComponent->SetMaterial(i, OriginalActorDescriptors[Actor][MeshComponent][i]);
				}
			}
			else
			{
				// Change to semantic material
				if (bDoAdd)
				{
					OriginalActorDescriptors[Actor][MeshComponent].Add(MeshComponent->GetMaterial(i));
				}
				if (bDoPaint)
				{
					UMaterialInterface* MaterialInterface = Cast<UMaterialInterface>(Material);
					if (MaterialInterface != nullptr)
					{
						MeshComponent->SetMaterial(i, MaterialInterface);
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
