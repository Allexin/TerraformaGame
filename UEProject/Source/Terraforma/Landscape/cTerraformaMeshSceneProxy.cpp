#include "cTerraformaMeshSceneProxy.h"
#include "cTerraformaLandscapeComponent.h"

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
			int indexRT = getVertexIndex(x + 1, y, step);
			int indexLB = getVertexIndex(x, y + 1, step);
			int indexRB = getVertexIndex(x + 1, y + 1, step);


			indexBuffer.Indices.Add(indexLB);
			indexBuffer.Indices.Add(indexRT);
			indexBuffer.Indices.Add(indexLT);

			indexBuffer.Indices.Add(indexRB);
			indexBuffer.Indices.Add(indexRT);
			indexBuffer.Indices.Add(indexLB);

			chunk.bodyFull.trianglesCount += 2;
		}

	//TODO - generate LODS with seams
}



FcTerraformaMeshSceneProxy::FcTerraformaMeshSceneProxy(UcTerraformaLandscapeComponent* Component)
	: FPrimitiveSceneProxy(Component)
	, MaterialRelevance(Component->GetMaterialRelevance(ERHIFeatureLevel::SM4)) // Feature level defined by the capabilities of DX10 Shader Model 4.
{
	m_LandscapeComponent = Component;

	Material = Component->GetMaterial(0);

	const FVector TangentX(1, 0, 0);
	const FVector TangentY(0, 1, 0);
	const FVector TangentZ(0, 0, 1);

	for (int y = 0; y <= CHUNK_RESOLUTION; y++)
		for (int x = 0; x <= CHUNK_RESOLUTION; x++) {
			FDynamicMeshVertex vertex;
			vertex.Position = FVector(CHUNK_SIZE_CM* x / (float)(CHUNK_RESOLUTION), CHUNK_SIZE_CM* y / (float)(CHUNK_RESOLUTION), 0);
			vertex.TextureCoordinate = FVector2D(x*CHUNK_TEXTURE_STEP + CHUNK_TEXTURE_STEP*1.5, y*CHUNK_TEXTURE_STEP + CHUNK_TEXTURE_STEP*1.5); //shift borders and set uv on texel center
			vertex.SetTangents(TangentX, TangentY, TangentZ);
			VertexBuffer.Vertices.Add(vertex);
		}
	int step = 1;
	for (int i = 0; i < (int)eChunkLOD::COUNT; i++) {
		GenerateChunk(step, m_LODS[i], IndexBuffer);
		step *= 2;
	}

	// Init vertex factory
	VertexFactory.Init(&VertexBuffer);

	// Enqueue initialization of render resource
	BeginInitResource(&VertexBuffer);
	BeginInitResource(&IndexBuffer);
	BeginInitResource(&VertexFactory);

	m_Width = 0;
	m_Height = 0;
	m_ChunksCount = 0;
}

FcTerraformaMeshSceneProxy::~FcTerraformaMeshSceneProxy()
{
	VertexBuffer.ReleaseResource();
	IndexBuffer.ReleaseResource();
	VertexFactory.ReleaseResource();
}

void FcTerraformaMeshSceneProxy::ReinitChunks() {	
	m_Width = m_LandscapeComponent->m_Landscape.width();
	m_Height = m_LandscapeComponent->m_Landscape.height();
	m_ChunksCount = m_Width*m_Height;

	sChunkInfo chunk;
	chunk.Mesh.VertexFactory = &VertexFactory;
	chunk.Mesh.ReverseCulling = false;
	chunk.Mesh.Type = PT_TriangleList;
	chunk.Mesh.DepthPriorityGroup = SDPG_World;
	chunk.Mesh.bCanApplyViewModeOverrides = false;
	chunk.Mesh.bWireframe = false;

	m_Chunks.Init(chunk, m_ChunksCount);

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

void FcTerraformaMeshSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const {
	//
}



void FcTerraformaMeshSceneProxy::DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) {
	if (m_ChunksCount != m_LandscapeComponent->m_Landscape.width()*m_LandscapeComponent->m_Landscape.height()) {
		ReinitChunks();
	}

	if (m_ChunksCount == 0)
		return;

	for (int i = 0; i < m_Chunks.Num(); i++) {
		sChunkInfo& chunk = (sChunkInfo&)m_Chunks[i];
		FMeshBatch &Mesh = chunk.Mesh;
		m_Chunks[i].MaterialInst = m_LandscapeComponent->m_DynMaterialInstances[i];
		Mesh.MaterialRenderProxy = m_Chunks[i].MaterialInst->GetRenderProxy(false);
		Mesh.Elements[0].PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(chunk.Transform, GetBounds(), GetLocalBounds(), true, UseEditorDepthTest());
		PDI->DrawMesh(Mesh, FLT_MAX);
	}
}

FPrimitiveViewRelevance FcTerraformaMeshSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = IsShown(View);
	Result.bShadowRelevance = IsShadowCast(View);
	Result.bDynamicRelevance = false;
	Result.bStaticRelevance = true;
	MaterialRelevance.SetPrimitiveViewRelevance(Result);
	return Result;
}








