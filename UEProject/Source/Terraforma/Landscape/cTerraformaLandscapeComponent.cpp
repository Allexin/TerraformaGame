// Fill out your copyright notice in the Description page of Project Settings.

#include "cTerraformaLandscapeComponent.h"
#include "ConstructorHelpers.h"









void UcTerraformaLandscapeComponent::ReinitMaterials() {
	int width = m_Landscape.width();
	int height = m_Landscape.height();
	UMaterialInterface* baseMaterial = GetMaterial(0);
	
	m_HeightmapTextures.SetNum(width*height);
	m_ColorTextures.SetNum(width*height);
	m_DynMaterialInstances.SetNum(width*height);

	for (int i = 0; i < width*height; i++) {
		sTerraformaGridChunk* chunk = m_Landscape.getData(i);

		if (i >= MaterialsReadyCount) {
			m_HeightmapTextures[i] = UTexture2D::CreateTransient(CHUNK_RESOLUTION + 2, CHUNK_RESOLUTION + 2, PF_R8G8);// PF_R16_UINT);
			m_HeightmapTextures[i]->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
			m_HeightmapTextures[i]->SRGB = 0;
			m_HeightmapTextures[i]->Filter = TextureFilter::TF_Nearest;//  ESamplerFilter::SF_Point;

			m_ColorTextures[i] = UTexture2D::CreateTransient(CHUNK_RESOLUTION + 2, CHUNK_RESOLUTION + 2, PF_R8G8B8A8);
			m_ColorTextures[i]->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
			m_ColorTextures[i]->Filter = TextureFilter::TF_Trilinear;//  ESamplerFilter::SF_Point;

			m_DynMaterialInstances[i] = UMaterialInstanceDynamic::Create(baseMaterial, this);
			m_DynMaterialInstances[i]->SetTextureParameterValue(FName("PHeightmap"), m_HeightmapTextures[i]);
			m_DynMaterialInstances[i]->SetTextureParameterValue(FName("PTexture"), m_ColorTextures[i]);
			MaterialsReadyCount++;
		}
		

		FTexture2DMipMap& MipH = m_HeightmapTextures[i]->PlatformData->Mips[0];
		void* DataH = MipH.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(DataH, chunk->dynDataHeightMap, (CHUNK_RESOLUTION + 2)* (CHUNK_RESOLUTION + 2) * sizeof(sChunkHeightmapData));
		MipH.BulkData.Unlock();
		m_HeightmapTextures[i]->UpdateResource();
		chunk->HeightmapChanged = false;

		
		FTexture2DMipMap& MipT = m_ColorTextures[i]->PlatformData->Mips[0];
		void* DataT = MipT.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(DataT, chunk->dynDataTexture, (CHUNK_RESOLUTION + 2)* (CHUNK_RESOLUTION + 2) * sizeof(sChunkTextureData));
		MipT.BulkData.Unlock();
		m_ColorTextures[i]->UpdateResource();
		chunk->TextureChanged = false;
	}
}

UcTerraformaLandscapeComponent::UcTerraformaLandscapeComponent():UMeshComponent() {
	PrimaryComponentTick.bCanEverTick = true;	

	static ConstructorHelpers::FObjectFinder<UMaterial> materialCH(TEXT("Material'/Game/LandscapeTechData/M_Terraforma.M_Terraforma'"));
	SetMaterial(0, materialCH.Object);

	ApplyTerraformingDelay = 0.5;

	m_HeightmapTextures.SetNum(0);
	m_ColorTextures.SetNum(0);
	m_DynMaterialInstances.SetNum(0);
	MaterialsReadyCount = 0;
	m_ApplyTerraformingCounter = 0;
	m_Terraformed = false;
}

void UcTerraformaLandscapeComponent::OnRegister() {
	loadLevel();
	Super::OnRegister();
}

void UcTerraformaLandscapeComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) {
	m_ApplyTerraformingCounter += DeltaTime;
	if (m_Terraformed && m_ApplyTerraformingCounter >= ApplyTerraformingDelay) {
		m_Terraformed = false;
		m_ApplyTerraformingCounter = 0;

		if (m_HeightmapTextures.Num() == m_Landscape.width()*m_Landscape.height() && m_ColorTextures.Num() == m_Landscape.width()*m_Landscape.height()) {
			int i = 0;
			for (int y = 0; y < m_Landscape.height(); y++)
				for (int x = 0; x < m_Landscape.width(); x++) {
					sTerraformaGridChunk* chunk = m_Landscape.getData(i);
					if (chunk->HeightmapChanged) {
						chunk->HeightmapChanged = false;
						//FUpdateTextureRegion2D region(0, 0, 0, 0, CHUNK_RESOLUTION + 2, CHUNK_RESOLUTION + 2);
						//m_HeightmapTextures[i]->UpdateTextureRegions(1, 1, &region, 0, 16, (uint8*)chunk->dynDataHeightMap);

						FTexture2DMipMap& MipH = m_HeightmapTextures[i]->PlatformData->Mips[0];
						void* DataH = MipH.BulkData.Lock(LOCK_READ_WRITE);
						FMemory::Memcpy(DataH, chunk->dynDataHeightMap, (CHUNK_RESOLUTION + 2)* (CHUNK_RESOLUTION + 2) * sizeof(sChunkHeightmapData));
						MipH.BulkData.Unlock();
						m_HeightmapTextures[i]->UpdateResource();
						chunk->HeightmapChanged = false;
					}
					if (chunk->TextureChanged) {
						chunk->TextureChanged = false;
						//FUpdateTextureRegion2D region(0, 0, 0, 0, CHUNK_RESOLUTION + 2, CHUNK_RESOLUTION + 2);
						//m_ColorTextures[i]->UpdateTextureRegions(0, 1, &region, (CHUNK_RESOLUTION + 2)*4, 4, (uint8*)chunk->dynDataTexture);
						//UpdateTextureRegions(m_ColorTextures[i], 0, 1, &region, 0, 4, (uint8*)chunk->dynDataTexture, false);

						FTexture2DMipMap& Mip = m_ColorTextures[i]->PlatformData->Mips[0];
						void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
						FMemory::Memcpy(Data, chunk->dynDataTexture, (CHUNK_RESOLUTION + 2)* (CHUNK_RESOLUTION + 2) * 4);
						Mip.BulkData.Unlock();
						m_ColorTextures[i]->UpdateResource();
					}

					i++;
				}
		}
	}
}


FPrimitiveSceneProxy* UcTerraformaLandscapeComponent::CreateSceneProxy()
{
	return new FcTerraformaMeshSceneProxy(this);;
}

int32 UcTerraformaLandscapeComponent::GetNumMaterials() const
{
	return 1;
}


FBoxSphereBounds UcTerraformaLandscapeComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	// Minimum Vector: It's set to the first vertex's position initially (NULL == FVector::ZeroVector might be required and a known vertex vector has intrinsically valid values)
	FVector vecMin(0,0,0);

	// Maximum Vector: It's set to the first vertex's position initially (NULL == FVector::ZeroVector might be required and a known vertex vector has intrinsically valid values)
	FVector vecMax(CHUNK_SIZE_CM*64, CHUNK_SIZE_CM * 64, CHUNK_SIZE_CM * 64);
	
	FVector vecOrigin = ((vecMax - vecMin) / 2) + vecMin;	/* Origin = ((Max Vertex's Vector - Min Vertex's Vector) / 2 ) + Min Vertex's Vector */
	FVector BoxPoint = vecMax - vecMin;			/* The difference between the "Maximum Vertex" and the "Minimum Vertex" is our actual Bounds Box */
	return FBoxSphereBounds(vecOrigin, BoxPoint, BoxPoint.Size()).TransformBy(LocalToWorld);
}


UBodySetup* UcTerraformaLandscapeComponent::GetBodySetup() {
	if (ModelBodySetup == NULL) {
		ModelBodySetup = NewObject<UBodySetup>(this);
		ModelBodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
		ModelBodySetup->bMeshCollideAll = true;
	}
	return ModelBodySetup;
}

void UcTerraformaLandscapeComponent::loadLevel() {
	if (GConfig) {

		//Retrieve Default Game Type
		FString PathToWorlds;
		GConfig->GetString(
			TEXT("/Script/EngineSettings.Perimeter.Worlds"),
			TEXT("Path"),
			PathToWorlds,
			GGameIni
		);

		FString tfaFileName = PathToWorlds + "\\" + VMPWorld.ToString() + "\\" + VMPWorld.ToString() + ".tfa";
		m_Landscape.convertVMPtoTFA(PathToWorlds + "\\" + VMPWorld.ToString(), tfaFileName);
		m_Landscape.loadFromFile(tfaFileName);
		ReinitMaterials();
	}
}

#if WITH_EDITOR  
void UcTerraformaLandscapeComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	//Get the name of the property that was changed  
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == "VMPWorld") {
		loadLevel();
	}
	// Call the base class version  
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif