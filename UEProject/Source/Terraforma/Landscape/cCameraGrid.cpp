#include "cCameraGrid.h"

struct sRasterLine {
	FIntPoint start;
	FIntPoint end;
	float xStepBy1Y;
	sRasterLine(FIntPoint Start, FIntPoint End) :start(Start), end(End) {
		int count = end.Y - start.Y;
		if (count==0) {
			xStepBy1Y = 0;
		}
		else {
			xStepBy1Y = float(end.X - start.X) / count;
		}
	}
};

inline const sGridVisibleState& cCameraGrid::getMask(int x, int y, int cameraXChunk, int cameraYChunk) {
	int center = (CHUNK_MASK_RESOLUTION - 1) / 2; 
	int gx = (cameraXChunk - x) + center;
	if (gx < 0 || gx >= CHUNK_MASK_RESOLUTION)
		return m_DefaultLODMask;
	int gy = (cameraYChunk - y) + center;
	if (gy < 0 || gy >= CHUNK_MASK_RESOLUTION)
		return m_DefaultLODMask;
	return m_LODMask[gx][gy];
}


void cCameraGrid::initLODMask() {	
	eChunkLOD worstLOD = (eChunkLOD)((int)eChunkLOD::COUNT - 1);

	m_DefaultLODMask.lod = worstLOD;
	m_DefaultLODMask.visible = true;
	m_DefaultLODMask.seamID = 0;

	int center = (CHUNK_MASK_RESOLUTION - 1) / 2;
	for (int y = 0; y<CHUNK_MASK_RESOLUTION; y++)
		for (int x = 0; x < CHUNK_MASK_RESOLUTION; x++) {
			int squaredDist = FMath::Square(x - center) + FMath::Square(y - center);
			eChunkLOD lod = worstLOD;
			for (int i = 0; i < (int)eChunkLOD::COUNT - 1; i++) {
				if (CHUNK_LODS_DISTANCE_SQUARED[i] > squaredDist) {
					lod = (eChunkLOD)i;
					break;
				}
			}
			m_LODMask[x][y].lod = lod;
			m_LODMask[x][y].visible = true;
			m_LODMask[x][y].seamID = 0;
		}

	for (int y = 1; y<CHUNK_MASK_RESOLUTION-1; y++)
		for (int x = 1; x < CHUNK_MASK_RESOLUTION - 1; x++) {
			eChunkLOD lod = m_LODMask[x][y].lod;
			if (lod != worstLOD) {
				m_LODMask[x][y].seamID = 0;
				if (m_LODMask[x + 1][y].lod > lod)
					m_LODMask[x][y].seamID += SEAM_MINUS_X_FLAG;
				if (m_LODMask[x - 1][y].lod > lod)
					m_LODMask[x][y].seamID += SEAM_PLUS_X_FLAG;
				if (m_LODMask[x][y + 1].lod > lod)
					m_LODMask[x][y].seamID += SEAM_MINUS_Y_FLAG;
				if (m_LODMask[x][y - 1].lod > lod)
					m_LODMask[x][y].seamID += SEAM_PLUS_Y_FLAG;
			}
		}
}

cCameraGrid::cCameraGrid() {
	initLODMask();
}

enum class eBoxSide {
	MINUS_Z = 0,
	//PLUS_Z,
	//MINUS_X,
	//PLUS_X,
	//MINUS_Y,
	//PLUS_Y,	
	COUNT
};

struct sPlaneInfo {
	FVector Origin;
	FVector Normal;
	sPlaneInfo() {}
	sPlaneInfo(FVector origin, FVector normal) :Origin(origin), Normal(normal) {}
};

const int FAR_PLANE_VECTORS_COUNT = 4;

int getPointDistanceToLine(FIntPoint start, FIntPoint end, FIntPoint p) {
	return (end.X - start.X)*(p.Y - end.Y) - (end.Y - start.Y)*(p.X - end.X);
}

template <class PREDICATE_CLASS>
void SortArrayFromSecond(TArray<FIntPoint>& points, const PREDICATE_CLASS& Predicate)
{
	::Sort(&(points.GetData()[1]), points.Num()-1, Predicate);
}

bool cCameraGrid::getFrustumPoints(FVector cameraPos, FRotationMatrix cameraRot, float cameraFOV, float cameraAspect, FVector gridPos, int chunksXCount, int chunksYCount, float maxHeight) {
	FVector gridSize(chunksXCount*CHUNK_SIZE_CM, chunksYCount*CHUNK_SIZE_CM, maxHeight);

	sPlaneInfo boundBox[eBoxSide::COUNT];
	boundBox[(int)eBoxSide::MINUS_Z] = sPlaneInfo(gridPos, FVector(0, 0, -1));
	//boundBox[(int)eBoxSide::PLUS_Z] = sPlaneInfo(gridPos + gridSize, FVector(0, 0, +1));
	//boundBox[(int)eBoxSide::MINUS_X] = sPlaneInfo(gridPos, FVector(-1, 0, 0));
	//boundBox[(int)eBoxSide::PLUS_X] = sPlaneInfo(gridPos+ gridSize, FVector(+1, 0, 0));
	//boundBox[(int)eBoxSide::MINUS_Y] = sPlaneInfo(gridPos, FVector(0, -1, 0));
	//boundBox[(int)eBoxSide::PLUS_Y] = sPlaneInfo(gridPos + gridSize, FVector(0, +1, 0));
	

	float maxCameraDistance = cameraPos.Size() + gridSize.Size()*2;
		
	FVector Direction = cameraRot.GetScaledAxis(EAxis::X);
	FVector RightVector = cameraRot.GetScaledAxis(EAxis::Y);
	FVector UpVector = cameraRot.GetScaledAxis(EAxis::Z);

	//FOVAngle controls the horizontal angle.
	const float HozHalfAngleInRadians = FMath::DegreesToRadians(cameraFOV * 0.5f);
	const float HozLength = maxCameraDistance * FMath::Tan(HozHalfAngleInRadians);
	const float VertLength = HozLength / cameraAspect;

	FVector FarPlaneVectors[FAR_PLANE_VECTORS_COUNT];
	FarPlaneVectors[0] = (Direction * maxCameraDistance) + (UpVector * VertLength) - (RightVector * HozLength);
	FarPlaneVectors[1] = (Direction * maxCameraDistance) + (UpVector * VertLength) + (RightVector * HozLength);
	FarPlaneVectors[2] = (Direction * maxCameraDistance) - (UpVector * VertLength) + (RightVector * HozLength);
	FarPlaneVectors[3] = (Direction * maxCameraDistance) - (UpVector * VertLength) - (RightVector * HozLength);

	for (int vector = 0; vector < FAR_PLANE_VECTORS_COUNT; vector++) {
		FVector& v = FarPlaneVectors[vector];
		float vSizeSquared = v.SizeSquared();
		for (int plane = 0; plane < (int)eBoxSide::COUNT; plane++) {
			sPlaneInfo& p = boundBox[plane];

			bool cameraIsBehindPlane = FVector::PointPlaneDist(cameraPos, p.Origin, p.Normal)<0;
			if (cameraIsBehindPlane) {
				bool IsVectorParralelToPlane = FMath::Abs(p.Normal.X*v.X + p.Normal.Y*v.Y + p.Normal.Z*v.Z) < 0.0001;
				if (!IsVectorParralelToPlane) {
					bool IsPlaneBehindVector = FVector::DotProduct(p.Normal, v)<0.0001;
					if (!IsPlaneBehindVector) {
						FVector intersectionPoint = FMath::LinePlaneIntersection(cameraPos, cameraPos+v, p.Origin, p.Normal);
						float intersectionPointSizeSquared = FVector::DistSquared(cameraPos, intersectionPoint);
						if (intersectionPointSizeSquared < vSizeSquared) {
							v = intersectionPoint - cameraPos;
							vSizeSquared = intersectionPointSizeSquared;
						}
					}					
				}				
			}			
		}
	}
	//at this moment we have four vectors of far plane clipped by grid bounding box

	TArray<FIntPoint> ZonePoints;
	ZonePoints.SetNum(FAR_PLANE_VECTORS_COUNT);
	for (int i = 0; i < FAR_PLANE_VECTORS_COUNT; i++)
		ZonePoints[i] = FIntPoint((int)((cameraPos.X+FarPlaneVectors[i].X-gridPos.X)/ CHUNK_SIZE_CM), (int)((cameraPos.Y+FarPlaneVectors[i].Y - gridPos.Y) / CHUNK_SIZE_CM) );
	ZonePoints.Add(FIntPoint((int)((cameraPos.X - gridPos.X) / CHUNK_SIZE_CM), (int)((cameraPos.Y - gridPos.Y) / CHUNK_SIZE_CM)));

	ZonePoints.Sort([](const FIntPoint& A, const FIntPoint& B) {
		return A.Y < B.Y;
	});

	//remove all duplicates
	int pos = 1;
	while (pos < ZonePoints.Num()) {
		if (ZonePoints[pos - 1].X == ZonePoints[pos].X && ZonePoints[pos - 1].Y == ZonePoints[pos].Y) {
			ZonePoints.RemoveAt(pos);
		}
		else {
			pos++;
		}
	}

	SortArrayFromSecond(ZonePoints, 
	[&](const FIntPoint& A, const FIntPoint& B) {		
		return getPointDistanceToLine(ZonePoints[0], A, B)<0;
	});

	TArray<FIntPoint> ZonePointsClean;
	ZonePointsClean.Reserve(ZonePoints.Num());
	ZonePointsClean.Add(ZonePoints[0]);
	ZonePointsClean.Add(ZonePoints[1]);

	
	for (int i = 2; i < ZonePoints.Num(); i++) {
		FIntPoint next = ZonePoints[i];
		FIntPoint current = ZonePointsClean[ZonePointsClean.Num() - 1];
		FIntPoint prev = ZonePointsClean[ZonePointsClean.Num() - 2];
		int pointDist = getPointDistanceToLine(prev, current , next);		
		while ( pointDist > 0){
			if (ZonePointsClean.Num() == 2) {
				GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::Red, "CRASH");
			}
			ZonePointsClean.Pop();

			current = ZonePointsClean[ZonePointsClean.Num() - 1];
			prev = ZonePointsClean[ZonePointsClean.Num() - 2];
			pointDist = getPointDistanceToLine(prev, current, next);
		}
		
		ZonePointsClean.Add(ZonePoints[i]);
	}
	
	//It is possible situation: 3 points(and only 3!) make one first line. Remove middle point.
	//Also its possible 3 point make another line, but important its only if this line is first and horizontal
	if (ZonePointsClean[2].Y == ZonePointsClean[0].Y)
		ZonePointsClean.RemoveAt(1);
	
	bool ZoneChanged = m_Convex.Num() != ZonePointsClean.Num();
	if (!ZoneChanged) {
		for (int i = 0; i<m_Convex.Num(); i++)
			if (m_Convex[i] != ZonePointsClean[i]) {
				ZoneChanged = true;
				break;
			}
	}

	if (ZoneChanged)
		m_Convex = ZonePointsClean;
	return ZoneChanged;
}

void cCameraGrid::fillGrid(int cameraXChunk, int cameraYChunk, int chunksXCount, int chunksYCount) {
	TArray<sRasterLine> LeftSide;
	TArray<sRasterLine> RightSide;
	LeftSide.Reserve(m_Convex.Num());
	RightSide.Reserve(m_Convex.Num());
	sRasterLine r(m_Convex[0], m_Convex[m_Convex.Num() - 1]);
	RightSide.Add(r);

	for (int i = 1; i < m_Convex.Num(); i++) {
		if (m_Convex[i].Y >= m_Convex[i - 1].Y) {
			sRasterLine l(m_Convex[i - 1], m_Convex[i]);
			LeftSide.Add(l);
		}
		else {
			sRasterLine r(m_Convex[i], m_Convex[i - 1]);
			RightSide.Add(r);
		}
	}	

	LeftSide.Sort([](const sRasterLine& A, const sRasterLine& B) {
		return A.start.Y < B.start.Y;
	});
	RightSide.Sort([](const sRasterLine& A, const sRasterLine& B) {
		return A.start.Y < B.start.Y;
	});

	int yStart = LeftSide[0].start.Y;
	int yEnd = FMath::Max(LeftSide[LeftSide.Num() - 1].end.Y, RightSide[RightSide.Num() - 1].end.Y);
	float xStart = LeftSide[0].start.X;
	float xEnd = RightSide[0].start.X;
	int leftID = 0;
	int rightID = 0;

	int BORDER = 2;
	
	for (int y = yStart; y <= yEnd && y<chunksYCount; y++) {
		if (y == LeftSide[leftID].end.Y) {
			xStart = LeftSide[leftID].end.X;
			leftID++;
		}
		if (y == RightSide[rightID].end.Y) {
			xEnd = RightSide[rightID].end.X;
			rightID++;
		}
		if (y > 0) {
			int sy = 0;
			int cy = 1;
			if (y == yStart) {				
				cy = BORDER;
				sy = 1-cy;
			}
			else
			if (y == yEnd) {
				cy = BORDER+1;
				sy = 0;
			}
			for (int iy = sy + y; iy < cy + y; iy++) {
				if (iy >= 0 && iy < chunksYCount) {
					int lx = FMath::Max(0, (int)xStart - BORDER);
					//int gridIndex = lx + iy*chunksYCount;
					for (int x = lx; x <= FMath::Min(chunksYCount - 1, (int)xEnd + BORDER); x++) {
						int gridIndex = x + iy*chunksYCount;
						m_VisibilityGrid[gridIndex] = getMask(x, iy, cameraXChunk, cameraYChunk);
						gridIndex++;
					}
				}
			}
		}

		if (y != yEnd) {
			xStart += LeftSide[leftID].xStepBy1Y;
			xEnd += RightSide[rightID].xStepBy1Y;
		}
	}

}


bool cCameraGrid::recalcVisibleItems(FVector cameraPos, FRotator cameraRot, float cameraFOV, float cameraAspect, FVector gridPos, int chunksXCount, int chunksYCount, float maxHeight) {
	bool ZoneChanged = getFrustumPoints(cameraPos, cameraRot, cameraFOV, cameraAspect, gridPos, chunksXCount, chunksYCount, maxHeight);

	if (m_VisibilityGrid.Num() != chunksXCount*chunksYCount)
		ZoneChanged = true;
	
	if (ZoneChanged) {
		m_VisibilityGrid.SetNum(chunksXCount*chunksYCount);
		for (int i = 0; i < m_VisibilityGrid.Num(); i++)
			m_VisibilityGrid[i].visible = false;

		fillGrid((int)((cameraPos.X - gridPos.X) / CHUNK_SIZE_CM), (int)((cameraPos.Y - gridPos.Y) / CHUNK_SIZE_CM),
				chunksXCount,chunksYCount);
	}		
	
	return ZoneChanged;
}