// Fill out your copyright notice in the Description page of Project Settings.

#include "cTerraformaLandscapeComponent.h"
#include "ConstructorHelpers.h"
#include "cTerraformaUtilsLibrary.h"









void UcTerraformaLandscapeComponent::ReinitMaterials() {
	int width = m_Landscape.width();
	int height = m_Landscape.height();
	UMaterialInterface* baseMaterial = GetMaterial(0);
	
	if (MaterialsReadyCount < width*height) {
		m_HeightmapTextures.SetNum(width*height);
		m_NormalmapTextures.SetNum(width*height);
		m_ColorTextures.SetNum(width*height);
		m_DynMaterialInstances.SetNum(width*height);
		m_Bounds.SetNum(width*height);
	}

	for (int i = 0; i < width*height; i++) {
		sTerraformaGridChunk* chunk = m_Landscape.getData(i);

		if (i >= MaterialsReadyCount || m_DynMaterialInstances[i]->Parent!= baseMaterial) {

			if (i >= MaterialsReadyCount) {
				m_HeightmapTextures[i] = UTexture2D::CreateTransient(CHUNK_RESOLUTION + 2, CHUNK_RESOLUTION + 2, PF_R8G8);// PF_R16_UINT);
				//m_HeightmapTextures[i]->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
				m_HeightmapTextures[i]->SRGB = 0;
				m_HeightmapTextures[i]->Filter = TextureFilter::TF_Nearest;

				m_NormalmapTextures[i] = UTexture2D::CreateTransient(CHUNK_RESOLUTION + 2, CHUNK_RESOLUTION + 2, PF_R8G8B8A8);
				m_NormalmapTextures[i]->SRGB = 0;
				m_NormalmapTextures[i]->LODGroup = TEXTUREGROUP_WorldNormalMap;
				m_NormalmapTextures[i]->Filter = TextureFilter::TF_Nearest;
				
				m_ColorTextures[i] = UTexture2D::CreateTransient(CHUNK_RESOLUTION + 2, CHUNK_RESOLUTION + 2, PF_R8G8B8A8);
				//m_ColorTextures[i]->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
				m_ColorTextures[i]->Filter = TextureFilter::TF_Trilinear;
			}

			m_DynMaterialInstances[i] = UMaterialInstanceDynamic::Create(baseMaterial, this);
			m_DynMaterialInstances[i]->SetTextureParameterValue(FName("PHeightmap"), m_HeightmapTextures[i]);
			m_DynMaterialInstances[i]->SetTextureParameterValue(FName("PNormalmap"), m_NormalmapTextures[i]);
			m_DynMaterialInstances[i]->SetTextureParameterValue(FName("PTexture"), m_ColorTextures[i]);
		}
		

		FTexture2DMipMap& MipH = m_HeightmapTextures[i]->PlatformData->Mips[0];
		void* DataH = MipH.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(DataH, chunk->dynDataHeightMap, (CHUNK_RESOLUTION + 2)* (CHUNK_RESOLUTION + 2) * sizeof(sChunkHeightData));
		MipH.BulkData.Unlock();
		m_HeightmapTextures[i]->UpdateResource();

		FTexture2DMipMap& MipN = m_NormalmapTextures[i]->PlatformData->Mips[0];
		void* DataN = MipN.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(DataN, chunk->dynDataNormalMap, (CHUNK_RESOLUTION + 2)* (CHUNK_RESOLUTION + 2) * sizeof(sChunkNormalData));
		MipN.BulkData.Unlock();
		m_NormalmapTextures[i]->UpdateResource();
		chunk->HeightmapChanged = false;

		
		FTexture2DMipMap& MipT = m_ColorTextures[i]->PlatformData->Mips[0];
		void* DataT = MipT.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(DataT, chunk->dynDataTexture, (CHUNK_RESOLUTION + 2)* (CHUNK_RESOLUTION + 2) * sizeof(sChunkTextureData));
		MipT.BulkData.Unlock();
		m_ColorTextures[i]->UpdateResource();
		chunk->TextureChanged = false;

		m_Bounds[i] = FBoxSphereBounds(FBox(FVector(chunk->x*CHUNK_SIZE_CM, chunk->y*CHUNK_SIZE_CM,0), FVector((chunk->x+1)*CHUNK_SIZE_CM, (chunk->y + 1)*CHUNK_SIZE_CM, 256*25)));
	}

	MaterialsReadyCount = width*height;
}

UcTerraformaLandscapeComponent::UcTerraformaLandscapeComponent():UMeshComponent() {
	PrimaryComponentTick.bCanEverTick = true;	

	static ConstructorHelpers::FObjectFinder<UMaterial> materialCH(TEXT("Material'/Game/LandscapeTechData/M_Terraforma.M_Terraforma'"));
	SetMaterial(0, materialCH.Object);

	ApplyTerraformingDelay = 0.2;
	TextureChangeSpeed = 0.01;
	
		
	FsTerraformaTemplate TerraformedTextureTemplate;

	m_HeightmapTextures.SetNum(0);
	m_NormalmapTextures.SetNum(0);
	m_ColorTextures.SetNum(0);
	m_DynMaterialInstances.SetNum(0);
	MaterialsReadyCount = 0;
	m_ApplyTerraformingCounter = 0;
	m_Terraformed = false;
	Wireframe = false;
}

void UcTerraformaLandscapeComponent::OnRegister() {
	loadLevel();
	Super::OnRegister();
}

void UcTerraformaLandscapeComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) {
	if (GEngine) {
		UWorld* world = GetWorld();
		if (world) {
			APlayerController* pc = GEngine->GetFirstLocalPlayerController(world);
			if (pc) {
				APlayerCameraManager* cameraManager = pc->PlayerCameraManager;
				if (cameraManager) {
					if (m_CameraGrid.recalcVisibleItems(cameraManager, FVector(0, 0, 0), m_Landscape.width(), m_Landscape.height(), 5000))
						MarkRenderStateDirty();
				}
			}
		}
	}


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
						FMemory::Memcpy(DataH, chunk->dynDataHeightMap, (CHUNK_RESOLUTION + 2)* (CHUNK_RESOLUTION + 2) * sizeof(sChunkHeightData));
						MipH.BulkData.Unlock();
						m_HeightmapTextures[i]->UpdateResource();

						FTexture2DMipMap& MipN = m_NormalmapTextures[i]->PlatformData->Mips[0];
						void* DataN = MipN.BulkData.Lock(LOCK_READ_WRITE);
						FMemory::Memcpy(DataN, chunk->dynDataNormalMap, (CHUNK_RESOLUTION + 2)* (CHUNK_RESOLUTION + 2) * sizeof(sChunkNormalData));
						MipN.BulkData.Unlock();
						m_NormalmapTextures[i]->UpdateResource();
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

void UcTerraformaLandscapeComponent::BeginPlay() {
	Super::BeginPlay();
	if (TerraformedTexture != nullptr)
		UcTerraformaUtilsLibrary::GetColorMapTemplateFromTexture(TerraformedTexture, TerraformedTextureTemplate, GEngine);

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
	int w = m_Landscape.width();
	int h = m_Landscape.height();
	if (w == 0) {
		w = 64;
		h = 64;
	}
	FVector vecMax(CHUNK_SIZE_CM*m_Landscape.width(), CHUNK_SIZE_CM * m_Landscape.height(), MAX_HEIGHT_CM);
	
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
	
	if (TerraformaWorld != NULL) {
		m_Landscape.loadFromArray(TerraformaWorld->TFAFileData);
		ReinitMaterials();
		MarkRenderStateDirty();
	}
	/**/
	/*
	if (GConfig) {

		//Retrieve Default Game Type
		FString PathToWorlds;
		GConfig->GetString(
			TEXT("/Script/EngineSettings.Perimeter.Worlds"),
			TEXT("Path"),
			PathToWorlds,
			GGameIni
		);

		FString tfaFileName = PathToWorlds + "\\" + "ABDULA" + "\\" + "ABDULA" + ".tfa";
		m_Landscape.convertVMPtoTFA(PathToWorlds + "\\" + "ABDULA", tfaFileName);
		m_Landscape.loadFromFile(tfaFileName);
		ReinitMaterials();
	}/**/
}

#if WITH_EDITOR  
void UcTerraformaLandscapeComponent::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	//Get the name of the property that was changed  
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == "TerraformaWorld") {
		loadLevel();
	}
	// Call the base class version  
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

bool UcTerraformaLandscapeComponent::LineIntersection(FVector start, FVector direction, FVector& intersectionLocation) {
	FVector landscapePos(0, 0, 0);
	FVector landscapeSize(m_Landscape.width()*CHUNK_SIZE_CM, m_Landscape.height()*CHUNK_SIZE_CM, 5000);

	if (start.Z <= 0)
		return false;

	if (FMath::Abs((int)(direction.X / TEXEL_SIZE_CM)) == 0 && FMath::Abs((int)(direction.Y / TEXEL_SIZE_CM)) == 0) {
		if (direction.Z > 0 || start.X < 0 || start.X >= landscapeSize.X || start.Y < 0 || start.Y >= landscapeSize.Y)
			return false;

		FIntPoint pos((int)(start.X / TEXEL_SIZE_CM), (int)(start.Y / TEXEL_SIZE_CM));
		float height = (float)m_Landscape.getHeightDirect(pos.X, pos.Y)* MAX_HEIGHT_CM / MAX_uint16 ;
		if (height > (start + direction).Z) {
			intersectionLocation = start + direction;
			intersectionLocation.Z = height;
			return true;
		}
	}
	
	int stepsCount = (int)(FMath::Sqrt(FMath::Square(direction.X) + FMath::Square(direction.Y)) / TEXEL_SIZE_CM);
	float stepSize = direction.Size() / stepsCount;

	direction.Normalize();

	int i = 0;
	//skip vector while not inside landscape
	while (i < stepsCount) {
		FVector newPos = start+direction * (i* stepSize);
		if (newPos.X >= 0 && newPos.X < landscapeSize.X && newPos.Y >= 0 && newPos.Y < landscapeSize.Y && newPos.Z >= 0 && newPos.Z < landscapeSize.Z)
			break;

		i++;
	}
	while (i < stepsCount) {
		FVector newPos = start+direction * (i* stepSize);		
		if (newPos.X < 0 || newPos.X >= landscapeSize.X || newPos.Y < 0 || newPos.Y >= landscapeSize.Y || newPos.Z >= landscapeSize.Z)
			return false;
		if (newPos.Z < 0) {
			intersectionLocation = newPos;
			intersectionLocation.Z = 0;
			return true;
		}

		FIntPoint point(newPos.X / TEXEL_SIZE_CM, newPos.Y / TEXEL_SIZE_CM);
		float height = (float)m_Landscape.getHeightDirect(point.X, point.Y)* MAX_HEIGHT_CM / MAX_uint16 ;

		if (height >(newPos).Z) {
			intersectionLocation = newPos;
			intersectionLocation.Z = height;
			return true;
		}
		
		i++;
	}
	
	return false;
}

int UcTerraformaLandscapeComponent::ApplyTerraforming(FVector Position, const FsTerraformaTemplate& cutHeightmap, const FsTerraformaTemplate& heightmap, uint8 heightmapFactor, const FsTerraformaTemplate& colormap) {
	bool haveColorMap = colormap.Size > 0 && colormap.Type == ETemplateTypeEnum::TTE_TEXTURE;
	FsTerraformaTemplate* terraformedTemplate = nullptr;
	if (!haveColorMap && TerraformedTextureTemplate.Size > 0 && TerraformedTextureTemplate.Type == ETemplateTypeEnum::TTE_TEXTURE)
		terraformedTemplate = &TerraformedTextureTemplate;
	colormap.RawData.Num() > 0 && colormap.Type == ETemplateTypeEnum::TTE_TEXTURE;
	int TerraformedCounter = 0;
	if (cutHeightmap.RawData.Num() > 0 && cutHeightmap.Type == ETemplateTypeEnum::TTE_HEIGHTMAP) {
		TerraformedCounter += m_Landscape.ApplyCutTerraforming(Position.X / TEXEL_SIZE_CM, Position.Y / TEXEL_SIZE_CM, (uint16)(Position.Z * MAX_uint16 / MAX_HEIGHT_CM), cutHeightmap, heightmapFactor);
	}
	if (heightmap.RawData.Num() > 0 && heightmap.Type==ETemplateTypeEnum::TTE_HEIGHTMAP) {
		TerraformedCounter+= m_Landscape.ApplyTerraforming(Position.X / TEXEL_SIZE_CM, Position.Y / TEXEL_SIZE_CM, heightmap, heightmapFactor, terraformedTemplate, TextureChangeSpeed);
	}
	if (haveColorMap) {
		m_Landscape.ApplyColoring(Position.X / TEXEL_SIZE_CM, Position.Y / TEXEL_SIZE_CM, colormap);
	}
	m_Terraformed = true;
	return TerraformedCounter;
}

