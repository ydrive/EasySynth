// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#include "TextureStyles/TextureStyleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "EditorAssetLibrary.h"
#include "Engine/Selection.h"
#include "FileHelpers.h"
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
		GEngine->OnEditorClose().AddUObject(this, &UTextureStyleManager::OnEditorClose);
		bEventsBound = true;
	}
}

bool UTextureStyleManager::NewSemanticClass(
	const FString& ClassName,
	const FColor& ClassColor,
	const bool bSaveTextureMappingAsset)
{
	if (ClassName.Len() == 0)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: Cannot create a class with the name ''"), *FString(__FUNCTION__));
		return false;
	}

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

	// Broadcast the semantic classes change
	SemanticClassesUpdatedEvent.Broadcast();

	return true;
}

FColor UTextureStyleManager::ClassColor(const FString& ClassName)
{
	if (TextureMappingAsset->SemanticClasses.Contains(ClassName))
	{
		return TextureMappingAsset->SemanticClasses[ClassName].Color;
	}
	return FColor::White;
}

bool UTextureStyleManager::UpdateClassName(const FString& OldClassName, const FString& NewClassName)
{
	if (OldClassName == NewClassName)
	{
		return true;
	}

	if (!TextureMappingAsset->SemanticClasses.Contains(OldClassName))
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Previous semantic class '%s' not found"),
			*FString(__FUNCTION__), *OldClassName);
		return false;
	}

	if (TextureMappingAsset->SemanticClasses.Contains(NewClassName))
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: New semantic class '%s' already exists"),
			*FString(__FUNCTION__), *NewClassName);
		return false;
	}

	if (NewClassName.Len() == 0)
	{
		UE_LOG(LogEasySynth, Warning, TEXT("%s: Cannot update class name to ''"), *FString(__FUNCTION__));
		return false;
	}

	const FColor ClassColor = TextureMappingAsset->SemanticClasses[OldClassName].Color;

	// Remove the existing class
	TextureMappingAsset->SemanticClasses.Remove(OldClassName);
	// Add new class with the same color
	NewSemanticClass(NewClassName, ClassColor);
	// Update actor mappings to the new semantic class name
	TArray<AActor*> LevelActors;
	UGameplayStatics::GetAllActorsOfClass(GEditor->GetEditorWorldContext().World(), AActor::StaticClass(), LevelActors);
	for (AActor* Actor : LevelActors)
	{
		if (TextureMappingAsset->ActorClassPairs.Contains(Actor->GetActorGuid()) &&
			TextureMappingAsset->ActorClassPairs[Actor->GetActorGuid()] == OldClassName)
		{
			TextureMappingAsset->ActorClassPairs[Actor->GetActorGuid()] = NewClassName;
		}
	}
	// No action regarding actor materials necessary

	SaveTextureMappingAsset();

	return true;
}

bool UTextureStyleManager::UpdateClassColor(const FString& ClassName, const FColor& NewClassColor)
{
	if (!TextureMappingAsset->SemanticClasses.Contains(ClassName))
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Requested semantic class '%s' not found"),
			*FString(__FUNCTION__), *ClassName);
		return false;
	}

	if (TextureMappingAsset->SemanticClasses[ClassName].Color == NewClassColor)
	{
		return true;
	}

	// Check if color is already in use
	for (auto& Element : TextureMappingAsset->SemanticClasses)
	{
		const FSemanticClass& SemanticClass = Element.Value;
		if (SemanticClass.Color == NewClassColor)
		{
			UE_LOG(LogEasySynth, Warning, TEXT("%s: Requested color (%d %d %d) already used by %s"),
				*FString(__FUNCTION__), NewClassColor.R, NewClassColor.G, NewClassColor.B, *SemanticClass.Name);
			return false;
		}
	}

	// Update the class color
	TextureMappingAsset->SemanticClasses[ClassName].Color = NewClassColor;
	// Invalidate the material instance
	TextureMappingAsset->SemanticClasses[ClassName].PlainColorMaterialInstance = nullptr;
	// Update each actor color immediately in case of the semantic view mode
	TArray<AActor*> LevelActors;
	UGameplayStatics::GetAllActorsOfClass(GEditor->GetEditorWorldContext().World(), AActor::StaticClass(), LevelActors);
	for (AActor* Actor : LevelActors)
	{
		if (TextureMappingAsset->ActorClassPairs.Contains(Actor->GetActorGuid()) &&
			TextureMappingAsset->ActorClassPairs[Actor->GetActorGuid()] == ClassName)
		{
			SetSemanticClassToActor(Actor, ClassName);
		}
	}

	SaveTextureMappingAsset();

	return true;
}

bool UTextureStyleManager::RemoveSemanticClass(const FString& ClassName)
{
	if (!TextureMappingAsset->SemanticClasses.Contains(ClassName))
	{
		UE_LOG(LogEasySynth, Log, TEXT("%s: Requested semantic class '%s' not found"),
			*FString(__FUNCTION__), *ClassName);
		return false;
	}

	// Reset all actor to the undefined class
	TArray<AActor*> LevelActors;
	UGameplayStatics::GetAllActorsOfClass(GEditor->GetEditorWorldContext().World(), AActor::StaticClass(), LevelActors);
	for (AActor* Actor : LevelActors)
	{
		if (TextureMappingAsset->ActorClassPairs.Contains(Actor->GetActorGuid()) &&
			TextureMappingAsset->ActorClassPairs[Actor->GetActorGuid()] == ClassName)
		{
			SetSemanticClassToActor(Actor, UndefinedSemanticClassName);
		}
	}

	// Remove the class
	TextureMappingAsset->SemanticClasses.Remove(ClassName);

	SaveTextureMappingAsset();

	// Broadcast the semantic classes change
	SemanticClassesUpdatedEvent.Broadcast();

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

TArray<const FSemanticClass*> UTextureStyleManager::SemanticClasses() const
{
	TArray<const FSemanticClass*> SemanticClasses;
	for (auto& Element : TextureMappingAsset->SemanticClasses)
	{
		SemanticClasses.Add(&Element.Value);
	}
	return SemanticClasses;
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
		OriginalActorDescriptors.Empty();
	}

	// Apply materials to all actors
	TArray<AActor*> LevelActors;
	UGameplayStatics::GetAllActorsOfClass(GEditor->GetEditorWorldContext().World(), AActor::StaticClass(), LevelActors);
	for (AActor* Actor : LevelActors)
	{
		// Get actor mesh components
		TArray<UActorComponent*> ActorComponenets;
		const bool bIncludeFromChildActors = true;
		Actor->GetComponents(UStaticMeshComponent::StaticClass(), ActorComponenets, bIncludeFromChildActors);

		// If no mesh componenets are found, ignore the actor
		if (ActorComponenets.Num() == 0)
		{
			continue;
		}

		// Cannot just proceed if the actor is unknown
		if (!TextureMappingAsset->ActorClassPairs.Contains(Actor->GetActorGuid()))
		{
			if (CurrentTextureStyle == ETextureStyle::COLOR)
			{
				// Moving from original to semantic colors, we need to assign a class to the actor
				SetSemanticClassToActor(Actor, UndefinedSemanticClassName);
			}
			else if (CurrentTextureStyle == ETextureStyle::SEMANTIC)
			{
				// Moving from semantic to original colors, cross the fingers and ignore the actor for now
				UE_LOG(LogEasySynth, Warning, TEXT("%s: Found unknown actor '%s' while transitionsing from semantic to original color mode, ignoring it"),
					*FString(__FUNCTION__), *Actor->GetName())
				continue;
			}
		}

		const FString& ClassName = TextureMappingAsset->ActorClassPairs[Actor->GetActorGuid()];

		UE_LOG(LogEasySynth, Log, TEXT("%s: Painting actor '%s'"), *FString(__FUNCTION__), *Actor->GetName())

		if (CurrentTextureStyle == ETextureStyle::COLOR)
		{
			// Changing to semantic view, store the original data
			OriginalActorDescriptors.Add(Actor);
		}
		else if (CurrentTextureStyle == ETextureStyle::SEMANTIC)
		{
			if (!OriginalActorDescriptors.Contains(Actor))
			{
				UE_LOG(LogEasySynth, Error, TEXT("%s: Actor original descriptor not found"), *FString(__FUNCTION__))
				continue;
			}
		}

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
				OriginalActorDescriptors[Actor].Add(MeshComponent);
			}
			else if (CurrentTextureStyle == ETextureStyle::SEMANTIC)
			{
				if (!OriginalActorDescriptors[Actor].Contains(MeshComponent))
				{
					UE_LOG(LogEasySynth, Error, TEXT("%s: Actor's mesh component original descriptor not found"),
						*FString(__FUNCTION__))
					continue;
				}
				else if (OriginalActorDescriptors[Actor][MeshComponent].Num() != MeshComponent->GetNumMaterials())
				{
					UE_LOG(LogEasySynth, Error, TEXT("%s: %d instead of %d actor's mesh component materials found"),
						*FString(__FUNCTION__),
						OriginalActorDescriptors[Actor][MeshComponent].Num(),
						MeshComponent->GetNumMaterials())
					continue;
				}
			}

			UE_LOG(LogEasySynth, Log, TEXT("%s: Painting mesh component '%s'"),
				*FString(__FUNCTION__), *MeshComponent->GetName());

			for (int i = 0; i < MeshComponent->GetNumMaterials(); i++)
			{
				// Apply to each material
				if (CurrentTextureStyle == ETextureStyle::COLOR)
				{
					// Change to semantic material
					OriginalActorDescriptors[Actor][MeshComponent].Add(MeshComponent->GetMaterial(i));
					MeshComponent->SetMaterial(
						i, GetSemanticClassMaterial(TextureMappingAsset->SemanticClasses[ClassName]));
				}
				else
				{
					// Revert to original material
					MeshComponent->SetMaterial(i, OriginalActorDescriptors[Actor][MeshComponent][i]);
				}
			}
		}
	}

	if (CurrentTextureStyle == ETextureStyle::SEMANTIC)
	{
		// Reset the original actor material storage as it is no longer needed
		OriginalActorDescriptors.Empty();
	}

	// Make sure any changes to the TextureMappingAsset are changed
	SaveTextureMappingAsset();

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

		// Register the plugin directroy with the editor
		FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().AddPath(FPathUtils::ProjectPluginContentDir());

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

	// Preemprively assign the undefined semantic class to the new actor
	// In the case of the semantic mode being selected, assigned class will be immediately displayed
	const bool bDelayAddingDescriptors = true;
	SetSemanticClassToActor(Actor, UndefinedSemanticClassName, bDelayAddingDescriptors);
}

void UTextureStyleManager::OnLevelActorDeleted(AActor* Actor)
{
	UE_LOG(LogEasySynth, Log, TEXT("%s: Removing actor '%s'"), *FString(__FUNCTION__), *Actor->GetName())
	TextureMappingAsset->ActorClassPairs.Remove(Actor->GetActorGuid());
	OriginalActorDescriptors.Remove(Actor);
}

void UTextureStyleManager::OnEditorClose()
{
	UE_LOG(LogEasySynth, Log, TEXT("%s: Making sure original mesh colors are selected"), *FString(__FUNCTION__))
	CheckoutTextureStyle(ETextureStyle::COLOR);
	// Make level dirty and save it
	ULevel* Level = GWorld->GetCurrentLevel();
	Level->MarkPackageDirty();
	FEditorFileUtils::SaveLevel(Level);
}

void UTextureStyleManager::SetSemanticClassToActor(
	AActor* Actor,
	const FString& ClassName,
	const bool bDelayAddingDescriptors)
{
	// Remove class if already assigned
	TextureMappingAsset->ActorClassPairs.Remove(Actor->GetActorGuid());

	// Set the new class
	TextureMappingAsset->ActorClassPairs.Add(Actor->GetActorGuid(), ClassName);

	// Immediately display the change when in the semantic mode
	if (CurrentTextureStyle == ETextureStyle::SEMANTIC)
	{
		// Simplified checkout of the semantic view for this actor

		// Get actor mesh components
		TArray<UActorComponent*> ActorComponenets;
		const bool bIncludeFromChildActors = true;
		Actor->GetComponents(UStaticMeshComponent::StaticClass(), ActorComponenets, bIncludeFromChildActors);

		// If no mesh componenets are found, ignore the actor
		if (ActorComponenets.Num() == 0)
		{
			return;
		}

		// In case this actor has not already been added to the original descriptor,
		// add its contents before changing the material
		const bool bAddingNew = !OriginalActorDescriptors.Contains(Actor);
		if (bAddingNew)
		{
			if (bDelayAddingDescriptors)
			{
				// Adding with a delay is requested, add the actor to the delay buffer, start the timer,
				// stop the current execution and come back in a few millisecond
				DelayActorBuffer.Add(Actor);
				const float DelaySeconds = 0.2f;
				const bool bLoop = false;
				GEditor->GetEditorWorldContext().World()->GetTimerManager().SetTimer(
					DelayActorTimerHandle,
					this,
					&UTextureStyleManager::ProcessDelayActorBuffer,
					DelaySeconds,
					bLoop);
				return;
			}
			OriginalActorDescriptors.Add(Actor);
		}

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
				OriginalActorDescriptors[Actor].Add(MeshComponent);
			}

			// Store actor descriptors immediately
			for (int i = 0; i < MeshComponent->GetNumMaterials(); i++)
			{
				// Apply to each material
				if (bAddingNew)
				{
					OriginalActorDescriptors[Actor][MeshComponent].Add(MeshComponent->GetMaterial(i));
				}

				MeshComponent->SetMaterial(
					i, GetSemanticClassMaterial(TextureMappingAsset->SemanticClasses[ClassName]));
			}
		}
	}

	// No need to save the TextureMappingAsset for every actor, the caller will do it
}

void UTextureStyleManager::ProcessDelayActorBuffer()
{
	bool bAnyActorProcessed = false;
	for (AActor* Actor : DelayActorBuffer)
	{
		if (IsValid(Actor))
		{
			// Must not call with bDelayAddingDescriptors = true, to avoid infinite recursion
			SetSemanticClassToActor(Actor, UndefinedSemanticClassName);
			bAnyActorProcessed = true;
		}
	}
	DelayActorBuffer.Empty();
	if (bAnyActorProcessed)
	{
		SaveTextureMappingAsset();
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
