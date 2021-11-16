// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataAsset.h"

#include "TextureMappingAsset.generated.h"


/** Structure that represents a user-defined semantic class */
USTRUCT()
struct FSemanticClass
{
	GENERATED_USTRUCT_BODY()

	/** The semantic class name */
	UPROPERTY()
	FString Name;

	/** The semantic class color */
	UPROPERTY()
	FColor Color;

	/** Reference to the plain color material instance */
	// TODO: Use material instance
	UPROPERTY()
	UMaterial* PlainColorMaterial;
};


/** An asset containing semantic mapping for each actor */
UCLASS()
class UTextureMappingAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Created semantic classes */
	UPROPERTY()
	TMap<FString, FSemanticClass> SemanticClasses;

	/** Actor to semantic class name bindings */
	UPROPERTY()
	TMap<AActor*, FString> ActorClassPairs;
};
