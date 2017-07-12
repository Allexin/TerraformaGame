// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "sTerraformaTemplate.h"
#include "cTerraformaUtilsLibrary.generated.h"

/**
 * 
 */
UCLASS()
class TERRAFORMA_API UcTerraformaUtilsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Utils")
		static bool GetHeightMapTemplateFromTexture(UTexture2D* texture, FsTerraformaTemplate& heightmap, UEngine* GEngineErrorHandler);
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Utils")
		static bool GetColorMapTemplateFromTexture(UTexture2D* texture, FsTerraformaTemplate& colormap, UEngine* GEngineErrorHandler);
	
};
