// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#include "TextureStyles/TextureStyleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "Engine/Selection.h"

#include "PathUtils.h"
#include "TextureStyles/TextureMappingAsset.h"


UTextureStyleManager::UTextureStyleManager() :
	CurrentTextureStyle(ETextureStyle::COLOR)
{
	LoadOrCreateTextureMappingAsset();
}

// TODO: On begin play bind event to OnLevelActorDeleted event to remove the actor from our buffers

bool UTextureStyleManager::NewSemanticClass(const FString& ClassName, const FColor& ClassColor)
{
	// TODO: Check collision with existing classes

	// Crate a new class
	FSemanticClass& SemanticClass = TextureMappingAsset->SemanticClasses.Add(ClassName);
	SemanticClass.Name = ClassName;
	SemanticClass.Color = ClassColor;
	// TODO: Create a material instance for the new class

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

void UTextureStyleManager::ApplySemanticClass(const FString& ClassName)
{
	if (!TextureMappingAsset->SemanticClasses.Contains(ClassName))
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Received semantic class '%s' not found"),
			*FString(__FUNCTION__), *ClassName);
		return;
	}

	UE_LOG(LogEasySynth, Log, TEXT("%s: Setting the '%s' semantic class"), *FString(__FUNCTION__), *ClassName)

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
}

void UTextureStyleManager::CheckoutTextureStyle(ETextureStyle TextureStyle)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s: %d"), *FString(__FUNCTION__), TextureStyle)
	CurrentTextureStyle = TextureStyle;

	for (auto& Element : TextureMappingAsset->ActorClassPairs)
	{
		AActor* Actor = Element.Key;
		const FString& ClassName = Element.Value;

		UE_LOG(LogEasySynth, Log, TEXT("%s: Painting actor '%s'"), *FString(__FUNCTION__), *Actor->GetName())

		// Get actor mesh components
		TArray<UActorComponent*> ActorComponenets;
		const bool bIncludeFromChildActors = true;
		Actor->GetComponents(UStaticMeshComponent::StaticClass(), ActorComponenets, bIncludeFromChildActors);

		// Set new materials
		for (UActorComponent* ActorComponent : ActorComponenets)
		{
			UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(ActorComponent);
			if (MeshComponent == nullptr)
			{
				UE_LOG(LogEasySynth, Log, TEXT("%s: Got null static mesh component"), *FString(__FUNCTION__))
				return;
			}
			UE_LOG(LogEasySynth, Log, TEXT("%s: Painting mesh component '%s'"),
				*FString(__FUNCTION__), *MeshComponent->GetName());

			// TODO: Work with materials
		}
	}
}

void UTextureStyleManager::LoadOrCreateTextureMappingAsset()
{
	// Try to load
	TextureMappingAsset = LoadObject<UTextureMappingAsset>(nullptr, *FPathUtils::TextureMappingAssetPath());

	if (TextureMappingAsset == nullptr)
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Texture mapping asset not found, creating a new one"),
			*FString(__FUNCTION__));

		// TODO: Remove all potentially existing EasySynth assets from the project

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

		// Don't save the asset yet to prevent crashing the editor on startup
	}
}

void UTextureStyleManager::SaveTextureMappingAsset()
{
	check(TextureMappingAsset)
	const bool bOnlyIfIsDirty = false;
	UEditorAssetLibrary::SaveLoadedAsset(TextureMappingAsset, bOnlyIfIsDirty);
}

void UTextureStyleManager::SetSemanticClassToActor(AActor* Actor, const FString& ClassName)
{
	// Remove class if already assigned
	if (TextureMappingAsset->ActorClassPairs.Contains(Actor))
	{
		TextureMappingAsset->ActorClassPairs.Remove(Actor);
	}

	// Set the new class
	TextureMappingAsset->ActorClassPairs.Add(Actor, ClassName);
}
