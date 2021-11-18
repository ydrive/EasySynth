// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#include "TextureStyles/TextureStyleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "EditorAssetLibrary.h"
#include "Engine/Selection.h"
#include "HAL/FileManagerGeneric.h"
#include "Kismet/GameplayStatics.h"

#include "PathUtils.h"
#include "TextureStyles/TextureMappingAsset.h"


const FString UTextureStyleManager::SemanticColorParameter(TEXT("SemanticColor"));
const FString UTextureStyleManager::UndefinedSemanticClassName(TEXT("Undefined"));

UTextureStyleManager::UTextureStyleManager() :
	PlainColorMaterial(LoadObject<UMaterial>(nullptr, *FPathUtils::PlainColorMaterialPath())),
	CurrentTextureStyle(ETextureStyle::COLOR),
	bEventsBound(false)
{
	// Check if the plain color material is loaded correctly
	if (PlainColorMaterial == nullptr)
	{
		UE_LOG(LogEasySynth, Error, TEXT("%s: Could not load the PlainColorMaterial"), *FString(__FUNCTION__))
		check(PlainColorMaterial)
	}

	// Prepare the texture mapping asset
	LoadOrCreateTextureMappingAsset();
}

void UTextureStyleManager::BindEvents()
{
	// Bind event to OnLevelActorDeleted event to remove the actor from our buffers
	if (!bEventsBound)
	{
		GEngine->OnLevelActorAdded().AddUObject(this, &UTextureStyleManager::OnLevelActorAdded);
		GEngine->OnLevelActorDeleted().AddUObject(this, &UTextureStyleManager::OnLevelActorDeleted);
		bEventsBound = true;
	}
}

bool UTextureStyleManager::NewSemanticClass(
	const FString& ClassName,
	const FColor& ClassColor,
	const bool bSaveTextureMappingAsset)
{
	// Check collisions with existing classes
	for (auto& Element : TextureMappingAsset->SemanticClasses)
	{
		const FSemanticClass& SemanticClass = Element.Value;
		if (SemanticClass.Name == ClassName || SemanticClass.Color == ClassColor)
		{
			UE_LOG(LogEasySynth, Warning, TEXT("%s: New semantic class (%s, (%d %d %d)) colliding with existing (%s, (%d %d %d))"),
				*FString(__FUNCTION__),
				*ClassName, ClassColor.R, ClassColor.G, ClassColor.B,
				*SemanticClass.Name, SemanticClass.Color.R, SemanticClass.Color.G, SemanticClass.Color.B);
			return false;
		}
	}

	// Crate the new class
	FSemanticClass& NewSemanticClass = TextureMappingAsset->SemanticClasses.Add(ClassName);
	NewSemanticClass.Name = ClassName;
	NewSemanticClass.Color = ClassColor;
	// The semantic class material instance will be created when it's needed

	if (bSaveTextureMappingAsset)
	{
		SaveTextureMappingAsset();
	}

	return true;
}

TArray<FString> UTextureStyleManager::SemanticClassNames() const
{
	TArray<FString> SemanticClassNames;
	for (auto& Element : TextureMappingAsset->SemanticClasses)
	{
		SemanticClassNames.Add(Element.Key);
	}
	return SemanticClassNames;
}

void UTextureStyleManager::ApplySemanticClassToSelectedActors(const FString& ClassName)
{
	if (!TextureMappingAsset->SemanticClasses.Contains(ClassName))
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Received semantic class '%s' not found"),
			*FString(__FUNCTION__), *ClassName);
		return;
	}

	UE_LOG(LogEasySynth, Log, TEXT("%s: Setting the '%s' semantic class to selected actors"),
		*FString(__FUNCTION__), *ClassName)

	TArray<UObject*> SelectedActors;
	GEditor->GetSelectedActors()->GetSelectedObjects(AActor::StaticClass(), SelectedActors);

	for (UObject* SelectedObject : SelectedActors)
	{
		AActor* SelectedActor = Cast<AActor>(SelectedObject);
		if (SelectedActor == nullptr)
		{
			UE_LOG(LogEasySynth, Log, TEXT("%s: Got null actor"), *FString(__FUNCTION__))
			return;
		}

		// Set the class to the actor
		SetSemanticClassToActor(SelectedActor, ClassName);
	}

	SaveTextureMappingAsset();
}

void UTextureStyleManager::CheckoutTextureStyle(ETextureStyle NewTextureStyle)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s: New texture style: %d"), *FString(__FUNCTION__), NewTextureStyle)

	if (NewTextureStyle == CurrentTextureStyle)
	{
		// Return if the desired style is already selected
		UE_LOG(LogEasySynth, Log, TEXT("%s: TextureStyle %d already selected"), *FString(__FUNCTION__), NewTextureStyle)
		return;
	}

	if (CurrentTextureStyle == ETextureStyle::COLOR)
	{
		// Reset the original actor material storage to prepare it for new data
		FOrignalActorDescriptors.Empty();
	}

	// Apply materials to all actors
	TArray<AActor*> LevelActors;
	UGameplayStatics::GetAllActorsOfClass(GEditor->GetEditorWorldContext().World(), AActor::StaticClass(), LevelActors);
	for (AActor* Actor : LevelActors)
	{
		// If actor is unknown, apply the undefined semantic class
		if (!TextureMappingAsset->ActorClassPairs.Contains(Actor->GetActorGuid()))
		{
			SetSemanticClassToActor(Actor, UndefinedSemanticClassName);
		}

		const FString& ClassName = TextureMappingAsset->ActorClassPairs[Actor->GetActorGuid()];

		UE_LOG(LogEasySynth, Log, TEXT("%s: Painting actor '%s'"), *FString(__FUNCTION__), *Actor->GetName())

		if (CurrentTextureStyle == ETextureStyle::COLOR)
		{
			// Changing to semantic view, store the original data
			FOrignalActorDescriptors.Add(Actor);
		}

		// Get actor mesh components
		TArray<UActorComponent*> ActorComponenets;
		const bool bIncludeFromChildActors = true;
		Actor->GetComponents(UStaticMeshComponent::StaticClass(), ActorComponenets, bIncludeFromChildActors);

		// Set new materials
		for (UActorComponent* ActorComponent : ActorComponenets)
		{
			// Apply to each static mesh componenet
			UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(ActorComponent);
			if (MeshComponent == nullptr)
			{
				UE_LOG(LogEasySynth, Error, TEXT("%s: Got null static mesh component"), *FString(__FUNCTION__))
				return;
			}

			if (CurrentTextureStyle == ETextureStyle::COLOR)
			{
				// Changing to semantic view, store the original data
				FOrignalActorDescriptors[Actor].Add(MeshComponent);
			}

			UE_LOG(LogEasySynth, Log, TEXT("%s: Painting mesh component '%s'"),
				*FString(__FUNCTION__), *MeshComponent->GetName());

			for (int i = 0; i < MeshComponent->GetNumMaterials(); i++)
			{
				// Apply to each material
				if (CurrentTextureStyle == ETextureStyle::COLOR)
				{
					// Change to semantic material
					FOrignalActorDescriptors[Actor][MeshComponent].Add(MeshComponent->GetMaterial(i));
					MeshComponent->SetMaterial(
						i, GetSemanticClassMaterial(TextureMappingAsset->SemanticClasses[ClassName]));
				}
				else
				{
					// Revert to original material
					MeshComponent->SetMaterial(i, FOrignalActorDescriptors[Actor][MeshComponent][i]);
				}
			}
		}
	}

	if (CurrentTextureStyle == ETextureStyle::SEMANTIC)
	{
		// Reset the original actor material storage as it is no longer needed
		FOrignalActorDescriptors.Empty();
	}

	CurrentTextureStyle = NewTextureStyle;
}

void UTextureStyleManager::LoadOrCreateTextureMappingAsset()
{
	// Try to load
	TextureMappingAsset = LoadObject<UTextureMappingAsset>(nullptr, *FPathUtils::TextureMappingAssetPath());

	if (TextureMappingAsset == nullptr)
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Texture mapping asset not found, creating a new one"),
			*FString(__FUNCTION__));

		// Remove all potentially existing EasySynth assets from the project
		const bool bIsAbsolute = true;
		const bool bRequireExists = false;
		const bool bTree = true;
		FFileManagerGeneric::Get().DeleteDirectory(
			*FPathUtils::ProjectPluginContentDir(bIsAbsolute), bRequireExists, bTree);

		// Register the plugin directroy with the editor
		FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().AddPath(FPathUtils::ProjectPluginContentDir());
		// TODO: Add all other subpaths here

		// Create and populate the asset
		UPackage *TextureMappingPackage = CreatePackage(*FPathUtils::TextureMappingAssetPath());
		check(TextureMappingPackage)
		TextureMappingAsset = NewObject<UTextureMappingAsset>(
			TextureMappingPackage,
			UTextureMappingAsset::StaticClass(),
			*FPathUtils::TextureMappingAssetName,
			EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
		check(TextureMappingAsset)

		// Initialize the default undefined semantic class with pure white color
		NewSemanticClass(UndefinedSemanticClassName, FColor(255, 255, 255, 255), false);

		// Don't save the asset yet to prevent crashing the editor on startup
	}
}

void UTextureStyleManager::SaveTextureMappingAsset()
{
	check(TextureMappingAsset)
	const bool bOnlyIfIsDirty = false;
	UEditorAssetLibrary::SaveLoadedAsset(TextureMappingAsset, bOnlyIfIsDirty);
}

void UTextureStyleManager::OnLevelActorAdded(AActor* Actor)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s: Adding actor '%s'"), *FString(__FUNCTION__), *Actor->GetName())

	if (CurrentTextureStyle == ETextureStyle::SEMANTIC)
	{
		// TODO: Assign a default semantic class and display it
	}
}

void UTextureStyleManager::OnLevelActorDeleted(AActor* Actor)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s: Removing actor '%s'"), *FString(__FUNCTION__), *Actor->GetName())
	TextureMappingAsset->ActorClassPairs.Remove(Actor->GetActorGuid());
	FOrignalActorDescriptors.Remove(Actor);
}

void UTextureStyleManager::SetSemanticClassToActor(AActor* Actor, const FString& ClassName)
{
	// Remove class if already assigned
	TextureMappingAsset->ActorClassPairs.Remove(Actor->GetActorGuid());

	// Set the new class
	TextureMappingAsset->ActorClassPairs.Add(Actor->GetActorGuid(), ClassName);

	// Immediately display the change when in the semantic mode
	if (CurrentTextureStyle == ETextureStyle::SEMANTIC)
	{
		// Simplified checkout of the semantic view for this actor

		// In case this actor has not already been added to the original descriptor,
		// add all its contents before chinging the material
		const bool bAddingNew = !FOrignalActorDescriptors.Contains(Actor);
		if (bAddingNew)
		{
			FOrignalActorDescriptors.Add(Actor);
		}

		// Get actor mesh components
		TArray<UActorComponent*> ActorComponenets;
		const bool bIncludeFromChildActors = true;
		Actor->GetComponents(UStaticMeshComponent::StaticClass(), ActorComponenets, bIncludeFromChildActors);

		// Set new materials
		for (UActorComponent* ActorComponent : ActorComponenets)
		{
			// Apply to each static mesh componenet
			UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(ActorComponent);
			if (MeshComponent == nullptr)
			{
				UE_LOG(LogEasySynth, Error, TEXT("%s: Got null static mesh component"), *FString(__FUNCTION__))
				return;
			}

			if (bAddingNew)
			{
				FOrignalActorDescriptors[Actor].Add(MeshComponent);
			}

			for (int i = 0; i < MeshComponent->GetNumMaterials(); i++)
			{
				// Apply to each material
				if (bAddingNew)
				{
					FOrignalActorDescriptors[Actor][MeshComponent].Add(MeshComponent->GetMaterial(i));
				}

				MeshComponent->SetMaterial(
					i, GetSemanticClassMaterial(TextureMappingAsset->SemanticClasses[ClassName]));
			}
		}
	}
}

UMaterialInstanceDynamic* UTextureStyleManager::GetSemanticClassMaterial(FSemanticClass& SemanticClass)
{
	// If the plain color material is null, create it
	if (SemanticClass.PlainColorMaterialInstance == nullptr)
	{
		SemanticClass.PlainColorMaterialInstance = UMaterialInstanceDynamic::Create(PlainColorMaterial, nullptr);
		if (SemanticClass.PlainColorMaterialInstance == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("%s: Could not create the plain color material instance"),
				*FString(__FUNCTION__))
			check(SemanticClass.PlainColorMaterialInstance)
		}
		SemanticClass.PlainColorMaterialInstance->SetVectorParameterValue(*SemanticColorParameter, SemanticClass.Color);
	}

	return SemanticClass.PlainColorMaterialInstance;
}