// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "TerraformaLandscapeLoaderImporterFactory.h"
#include "EngineGlobals.h"
#include "Editor.h"
#include "TerraformaLandscapeLoader.h"
//////////////////////////////////////////////////////////////////////////
// UTerraformaLandscapeLoaderImporterFactory


UTerraformaLandscapeLoaderImporterFactory::UTerraformaLandscapeLoaderImporterFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = false;
	SupportedClass = UTerraformaLandscapeLoader::StaticClass();

	bEditorImport = true;

	Formats.Add(TEXT("tfa;Terraforma Landscape File"));
}

UObject* UTerraformaLandscapeLoaderImporterFactory::FactoryCreateBinary
(
	UClass*				Class,
	UObject*			InParent,
	FName				Name,
	EObjectFlags		Flags,
	UObject*			Context,
	const TCHAR*		FileType,
	const uint8*&		Buffer,
	const uint8*		BufferEnd,
	FFeedbackContext*	Warn
)
{	
	UTerraformaLandscapeLoader* Landscape = NewObject<UTerraformaLandscapeLoader>(InParent, Name, Flags);
	
	Landscape->TFAFileData.SetNum(BufferEnd - Buffer);
	FMemory::Memcpy(Landscape->TFAFileData.GetData(), Buffer, BufferEnd - Buffer);

	FEditorDelegates::OnAssetPostImport.Broadcast(this, Landscape);

	return Landscape;
}
