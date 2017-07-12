// Fill out your copyright notice in the Description page of Project Settings.

#include "cTerraformaUtilsLibrary.h"

bool UcTerraformaUtilsLibrary::GetHeightMapTemplateFromTexture(UTexture2D* texture, FsTerraformaTemplate& heightmap, uint8 clampMin, uint8 clampMax) {
	if (texture == nullptr)
		return false;
	FTexture2DMipMap& MipH = texture->PlatformData->Mips[0];
	if (MipH.SizeX != MipH.SizeY) {
		UE_LOG(LogTemp, Error, TEXT("%s"), TEXT("Can't get terraforma heightmap template from texture, texture must be quad"));
		return false; //musq be quad texture
	}

	if (MipH.BulkData.GetElementCount() != MipH.SizeX*MipH.SizeY) {
		UE_LOG(LogTemp, Error, TEXT("%s"), *("Can't get terraforma heightmap template from texture, size is " + FString::FromInt(MipH.BulkData.GetElementCount()) + " but need " + FString::FromInt(MipH.SizeX*MipH.SizeY)));
		return false; //only 1 channel maps supported for heightmap template
	}

	heightmap.Size = MipH.SizeX;
	heightmap.RawData.SetNum(heightmap.Size*heightmap.Size);
	heightmap.Type = ETemplateTypeEnum::TTE_HEIGHTMAP;
	

	void* DataH = MipH.BulkData.Lock(LOCK_READ_ONLY);
	FMemory::Memcpy(heightmap.RawData.GetData(), DataH, heightmap.Size*heightmap.Size);
	MipH.BulkData.Unlock();

	if (clampMin != 0 || clampMax!=255)
		for (int i = 0; i < heightmap.RawData.Num(); i++)
			heightmap.RawData[i] = FMath::Clamp(heightmap.RawData[i], clampMin, clampMax);

	return true;
}
bool UcTerraformaUtilsLibrary::GetColorMapTemplateFromTexture(UTexture2D* texture, FsTerraformaTemplate& colormap) {
	if (texture == nullptr)
		return false;
	FTexture2DMipMap& MipT = texture->PlatformData->Mips[0];
	if (MipT.SizeX != MipT.SizeY) {
		UE_LOG(LogTemp, Error, TEXT("%s"), TEXT("Can't get terraforma colormap template from texture, texture must be quad"));
		return false; //musq be quad texture
	}

	if (MipT.BulkData.GetElementCount() != MipT.SizeX*MipT.SizeY * 4) {
		UE_LOG(LogTemp, Error, TEXT("%s"), *("Can't get terraforma colormap template from texture, size is " + FString::FromInt(MipT.BulkData.GetElementCount()) + " but need " + FString::FromInt(MipT.SizeX*MipT.SizeY)));
		return false; //only 1 channel maps supported for heightmap template
	}

	colormap.Size = MipT.SizeX;
	colormap.RawData.SetNum(colormap.Size*colormap.Size*4);
	colormap.Type = ETemplateTypeEnum::TTE_TEXTURE;

	void* DataT = MipT.BulkData.Lock(LOCK_READ_ONLY);
	FMemory::Memcpy(colormap.RawData.GetData(), DataT, colormap.RawData.Num());
	MipT.BulkData.Unlock();

	return true;
}