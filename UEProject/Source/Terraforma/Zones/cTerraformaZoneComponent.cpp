// Fill out your copyright notice in the Description page of Project Settings.

#include "cTerraformaZoneComponent.h"
#include "ConstructorHelpers.h"


UcTerraformaZoneComponent::UcTerraformaZoneComponent() :UStaticMeshComponent(), m_LandscapeComponent(nullptr){
	PrimaryComponentTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> heightToolMeshCH(TEXT("StaticMesh'/Game/GameTechData/height_tool_mesh.height_tool_mesh'"));
	static ConstructorHelpers::FObjectFinder<UMaterial> heightToolMatCH(TEXT("Material'/Game/GameTechData/M_HeightTool.M_HeightTool'"));
	
	SetStaticMesh(heightToolMeshCH.Object);

	if (heightToolMatCH.Succeeded()) 
		m_BaseMaterial = heightToolMatCH.Object;
	SetWorldScale3D(FVector(ZONE_TILE_SIZE_CM, ZONE_TILE_SIZE_CM,0.f));

	m_CheckHeightmapCounter = 0.f;
	CheckHeightmapDelay = 1.f;

	SetVisibility(false);
}

void UcTerraformaZoneComponent::reinitTexture(int width, int height) {
	m_Width = width;
	m_Height = height;
	m_ZoneTexture = UTexture2D::CreateTransient(m_Width, m_Height, PF_R8G8);																										
	m_ZoneTexture->SRGB = 0;
	m_ZoneTexture->Filter = TextureFilter::TF_Nearest;

	m_ZoneData.SetNum(m_Width*m_Height);

	UMaterialInstanceDynamic* MatInst = UMaterialInstanceDynamic::Create(m_BaseMaterial, this);
	if (MatInst) {
		MatInst->SetTextureParameterValue(FName("PHeightmap"), m_ZoneTexture);
		SetMaterial(0, MatInst);
	}
}

void UcTerraformaZoneComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) {
	if (m_LandscapeComponent == nullptr)
		return;

	const cTerraformaGrid& landscape = m_LandscapeComponent->getLandscape();
	int w = (landscape.width()  * CHUNK_H_RESOLUTION) / ZONE_POINTS_PER_TILE;
	int h = (landscape.height() * CHUNK_H_RESOLUTION) / ZONE_POINTS_PER_TILE;

	if (w == 0 || h == 0)
		return;

	if (w != m_Width || h != m_Height)
		reinitTexture(w,h);
	
	if (GEngine) {
		UWorld* world = GetWorld();
		if (world) {
			APlayerController* pc = GEngine->GetFirstLocalPlayerController(world);
			if (pc) {
				APlayerCameraManager* cameraManager = pc->PlayerCameraManager;
				if (cameraManager) {
					UMaterialInstanceDynamic* MatInst = Cast<UMaterialInstanceDynamic>(GetMaterial(0));
					if (MatInst) {
						FVector cameraPos = cameraManager->CameraCache.POV.Location;
						FVector cameraDirection = FRotationMatrix(cameraManager->CameraCache.POV.Rotation).GetScaledAxis(EAxis::X);
						cameraDirection.Normalize();
						cameraPos = cameraPos + cameraDirection * ZONE_MESH_HALF_SIZE * 0.7f;
						FIntPoint ZonePos((int)((cameraPos.X - ZONE_TILE_SIZE_CM*ZONE_MESH_HALF_RESOLUTION) / ZONE_TILE_SIZE_CM), (int)((cameraPos.Y - ZONE_TILE_SIZE_CM*ZONE_MESH_HALF_RESOLUTION) / ZONE_TILE_SIZE_CM));
						ZonePos.X = FMath::Clamp(ZonePos.X, 0, m_Width - ZONE_MESH_RESOLUTION);
						ZonePos.Y = FMath::Clamp(ZonePos.Y, 0, m_Height - ZONE_MESH_RESOLUTION);

						SetRelativeLocation(FVector(ZonePos.X*ZONE_TILE_SIZE_CM + ZONE_MESH_HALF_SIZE, ZonePos.Y*ZONE_TILE_SIZE_CM + ZONE_MESH_HALF_SIZE,0));

						float uSize = 1.f / m_Width;
						float vSize = 1.f / m_Height;
						MatInst->SetScalarParameterValue(FName("PUStart"), ZonePos.X* uSize);
						MatInst->SetScalarParameterValue(FName("PVStart"), ZonePos.Y* vSize);

						MatInst->SetScalarParameterValue(FName("PUScale"), (float)ZONE_MESH_RESOLUTION / m_Width);
						MatInst->SetScalarParameterValue(FName("PVScale"), (float)ZONE_MESH_RESOLUTION / m_Height);
					}
				}
			}
		}
	}

	m_CheckHeightmapCounter -= DeltaTime;
	if (m_CheckHeightmapCounter <= 0) {
		m_CheckHeightmapCounter = CheckHeightmapDelay;

		int i = 0;
		bool needReinitTexture = false;
		for (int y = 0; y<m_Height; y++)
			for (int x = 0; x < m_Width; x++) {
				uint8 realHeight = landscape.getHeightDirect(x*ZONE_POINTS_PER_TILE, y*ZONE_POINTS_PER_TILE) / 256;
				uint8 toolHeight = landscape.getToolHeightDirect(x*ZONE_POINTS_PER_TILE, y*ZONE_POINTS_PER_TILE);
				if (realHeight != m_ZoneData[i].realHeight || toolHeight != m_ZoneData[i].toolHeight) {
					needReinitTexture = true;
					m_ZoneData[i].realHeight = realHeight;
					m_ZoneData[i].toolHeight = toolHeight;
				}

				i++;
			}
		if (needReinitTexture)
			updateTexture();
	}
}

void UcTerraformaZoneComponent::updateTexture() {
	FTexture2DMipMap& Mip = m_ZoneTexture->PlatformData->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, m_ZoneData.GetData(), m_ZoneData.Num() * sizeof(sTerraformaZoneData));
	Mip.BulkData.Unlock();
	m_ZoneTexture->UpdateResource();
}