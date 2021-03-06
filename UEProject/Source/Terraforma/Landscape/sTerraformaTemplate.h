// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/UserDefinedStruct.h"
#include "sTerraformaTemplate.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class ETerraformingToolEnum : uint8
{
	TTE_OFF 	UMETA(DisplayName = "OFF"),
	TTE_CLEAR 	UMETA(DisplayName = "Clear"),
	TTE_FLAT 	UMETA(DisplayName = "Flat"),
	TTE_UP 		UMETA(DisplayName = "Up"),
	TTE_DOWN 	UMETA(DisplayName = "Down")
};

UENUM(BlueprintType)
enum class ETemplateTypeEnum : uint8
{
	TTE_HEIGHTMAP 	UMETA(DisplayName = "Heightmap"),
	TTE_TEXTURE 	UMETA(DisplayName = "Texture")
};

USTRUCT(BlueprintType)
struct FsTerraformaTemplate{
	GENERATED_BODY()

	TArray<uint8> RawData;
	int Size;
	ETemplateTypeEnum Type;

	int getIndex(int x, int y) const {
		return Type == ETemplateTypeEnum::TTE_HEIGHTMAP ? (y*Size + x) : (y*Size + x)*4;
	}
};
