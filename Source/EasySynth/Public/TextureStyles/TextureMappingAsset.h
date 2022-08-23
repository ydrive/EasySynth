// Copyright (c) 2022 YDrive Inc. All rights reserved.

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
	UPROPERTY(EditAnywhere, Category = "Semantic Class Properties")
	FString Name;

	/** The semantic class color */
	UPROPERTY(EditAnywhere, Category = "Semantic Class Properties")
	FColor Color;

	/** Reference to the plain color material instance */
	UPROPERTY(EditAnywhere, Category = "Semantic Class Material")
	UMaterialInstanceConstant* PlainColorMaterialInstance;
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
