// Fill out your copyright notice in the Description page of Project Settings.

#include "cTerraformaLandscapeComponent.h"
#include "ConstructorHelpers.h"




inline int getVertexIndex(int x, int y, int step = 1) {
	return x*step + y*step*(CHUNK_RESOLUTION + 1);
}

void GenerateChunk(int step, sTerraformaChunkLOD& chunk, FTerraformaMeshIndexBuffer& indexBuffer) {	
	int Resolution = CHUNK_RESOLUTION / step;
	chunk.bodyFull.start = indexBuffer.Indices.Num();
	chunk.bodyFull.trianglesCount = 0;
	for (int y = 0; y<Resolution; y++)
		for (int x = 0; x < Resolution; x++) {
			int indexLT = getVertexIndex(x, y, step);
			int indexRT = getVertexIndex(x+1, y, step);
			int indexLB = getVertexIndex(x, y+1, step);
			int indexRB = getVertexIndex(x + 1, y+1, step);
			
			
			indexBuffer.Indices.Add(indexLB);
			indexBuffer.Indices.Add(indexRT);
			indexBuffer.Indices.Add(indexLT);

			indexBuffer.Indices.Add(indexRB);			
			indexBuffer.Indices.Add(indexRT);
			indexBuffer.Indices.Add(indexLB);
			
			chunk.bodyFull.trianglesCount += 2;
		}
}



FTerraformaMeshSceneProxy::FTerraformaMeshSceneProxy(UcTerraformaLandscapeComponent* Component)
		: FPrimitiveSceneProxy(Component)
		, MaterialRelevance(Component->GetMaterialRelevance(ERHIFeatureLevel::SM4)) // Feature level defined by the capabilities of DX10 Shader Model 4.
	{
		m_LandscapeComponent = Component;		

		Material = Component->GetMaterial(0);

		const FVector TangentX(1, 0, 0);
		const FVector TangentY(0, 1, 0);
		const FVector TangentZ(0, 0, 1);		

		for (int y = 0; y<=CHUNK_RESOLUTION; y++)
			for (int x = 0; x <= CHUNK_RESOLUTION; x++) {
				FDynamicMeshVertex vertex;
				vertex.Position = FVector(CHUNK_SIZE_CM* x / (float)(CHUNK_RESOLUTION), CHUNK_SIZE_CM* y / (float)(CHUNK_RESOLUTION), 0);
				vertex.TextureCoordinate = FVector2D(x*CHUNK_TEXTURE_STEP+CHUNK_TEXTURE_STEP*1.5, y*CHUNK_TEXTURE_STEP + CHUNK_TEXTURE_STEP*1.5); //shift borders and set uv on texel center
				vertex.SetTangents(TangentX, TangentY, TangentZ);
				VertexBuffer.Vertices.Add(vertex);
			}
		int step = 1;
		for (int i = 0; i < (int)eChunkLOD::COUNT; i++) {
			GenerateChunk(step, m_LODS[i], IndexBuffer);
			step*=2;
		}

		// Init vertex factory
		VertexFactory.Init(&VertexBuffer);

		// Enqueue initialization of render resource
		BeginInitResource(&VertexBuffer);
		BeginInitResource(&IndexBuffer);
		BeginInitResource(&VertexFactory);

		m_Width = 0;
		m_Height = 0;
		updateChunksState(true);
	}

	FTerraformaMeshSceneProxy::~FTerraformaMeshSceneProxy()
	{
		VertexBuffer.ReleaseResource();
		IndexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();
	}

	void FTerraformaMeshSceneProxy::updateChunksState(bool forceReinit) {
		if (m_Width == m_LandscapeComponent->m_Landscape.width() && m_Height == m_LandscapeComponent->m_Landscape.height() && !forceReinit)
			return;
		m_NeedInvalidate = true;
		m_Width = m_LandscapeComponent->m_Landscape.width();
		m_Height = m_LandscapeComponent->m_Landscape.height();
		m_LandscapeComponent->reinitMaterials(Material, m_Width,m_Height);

		sChunkInfo chunk;
		chunk.Mesh.VertexFactory = &VertexFactory;
		chunk.Mesh.ReverseCulling = false;
		chunk.Mesh.Type = PT_TriangleList;
		chunk.Mesh.DepthPriorityGroup = SDPG_World;
		chunk.Mesh.bCanApplyViewModeOverrides = false;
		chunk.Mesh.bWireframe = false;
		
		m_Chunks.Init(chunk, m_Width*m_Height);

		FMatrix MeshTransform;
		MeshTransform.SetIdentity();
		int i = 0;
		for (int y = 0; y<m_Height; y++)
			for (int x = 0; x < m_Width; x++) {
				m_Chunks[i].Data = m_LandscapeComponent->m_Landscape.getData(x, y);

				FMeshBatch& Mesh = m_Chunks[i].Mesh; 
				m_Chunks[i].MaterialInst = m_LandscapeComponent->m_DynMaterialInstances[i];
				Mesh.MaterialRenderProxy = m_Chunks[i].MaterialInst->GetRenderProxy(false);
				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.IndexBuffer = &IndexBuffer;
				MeshTransform.SetOrigin(FVector(x*CHUNK_SIZE_CM, y*CHUNK_SIZE_CM, 0));
				m_Chunks[i].Transform = MeshTransform;				
				BatchElement.FirstIndex = m_LODS[(int)eChunkLOD::LOD_1].bodyFull.start;
				BatchElement.NumPrimitives = m_LODS[(int)eChunkLOD::LOD_1].bodyFull.trianglesCount;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = VertexBuffer.Vertices.Num() - 1;	

				i++;
			}
	}

	void FTerraformaMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const {
		//
	}

	

	void FTerraformaMeshSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) {
		if (m_Chunks.Num() != m_LandscapeComponent->m_Landscape.width()*m_LandscapeComponent->m_Landscape.height()) {
			if (IsInGameThread()) {
				ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(TerraformaUpdate, FTerraformaMeshSceneProxy*, terraformaProxy, this,
				{
					terraformaProxy->updateChunksState(true);
				});
			}
			else {
				updateChunksState(true);
			}
			m_LandscapeComponent->m_NeedInvalidate = true;
			return;
		}

		if (m_Chunks.Num() == 0)
			return;

		/*
		if (m_LandscapeComponent->m_DynMaterialInstances[m_LandscapeComponent->m_DynMaterialInstances.Num()-1] == nullptr) {
			ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER_DECLARE(TerraformaUpdate, FTerraformaMeshSceneProxy*, terraformaProxy, this,
			{
				terraformaProxy->updateChunksState(true);
			});
			//NEED INVALIDATE
			return;
		}
		*/
		
		for (int i = 0; i < m_Chunks.Num(); i++) {
			sChunkInfo& chunk = (sChunkInfo&)m_Chunks[i];
			FMeshBatch &Mesh = chunk.Mesh;
			m_Chunks[i].MaterialInst = m_LandscapeComponent->m_DynMaterialInstances[i];
			Mesh.MaterialRenderProxy = m_Chunks[i].MaterialInst->GetRenderProxy(false);
			Mesh.Elements[0].PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(chunk.Transform, GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
			PDI->DrawMesh(Mesh, FLT_MAX);
		}
		m_NeedInvalidate = false;
	}	
	
	FPrimitiveViewRelevance FTerraformaMeshSceneProxy::GetViewRelevance(const FSceneView* View) const
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bStaticRelevance = true;
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	bool FTerraformaMeshSceneProxy::CanBeOccluded() const 
	{
		return !MaterialRelevance.bDisableDepthTest;
	}

	 uint32 FTerraformaMeshSceneProxy::GetMemoryFootprint(void) const { return(sizeof(*this) + GetAllocatedSize()); }

	uint32 FTerraformaMeshSceneProxy::GetAllocatedSize(void) const { return(FPrimitiveSceneProxy::GetAllocatedSize()); }













void UcTerraformaLandscapeComponent::reinitMaterials(UMaterialInterface* baseMaterial, int width, int height) {
	
	m_HeightmapTextures.SetNum(width*height);
	m_ColorTextures.SetNum(width*height);
	m_DynMaterialInstances.SetNum(width*height);

	for (int i = 0; i < width*height; i++) {
		sTerraformaGridChunk* chunk = m_Landscape.getData(i);

		m_HeightmapTextures[i] = UTexture2D::CreateTransient(CHUNK_RESOLUTION + 2, CHUNK_RESOLUTION + 2, PF_R8G8);// PF_R16_UINT);
		m_HeightmapTextures[i]->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
		m_HeightmapTextures[i]->SRGB = 0;
		m_HeightmapTextures[i]->Filter = TextureFilter::TF_Nearest;//  ESamplerFilter::SF_Point;
		

		FTexture2DMipMap& MipH = m_HeightmapTextures[i]->PlatformData->Mips[0];
		void* DataH = MipH.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(DataH, chunk->dynDataHeightMap, (CHUNK_RESOLUTION + 2)* (CHUNK_RESOLUTION + 2) * sizeof(sChunkHeightmapData));
		MipH.BulkData.Unlock();
		m_HeightmapTextures[i]->UpdateResource();
		chunk->HeightmapChanged = false;

		m_ColorTextures[i] = UTexture2D::CreateTransient(CHUNK_RESOLUTION + 2, CHUNK_RESOLUTION + 2, PF_R8G8B8A8);
		m_ColorTextures[i]->MipGenSettings = TextureMipGenSettings::TMGS_NoMipmaps;
		m_ColorTextures[i]->Filter = TextureFilter::TF_Trilinear;//  ESamplerFilter::SF_Point;
		FTexture2DMipMap& MipT = m_ColorTextures[i]->PlatformData->Mips[0];
		void* DataT = MipT.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(DataT, chunk->dynDataTexture, (CHUNK_RESOLUTION + 2)* (CHUNK_RESOLUTION + 2) * sizeof(sChunkTextureData));
		MipT.BulkData.Unlock();
		m_ColorTextures[i]->UpdateResource();
		chunk->TextureChanged = false;

		//if (IsInGameThread()) {
			m_DynMaterialInstances[i] = UMaterialInstanceDynamic::Create(baseMaterial, this);
			m_DynMaterialInstances[i]->SetTextureParameterValue(FName("PHeightmap"), m_HeightmapTextures[i]);
			m_DynMaterialInstances[i]->SetTextureParameterValue(FName("PTexture"), m_ColorTextures[i]);
		//}
		m_NeedInvalidate = false;
	}
}

UcTerraformaLandscapeComponent::UcTerraformaLandscapeComponent():UMeshComponent() {
	PrimaryComponentTick.bCanEverTick = true;	

	static ConstructorHelpers::FObjectFinder<UMaterial> materialCH(TEXT("Material'/Game/LandscapeTechData/M_Terraforma.M_Terraforma'"));
	SetMaterial(0, materialCH.Object);
}

void UcTerraformaLandscapeComponent::OnRegister() {
	loadLevel();
	Super::OnRegister();
}


/*
void UpdateTextureRegions(UTexture2D* Texture, int32 MipIndex, uint32 NumRegions, FUpdateTextureRegion2D* Regions, uint32 SrcPitch, uint32 SrcBpp, uint8* SrcData, bool bFreeData)
{
	if (Texture->Resource)
	{
		struct FUpdateTextureRegionsData
		{
			FTexture2DResource* Texture2DResource;
			int32 MipIndex;
			uint32 NumRegions;
			FUpdateTextureRegion2D* Regions;
			uint32 SrcPitch;
			uint32 SrcBpp;
			uint8* SrcData;
		};

		FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;

		RegionData->Texture2DResource = (FTexture2DResource*)Texture->Resource;
		RegionData->MipIndex = MipIndex;
		RegionData->NumRegions = NumRegions;
		RegionData->Regions = Regions;
		RegionData->SrcPitch = SrcPitch;
		RegionData->SrcBpp = SrcBpp;
		RegionData->SrcData = SrcData;

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			UpdateTextureRegionsData,
			FUpdateTextureRegionsData*, RegionData, RegionData,
			bool, bFreeData, bFreeData,
			{
				for (uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex)
				{
					int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
					if (RegionData->MipIndex >= CurrentFirstMip)
					{
						RHIUpdateTexture2D(
							RegionData->Texture2DResource->GetTexture2DRHI(),
							RegionData->MipIndex - CurrentFirstMip,
							RegionData->Regions[RegionIndex],
							RegionData->SrcPitch,
							RegionData->SrcData
							+ RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch
							+ RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp
						);
					}
				}
		if (bFreeData)
		{
			FMemory::Free(RegionData->Regions);
			FMemory::Free(RegionData->SrcData);
		}
		delete RegionData;
			});
	}
}*/

void UcTerraformaLandscapeComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) {
	if (m_Landscape.isDataChanged() && m_HeightmapTextures.Num()== m_Landscape.width()*m_Landscape.height() && m_ColorTextures.Num() == m_Landscape.width()*m_Landscape.height()) {
		int i = 0;
		for (int y = 0;  y<m_Landscape.height(); y++)
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

		m_Landscape.resetDataChangedFlag();
	}
}


FPrimitiveSceneProxy* UcTerraformaLandscapeComponent::CreateSceneProxy()
{
	m_Proxy = new FTerraformaMeshSceneProxy(this);;
	return m_Proxy;
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