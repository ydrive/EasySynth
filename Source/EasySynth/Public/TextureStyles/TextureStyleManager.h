// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "TextureStyleManager.generated.h"

class UTextureMappingAsset;


/** Enum representing mesh texture styles */
UENUM()
enum class ETextureStyle : uint8
{
	COLOR = 0 UMETA(DisplayName = "COLOR"),
	SEMANTIC = 1 UMETA(DisplayName = "SEMANTIC"),
};


/**
 * Class for managing mesh texture appearances,
 * such as colored and semantic views
*/
UCLASS()
class UTextureStyleManager : public UObject
{
	GENERATED_BODY()
public:
	UTextureStyleManager();

	/** Create new semantic class */
	bool NewSemanticClass(const FString& ClassName, const FColor& ClassColor);

	/** Returns names of existing semantic classes */
	TArray<FString> SemanticClassNames() const;

	/** Apllies desired class to all selected actors */
	void ApplySemanticClass(const FString& ClassName);

	/** Update mesh materials to show requested texture styles */
	void CheckoutTextureStyle(ETextureStyle TextureStyle);

	/** Get the selected texture style */
	ETextureStyle SelectedTextureStyle() const { return CurrentTextureStyle; }

private:
	/** Load or create texture mapping asset on startup */
	void LoadOrCreateTextureMappingAsset();

	/** Save texture mapping asset modifications */
	void SaveTextureMappingAsset();

	/** Sets a semantic class to the actor */
	void SetSemanticClassToActor(AActor* Actor, const FString& ClassName);

	/** Global texture mapping asset of the specific project */
	UPROPERTY()
	UTextureMappingAsset* TextureMappingAsset;

	/** Currently selected texture style */
	ETextureStyle CurrentTextureStyle;
};
