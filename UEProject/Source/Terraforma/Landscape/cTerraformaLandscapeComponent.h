// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "DynamicMeshBuilder.h"
#include "cTerraformaGrid.h"
#include "cTerraformaLandscapeComponent.generated.h"

/**
 * 
 */




class FTerraformaMeshVertexBuffer : public FVertexBuffer
{
public:
	TArray<FDynamicMeshVertex> Vertices;

	virtual void InitRHI()
	{
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.Num() * sizeof(FDynamicMeshVertex), BUF_Static, CreateInfo);

		// Copy the vertex data into the vertex buffer.
		void* VertexBufferData = RHILockVertexBuffer(VertexBufferRHI, 0, Vertices.Num() * sizeof(FDynamicMeshVertex), RLM_WriteOnly);
		FMemory::Memcpy(VertexBufferData, Vertices.GetData(), Vertices.Num() * sizeof(FDynamicMeshVertex));
		RHIUnlockVertexBuffer(VertexBufferRHI);

	}

};

/** Index Buffer */
class FTerraformaMeshIndexBuffer : public FIndexBuffer
{
public:
	TArray<int32> Indices;

	virtual void InitRHI()
	{
		FRHIResourceCreateInfo CreateInfo;
		IndexBufferRHI = RHICreateIndexBuffer(sizeof(int32), Indices.Num() * sizeof(int32), BUF_Static, CreateInfo);
		// Write the indices to the index buffer.
		void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, Indices.Num() * sizeof(int32), RLM_WriteOnly);
		FMemory::Memcpy(Buffer, Indices.GetData(), Indices.Num() * sizeof(int32));
		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
};

/** Vertex Factory */
class FTerraformaMeshVertexFactory : public FLocalVertexFactory
{
public:

	FTerraformaMeshVertexFactory()
	{}


	/** Initialization */
	void Init(const FTerraformaMeshVertexBuffer* VertexBuffer)
	{
		check(!IsInRenderingThread());

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			InitTerraformaMeshVertexFactory,
			FTerraformaMeshVertexFactory*, VertexFactory, this,
			const FTerraformaMeshVertexBuffer*, VertexBuffer, VertexBuffer,
			{
				// Initialize the vertex factory's stream components.
				DataType NewData;
		NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,Position,VET_Float3);
		NewData.TextureCoordinates.Add(
			FVertexStreamComponent(VertexBuffer,STRUCT_OFFSET(FDynamicMeshVertex,TextureCoordinate),sizeof(FDynamicMeshVertex),VET_Float2));
		NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,TangentX,VET_PackedNormal);
		NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer,FDynamicMeshVertex,TangentZ,VET_PackedNormal);
		NewData.ColorComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, Color, VET_Color);
		VertexFactory->SetData(NewData);
			});
	}
};

struct sIndexRange {
	int start;
	int trianglesCount;
	sIndexRange() {}
};

enum class eSeamSide {
	MINUS_Y = 0,
	PLUS_Y,
	MINUS_X,
	PLUS_C,
	COUNT
};

struct sTerraformaChunkLOD {
	sIndexRange bodyFull;
	sIndexRange bodyForSeam;
	sIndexRange seams[eSeamSide::COUNT];
	sTerraformaChunkLOD() {}
};

enum class eChunkLOD {
	LOD_0 = 0,
	LOD_1,
	LOD_2,
	LOD_3,
	LOD_4,
	LOD_5,

	COUNT
};

struct sChunkInfo {
	FMeshBatch Mesh;
	FMatrix Transform;
	sTerraformaGridChunk* Data;

	UMaterialInstanceDynamic* MaterialInst;
};

class UcTerraformaLandscapeComponent;

class FTerraformaMeshSceneProxy : public FPrimitiveSceneProxy
{
protected:	
	sTerraformaChunkLOD m_LODS[eChunkLOD::COUNT];
	TArray<sChunkInfo> m_Chunks;
	int m_Width;
	int m_Height;
	UcTerraformaLandscapeComponent* m_LandscapeComponent;
	bool m_NeedInvalidate;
public:

	FTerraformaMeshSceneProxy(UcTerraformaLandscapeComponent* Component);
	virtual ~FTerraformaMeshSceneProxy();

	void updateChunksState(bool forceReinit);
	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const;
	virtual bool CanBeOccluded() const override;
	virtual uint32 GetMemoryFootprint(void) const;

	uint32 GetAllocatedSize(void) const;

	bool needInvalidate() { return m_NeedInvalidate; }
private:
	UMaterialInterface* Material;
	FTerraformaMeshVertexBuffer VertexBuffer;
	FTerraformaMeshIndexBuffer IndexBuffer;
	FTerraformaMeshVertexFactory VertexFactory;

	FMaterialRelevance MaterialRelevance;
};


UCLASS()
class TERRAFORMA_API UcTerraformaLandscapeComponent : public UMeshComponent
{
	GENERATED_BODY()
	friend class FTerraformaMeshSceneProxy;
protected:
	FTerraformaMeshSceneProxy*			m_Proxy;
	cTerraformaGrid						m_Landscape;
	
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*>	m_DynMaterialInstances;
	UPROPERTY()
	TArray<UTexture2D*>	m_HeightmapTextures;
	UPROPERTY()
	TArray<UTexture2D*>	m_ColorTextures;

	bool m_NeedInvalidate;

	void reinitMaterials(UMaterialInterface* baseMaterial, int width, int height);
	void loadLevel();
public:
	UPROPERTY(EditAnywhere, Category = "Landscape")
		FName VMPWorld;
public:
	UcTerraformaLandscapeComponent();

	bool needInvalidate() { return m_Proxy->needInvalidate() || m_NeedInvalidate; } //TODO - or m_Landscape NeedInvalidate

	/** Description of collision */
	UPROPERTY(BlueprintReadOnly, Category = "Collision")
		class UBodySetup* ModelBodySetup;

	// Begin UMeshComponent interface.
	virtual int32 GetNumMaterials() const override;
	// End UMeshComponent interface.

	// Begin UPrimitiveComponent interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual class UBodySetup* GetBodySetup() override;
	/**
	* Initializes the component.  Occurs at level startup. This is before BeginPlay (Actor or Component).
	* All Components in the level will be Initialized on load before any Actor/Component gets BeginPlay
	* Requires component to be registered, and bWantsInitializeComponent to be true.
	*/
	virtual void OnRegister() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	// End UPrimitiveComponent interface.

	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
private:
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;

};
