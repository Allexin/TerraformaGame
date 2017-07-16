#pragma once
#include "Components/MeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "DynamicMeshBuilder.h"
#include "cTerraformaGrid.h"
#include "cCameraGrid.h"

struct FTerraformaMeshVertex
{
	FTerraformaMeshVertex() {}

	FVector Position;
	FVector2D TextureCoordinate;
	FVector2D TextureCoordinate2;
	FPackedNormal TangentX;
	FPackedNormal TangentZ;

	void SetTangents(const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ)
	{
		TangentX = InTangentX;
		TangentZ = InTangentZ;
		// store determinant of basis in w component of normal vector
		TangentZ.Vector.W = GetBasisDeterminantSign(InTangentX, InTangentY, InTangentZ) < 0.0f ? 0 : 255;
	}	
}; 

class FTerraformaMeshVertexBuffer : public FVertexBuffer
{
public:
	TArray<FTerraformaMeshVertex> Vertices;

	virtual void InitRHI()
	{
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.Num() * sizeof(FTerraformaMeshVertex), BUF_Static, CreateInfo);

		// Copy the vertex data into the vertex buffer.
		void* VertexBufferData = RHILockVertexBuffer(VertexBufferRHI, 0, Vertices.Num() * sizeof(FTerraformaMeshVertex), RLM_WriteOnly);
		FMemory::Memcpy(VertexBufferData, Vertices.GetData(), Vertices.Num() * sizeof(FTerraformaMeshVertex));
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
				FDataType NewData;
		NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FTerraformaMeshVertex,Position,VET_Float3);
		NewData.TextureCoordinates.Add(
			FVertexStreamComponent(VertexBuffer,STRUCT_OFFSET(FTerraformaMeshVertex,TextureCoordinate),sizeof(FTerraformaMeshVertex),VET_Float2));
		NewData.TextureCoordinates.Add(
			FVertexStreamComponent(VertexBuffer, STRUCT_OFFSET(FTerraformaMeshVertex, TextureCoordinate2), sizeof(FTerraformaMeshVertex), VET_Float2));
		NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FTerraformaMeshVertex, TangentX, VET_PackedNormal);
		NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FTerraformaMeshVertex, TangentZ, VET_PackedNormal);
		VertexFactory->SetData(NewData);
			});
	}
};

struct sIndexRange {
	int start;
	int trianglesCount;
	sIndexRange() {}
};

struct sTerraformaChunkLOD {
	sIndexRange variants[SEAM_COUNT];
	sTerraformaChunkLOD() {}
};

struct sChunkInfo {
	FMeshBatch Mesh;
	FMatrix Transform;	
	sTerraformaGridChunk* Data;
	FBoxSphereBounds MeshBound;

	UMaterialInstanceDynamic* MaterialInst;
};

class UcTerraformaLandscapeComponent;

class FcTerraformaMeshSceneProxy : public FPrimitiveSceneProxy
{
protected:
	sTerraformaChunkLOD m_LODS[eChunkLOD::COUNT];
	TArray<sChunkInfo> m_Chunks;
	int m_ChunksCount;
	int m_Width;
	int m_Height;
	UcTerraformaLandscapeComponent* m_LandscapeComponent;
	void ReinitChunks();
public:

	FcTerraformaMeshSceneProxy(UcTerraformaLandscapeComponent* Component);
	virtual ~FcTerraformaMeshSceneProxy();

	
	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const;
	
	virtual bool CanBeOccluded() const override { return !MaterialRelevance.bDisableDepthTest;}
	virtual uint32 GetMemoryFootprint(void) const override { return(sizeof(*this) + GetAllocatedSize()); }
	uint32 GetAllocatedSize(void) const { return(FPrimitiveSceneProxy::GetAllocatedSize()); }
	
private:
	UMaterialInterface* Material;
	FTerraformaMeshVertexBuffer VertexBuffer;
	FTerraformaMeshIndexBuffer IndexBuffer;
	FTerraformaMeshVertexFactory VertexFactory;

	FMaterialRelevance MaterialRelevance;
};