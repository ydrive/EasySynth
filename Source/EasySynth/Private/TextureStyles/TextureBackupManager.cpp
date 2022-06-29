// Copyright (c) 2022 YDrive Inc. All rights reserved.

#include "TextureStyles/TextureBackupManager.h"

#include "LandscapeProxy.h"


void UTextureBackupManager::AddAndPaint(
	AActor* Actor,
	const bool bDoAdd,
	const bool bDoPaint,
    UMaterialInstanceDynamic* Material)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s"), *FString(__FUNCTION__))

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
    UMaterialInstanceDynamic* Material)
{
	const bool bDoRestore = (Material == nullptr);

	if (bDoAdd)
	{
		LandscapeActorDescriptors.Add(LandscapeProxy);
	}
}

void UTextureBackupManager::AddDefaultActor(
	AActor* Actor,
	const bool bDoAdd,
	const bool bDoPaint,
    UMaterialInstanceDynamic* Material)
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
					MeshComponent->SetMaterial(i, Material);
				}
			}
		}
	}

	if (bDoRestore)
	{
		OriginalActorDescriptors.Remove(Actor);
	}
}
