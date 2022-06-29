// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

#include "TextureBackupManager.generated.h"

class ALandscapeProxy;
class UMaterialInstanceDynamic;
class UMaterialInterface;


/** Structure wrapping TArray of static mesh component materials */
USTRUCT()
struct FOriginalComponentDescriptor
{
	GENERATED_USTRUCT_BODY()

	/** The array of material interfaces */
	UPROPERTY()
	TArray<UMaterialInterface*> MaterialInterfaces;

	/** Wrap TArray Add method */
	void Add(UMaterialInterface* MaterialInterface) { MaterialInterfaces.Add(MaterialInterface); }

	/** Wrap TArray Num method */
	int Num() const { return MaterialInterfaces.Num(); }

	/** Wrap TArray [] operator */
	UMaterialInterface* operator[](int i) { return MaterialInterfaces[i]; }
};


/** Structure wrapping TMap of static mesh components to their original materials */
USTRUCT()
struct FOriginalActorDescriptor
{
	GENERATED_USTRUCT_BODY()

	/** The map of component descriptors */
	UPROPERTY()
	TMap<UStaticMeshComponent*, FOriginalComponentDescriptor> CompDescriptors;

	/** Wrap TMap Add method */
	void Add(UStaticMeshComponent* Component) { CompDescriptors.Add(Component); }

	/** Wrap TMap Contains method */
	bool Contains(UStaticMeshComponent* Component) const { return CompDescriptors.Contains(Component); }

	/** Wrap TMap [] operator */
	FOriginalComponentDescriptor& operator[](UStaticMeshComponent* Component) { return CompDescriptors[Component]; }
};

/**
 * Class that keeps backup of actors' original materials while semantic ones are displayed,
 * also handles material swapping
 */
UCLASS()
class UTextureBackupManager : public UObject
{
	GENERATED_BODY()

public:
	UTextureBackupManager() {};

    /**
	 * Function that serves double purpose of backing up the original actor material
	 * and swapping the displayed material
	*/
    void AddAndPaint(
        AActor* Actor,
        const bool bDoAdd,
        const bool bDoPaint,
        UMaterialInstanceDynamic* Material = nullptr);

    /** Checks whether the actor exists inside any of the caches */
    bool ContainsActor(AActor* Actor);

    /** Removes the actor from its cache if it exists */
    void RemoveActor(AActor* Actor);

private:
    /** Sub-method of the AddAndPaint that handles landscape actors */
    void AddLandscapeActor(
        ALandscapeProxy* LandscapeProxy,
        const bool bDoAdd,
        const bool bDoPaint,
        UMaterialInstanceDynamic* Material);

    /** Sub-method of the AddAndPaint that handles default static mesh actors */
    void AddDefaultActor(
        AActor* Actor,
        const bool bDoAdd,
        const bool bDoPaint,
        UMaterialInstanceDynamic* Material);

	/**
	 * Storage of the original actor materials while semantics are displayed
	 * Mimics the behavior of the structure defined as
	 * TMap<AActor*, TMap<UStaticMeshComponent*, TArray<UMaterialInterface*>>>
	 * which throws the UE specific compile error "Nested containers are not supported."
	*/
	UPROPERTY()
	TMap<AActor*, FOriginalActorDescriptor> OriginalActorDescriptors;

    /** Storage of the original landscape materials while semantics are displayed */
	UPROPERTY()
	TMap<ALandscapeProxy*, UMaterialInterface*> LandscapeActorDescriptors;
};