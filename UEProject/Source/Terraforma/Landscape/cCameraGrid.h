#pragma once

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "cTerraformaGrid.h"
#include "DrawDebugHelpers.h"

const float CHUNK_SIZE_CM = 1000.f;
const float TEXEL_SIZE_CM = CHUNK_SIZE_CM / CHUNK_H_RESOLUTION;
const float MAX_HEIGHT_CM = 256.f * 25.f;// 6400
const int CONVEX_MIN_BORDER = 2;
const float CONVEX_BORDER_INCREASE_PER_CHUNK = 0.1;

enum class eBoxSide {
	MINUS_Z = 0,
	PLUS_Z,
	MINUS_X,
	PLUS_X,
	MINUS_Y,
	PLUS_Y,
	COUNT
};

struct sPlaneInfo {
	FVector Origin;
	FVector Normal;
	sPlaneInfo() {}
	sPlaneInfo(FVector origin, FVector normal) :Origin(origin), Normal(normal) {}
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

const unsigned char SEAM_MINUS_X_FLAG = 1;
const unsigned char SEAM_PLUS_X_FLAG  = 2;
const unsigned char SEAM_MINUS_Y_FLAG = 4;
const unsigned char SEAM_PLUS_Y_FLAG  = 8;
const unsigned char SEAM_COUNT = SEAM_MINUS_X_FLAG + SEAM_PLUS_X_FLAG + SEAM_MINUS_Y_FLAG + SEAM_PLUS_Y_FLAG;

inline unsigned char BuildSeamID(bool MinusXSeam, bool PlusXSeam, bool MinusYSeam, bool PlusYSeam) {
	return 0 &	(MinusXSeam ? SEAM_MINUS_X_FLAG : 0) & 
				(PlusXSeam ? SEAM_PLUS_X_FLAG : 0) & 
				(MinusYSeam ? SEAM_MINUS_Y_FLAG : 0) & 
				(PlusYSeam ? SEAM_PLUS_Y_FLAG : 0);
}

const int CHUNK_LODS_DISTANCE_SQUARED[(int)eChunkLOD::COUNT - 1] = { 0*0, 4*4, 6*6, 8*8, 10*10 };
const int CHUNK_MASK_RESOLUTION = 16*2 + 1;

struct sGridVisibleState {
	bool visible;
	eChunkLOD lod;
	unsigned char seamID;
};

class cCameraGrid
{
protected:	
	TArray<sGridVisibleState>	m_VisibilityGrid;
	sGridVisibleState			m_DefaultLODMask;
	sGridVisibleState			m_LODMask[CHUNK_MASK_RESOLUTION][CHUNK_MASK_RESOLUTION];

	TArray<FIntPoint>	m_Convex;
	bool getFrustumPoints(FVector cameraPos, FRotationMatrix cameraRot, float cameraFOV, float cameraAspect, FVector gridPos, int chunksXCount, int chunksYCount, float maxHeight);
	void fillGrid(int cameraXChunk, int cameraYChunk, int chunksXCount, int chunksYCount);
	
	void initLODMask();
	inline const sGridVisibleState& getMask(int x, int y, int cameraXChunk, int cameraYChunk);
public:
	cCameraGrid();

	bool recalcVisibleItems(FVector cameraPos, FRotator cameraRot, float cameraFOV, float cameraAspect, FVector gridPos, int chunksXCount, int chunksYCount, float maxHeight);
	bool recalcVisibleItems(UCameraComponent* camera, FVector gridPos, int chunksXCount, int chunksYCount, float maxHeight) {
		return recalcVisibleItems(camera->GetComponentLocation(), camera->GetComponentRotation(), camera->FieldOfView, camera->AspectRatio,
			gridPos, chunksXCount, chunksYCount, maxHeight);
	}
	bool recalcVisibleItems(APlayerCameraManager* camera, FVector gridPos, int chunksXCount, int chunksYCount, float maxHeight) {
		return recalcVisibleItems(camera->CameraCache.POV.Location, camera->CameraCache.POV.Rotation, camera->CameraCache.POV.FOV, camera->CameraCache.POV.AspectRatio,
			gridPos, chunksXCount, chunksYCount, maxHeight);
	}

	const TArray<sGridVisibleState>& VisibilityGrid() { return m_VisibilityGrid; }
};

