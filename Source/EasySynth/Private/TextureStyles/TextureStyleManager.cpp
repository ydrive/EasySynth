// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#include "TextureStyles/TextureStyleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"

#include "PathUtils.h"
#include "TextureStyles/TextureMappingAsset.h"


UTextureStyleManager::UTextureStyleManager() :
	CurrentTextureStyle(ETextureStyle::COLOR)
{
	LoadOrCreateTextureMappingAsset();
}

bool UTextureStyleManager::NewSemanticClass(const FString& ClassName, const FColor& ClassColor)
{
	// TODO: Check collision with existing classes
	// TODO: Crate a new class with its material instance
	return true;
}

void UTextureStyleManager::CheckoutTextureStyle(ETextureStyle TextureStyle)
{
	// TODO: Apply new style to all meshes
	CurrentTextureStyle = TextureStyle;
}

void UTextureStyleManager::LoadOrCreateTextureMappingAsset()
{
	// Try to load
	TextureMappingAsset = LoadObject<UTextureMappingAsset>(nullptr, *FPathUtils::TextureMappingAssetPath());

	if (TextureMappingAsset == nullptr)
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Texture mapping asset not found, creating a new one"), *FString(__FUNCTION__));

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

		// Save the asset
		SaveTextureMappingAsset();
	}
}

void UTextureStyleManager::SaveTextureMappingAsset()
{
	check(TextureMappingAsset)
	const bool bOnlyIfIsDirty = false;
	UEditorAssetLibrary::SaveLoadedAsset(TextureMappingAsset, bOnlyIfIsDirty);
}
