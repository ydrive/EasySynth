// Copyright (c) YDrive Inc. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "CoreMinimal.h"

#include "Engine/DataAsset.h"

#include "TextureMappingAsset.generated.h"


/** Structure that represents a user-defined semantic class */
USTRUCT(BlueprintType)
struct FSemanticClass
{
	GENERATED_USTRUCT_BODY()

	/** The semantic class name */
	UPROPERTY(EditAnywhere, Category = "Semantic Class Propterties")
	FString Name;

	/** The semantic class color */
	UPROPERTY(EditAnywhere, Category = "Semantic Class Propterties")
	FColor Color;

	/** Reference to the plain color material instance */
	UPROPERTY(EditAnywhere, Category = "Semantic Class Material")
	UMaterialInstanceDynamic* PlainColorMaterialInstance;
};


/** An asset containing semantic mapping for each actor */
UCLASS()
class EASYSYNTH_API UTextureMappingAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Created semantic classes */
	UPROPERTY(EditAnywhere, Category = "Semantic Classes")
	TMap<FString, FSemanticClass> SemanticClasses;

	/** Actor to semantic class name bindings */
	UPROPERTY(EditAnywhere, Category = "Actor Data")
	TMap<FGuid, FString> ActorClassPairs;
};
