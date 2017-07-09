// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "TerraformaLandscapeLoader.h"
#include "EngineDefines.h"

UTerraformaLandscapeLoader::UTerraformaLandscapeLoader(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTerraformaLandscapeLoader::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	uint32 DataSize = TFAFileData.Num();
	Ar.SerializeInt(DataSize,1);

	if (Ar.IsLoading()) {
		TFAFileData.SetNum(DataSize);
		Ar.SerializeCompressed(TFAFileData.GetData(), TFAFileData.Num(), COMPRESS_Default);
	}
	else {
		Ar.SerializeCompressed(TFAFileData.GetData(), TFAFileData.Num(), COMPRESS_Default);
	}

	
}

