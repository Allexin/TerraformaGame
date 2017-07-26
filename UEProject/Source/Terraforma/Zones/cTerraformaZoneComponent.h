// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "../Landscape/cTerraformaGrid.h"
#include "../Landscape/cCameraGrid.h"
#include "../Landscape/cTerraformaLandscapeComponent.h"
#include "cTerraformaZoneComponent.generated.h"


/**
 * 
 */

const int ZONE_POINTS_PER_TILE = 8;
const float ZONE_TILE_SIZE_CM = CHUNK_SIZE_CM / (CHUNK_H_RESOLUTION / ZONE_POINTS_PER_TILE);
const int ZONE_MESH_RESOLUTION = 128; 
const int ZONE_MESH_HALF_RESOLUTION = ZONE_MESH_RESOLUTION / 2;
const float ZONE_MESH_SIZE = ZONE_TILE_SIZE_CM * ZONE_MESH_RESOLUTION;
const float ZONE_MESH_HALF_SIZE = ZONE_TILE_SIZE_CM * ZONE_MESH_HALF_RESOLUTION;

struct sTerraformaZoneData {
	uint8 realHeight;
	uint8 toolHeight;
};

UCLASS()
class TERRAFORMA_API UcTerraformaZoneComponent : public UStaticMeshComponent
{
	GENERATED_BODY()
protected:
	UcTerraformaLandscapeComponent*			m_LandscapeComponent;
	UMaterial*								m_BaseMaterial;

	int										m_Width;
	int										m_Height;

	float									m_CheckHeightmapCounter;

	void reinitTexture(int width, int height);
protected:
	UPROPERTY()
		UTexture2D*							m_ZoneTexture;
	TArray<sTerraformaZoneData>				m_ZoneData;

	void updateTexture();
public:
	
	UPROPERTY(EditAnywhere, Category = "Visualisation")
		float CheckHeightmapDelay;
	UFUNCTION(BlueprintCallable, Category = "Visualisation")
		void ForceZoneUpdate() { m_CheckHeightmapCounter = 0; }
public:
	UcTerraformaZoneComponent();
	void init(UcTerraformaLandscapeComponent* landscapeComponent) {
		m_LandscapeComponent = landscapeComponent;
	}

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	
	
};
