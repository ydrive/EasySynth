// Copyright (c) 2022 YDrive Inc. All rights reserved.

#pragma once

#include "CoreMinimal.h"

class UTextureMappingAsset;
class UTextureStyleManager;


/**
 * Class containing interface methods for parsing and generating
 * CVS files representing semantic class sets
 */
class FSemanticCsvInterface
{
public:
	FSemanticCsvInterface() {}

	/** Handles importing semantic classes from a CSV file */
	FReply OnImportSemanticClassesClicked(UTextureStyleManager* TextureStyleManager);

	/** Handles exporting semantic classes into a CSV file */
	bool ExportSemanticClasses(const FString& OutputDir, UTextureMappingAsset* TextureMappingAsset);
};
