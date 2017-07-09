// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "TerraformaLandscapeLoaderImporterFactory.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "TerraformaLandscapeLoaderImporterPrivate.h"
#include "AssetToolsModule.h"

//////////////////////////////////////////////////////////////////////////
// FTerraformaLandscapeLoaderImporterModule

class FTerraformaLandscapeLoaderImporterModule : public FDefaultModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// Register the asset type
	}

	virtual void ShutdownModule() override
	{
		if (!UObjectInitialized())
		{
			return;
		}

		// Only unregister if the asset tools module is loaded.  We don't want to forcibly load it during shutdown phase.
	}
};

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_MODULE(FTerraformaLandscapeLoaderImporterModule, TerraformaLandscapeLoaderImporter);
DEFINE_LOG_CATEGORY(LogTerraformaLandscapeLoaderImporter);
