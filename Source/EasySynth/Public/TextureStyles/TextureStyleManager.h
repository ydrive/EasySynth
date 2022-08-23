// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "TextureStyleManager.generated.h"

class AActor;
class UMaterial;

struct FSemanticClass;
class UMaterialInstanceConstant;
class UTextureBackupManager;
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

	/** Called when the editor has finished starting up to bind event handlers */
	void BindEvents();

	/** Create new semantic class */
	bool NewSemanticClass(
		const FString& ClassName,
		const FColor& ClassColor,
		const bool bSaveTextureMappingAsset = true);

	/** Gets the class color if it exists */
	FColor ClassColor(const FString& ClassName);

	/** Update the name of a semantic class */
	bool UpdateClassName(const FString& OldClassName, const FString& NewClassName);

	/** Update the color of a semantic class */
	bool UpdateClassColor(const FString& ClassName, const FColor& NewClassColor);

	/** Remove a semantic class */
	bool RemoveSemanticClass(const FString& ClassName);

	/** Remove all semantic classes except for the default one */
	void RemoveAllSemanticCLasses();

	/** Returns names of existing semantic classes */
	TArray<FString> SemanticClassNames() const;

	/** Returns array of const pointers to semantic classes */
	TArray<const FSemanticClass*> SemanticClasses() const;

	/** Applies desired class to all selected actors */
	void ApplySemanticClassToSelectedActors(const FString& ClassName);

	/** Update mesh materials to show requested texture styles */
	void CheckoutTextureStyle(const ETextureStyle NewTextureStyle);

	/** Get the selected texture style */
	ETextureStyle SelectedTextureStyle() const { return CurrentTextureStyle; }

	/** Delegate type used to broadcast the semantic classes updated event */
	DECLARE_EVENT(UTextureStyleManager, FSemanticClassesUpdatedEvent);

	/** Returns a reference to the event for others to bind */
	FSemanticClassesUpdatedEvent& OnSemanticClassesUpdated() { return SemanticClassesUpdatedEvent; }

	/** Export current semantic classes to a CSV file */
	bool ExportSemanticClasses(const FString& OutputDir);

private:
	/** Load or create texture mapping asset on startup */
	void LoadOrCreateTextureMappingAsset();

	/** Save texture mapping asset modifications */
	void SaveTextureMappingAsset();

	/** Handles adding a new actor to the level */
	void OnLevelActorAdded(AActor* Actor);

	/** Handles removing actor references from the manager */
	void OnLevelActorDeleted(AActor* Actor);

	/** Handles editor closing, making sure original mesh colors are selected */
	void OnEditorClose();

	/** Sets a semantic class to the actor */
	void SetSemanticClassToActor(
		AActor* Actor,
		const FString& ClassName,
		const bool bForceDisplaySemanticClass = false,
		const bool bDelayAddingDescriptors = false);

	/** Set active actor texture style to original or semantic color */
	void CheckoutActorTexture(AActor* Actor, const ETextureStyle NewTextureStyle);

	/** Adds semantic classes to actors in the delay actor buffer after a delay */
	void ProcessDelayActorBuffer();

	/** Generates the semantic class material if needed and returns it */
	UMaterialInstanceConstant* GetSemanticClassMaterial(FSemanticClass& SemanticClass);

	/** Semantic classes updated event dispatcher */
	FSemanticClassesUpdatedEvent SemanticClassesUpdatedEvent;

	/** Global texture mapping asset of the specific project */
	UPROPERTY()
	UTextureMappingAsset* TextureMappingAsset;

	/** Plain color material used for semantic mesh coloring */
	UPROPERTY()
	UMaterial* PlainColorMaterial;

	/** Currently selected texture style */
	ETextureStyle CurrentTextureStyle;

	/** Object that manages backing up of the original actor textures */
	UPROPERTY()
	UTextureBackupManager* TextureBackupManager;

	/**
	 * Buffer used to store actors that need to have the semantic class set with a delay
	 * This is needed when immediately setting the undefined class to just spawned actor
	*/
	UPROPERTY()
	TArray<AActor*> DelayActorBuffer;

	/** The handle for the timer that managers DelayActorBuffer */
	FTimerHandle DelayActorTimerHandle;

	/** Marks if events have already been bounded */
	bool bEventsBound;

	/** The name of the semantic color material parameter */
	static const FString SemanticColorParameter;

	/** The name of the Undefined semantic class */
	static const FString UndefinedSemanticClassName;
};
