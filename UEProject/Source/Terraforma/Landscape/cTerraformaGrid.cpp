#include "cTerraformaGrid.h"
#include "MemoryReader.h"

void packNormal(float x, float y, float z, sChunkNormalData& outNormal) {
	outNormal.x = 128 + (int)(x * 128);
	outNormal.y = 128 + (int)(y * 128);
	outNormal.z = 128 + (int)(z * 128);
}

int sTerraformaGridChunk::ApplyCutTerraforming(int globalX, int globalY, uint16 targetHeight, int dstX, int dstY, int dstWidth, int dstHeight, int srcX, int srcY, const FsTerraformaTemplate& heightmap, uint8 heightmapFactor, uint16 ApplyRangeMin, uint16 ApplyRangeMax) {
	int terraformingCounter = 0;
	for (int y = 0; y < dstHeight; y++) {
		sChunkHeightData* dstHeightmap = &dynDataHeightMap[y + dstY][dstX];
		sTechInfo* dstTech = &TechData[y + dstY][dstX];
		uint8* srcData = (uint8*)heightmap.RawData.GetData() + (srcY + y)*heightmap.Size + srcX;
		for (int x = 0; x < dstWidth; x++) {
			if (dstHeightmap[x].height >= ApplyRangeMin && dstHeightmap[x].height <= ApplyRangeMax) {
				int dataChange = (srcData[x] - 127)*heightmapFactor;
				if (dataChange < 0) {
					dstTech[x].Changed = true;
					int newHeight = targetHeight + dataChange;
					if (newHeight < 0)
						newHeight = 0;

					int diff = dstHeightmap[x].height - newHeight;

					if (diff > 0) {
						dstHeightmap[x].height = newHeight;
						terraformingCounter += diff;
					};
				}
			}
		}
	}
	
	HeightmapChanged = true;
	return terraformingCounter;
}


int sTerraformaGridChunk::ApplyTerraforming(int globalX, int globalY, int dstX, int dstY, int dstWidth, int dstHeight, int srcX, int srcY, const FsTerraformaTemplate& heightmap, uint8 heightmapFactor, const FsTerraformaTemplate* terraformedColormap, float colorFactor, uint16 ApplyRangeMin, uint16 ApplyRangeMax) {
	int terraformingCounter = 0;
	for (int y = 0; y < dstHeight; y++) {
		sChunkHeightData* dstHeightmap = &dynDataHeightMap[y+dstY][dstX];
		sTechInfo* dstTech = &TechData[y+dstY][dstX];
		uint8* srcData = (uint8*)heightmap.RawData.GetData() + (srcY+y)*heightmap.Size + srcX;
		for (int x = 0; x < dstWidth; x++) {
			if (dstHeightmap[x].height >= ApplyRangeMin && dstHeightmap[x].height <= ApplyRangeMax) {
				dstTech[x].Changed = true;
				int dataChange = (srcData[x] - 127)*heightmapFactor;
				/*if (x+srcX == 10 || y+srcY-dstY == 10)
					dataChange = 15;
				else
					dataChange = 0;*/
				int newHeight = dataChange + dstHeightmap[x].height;
				if (newHeight < 0) {
					dataChange -= newHeight;
					dstHeightmap[x].height = 0;
				}
				else
					if (newHeight > MAX_uint16) {
						dataChange -= (MAX_uint16 - newHeight);
						dstHeightmap[x].height = MAX_uint16;
					}
					else
						dstHeightmap[x].height = newHeight;
				int ix = x*2 + dstX*2;
				int iy = y*2 + dstY*2;

				if (ix > 0 && ix < CHUNK_T_RESOLUTION + 1 && iy > 0 && iy < CHUNK_T_RESOLUTION + 1)
					terraformingCounter += dataChange;
				if (terraformedColormap != nullptr) {
					float lerpFactor = FMath::Min(FMath::Abs(dataChange*colorFactor), 1.f);
					int u = (globalX + x) % terraformedColormap->Size;
					int v = (globalY + y) % terraformedColormap->Size;
					sChunkTextureData* newColor = (sChunkTextureData*)(&(terraformedColormap->RawData.GetData()[terraformedColormap->getIndex(u, v)]));


					for (int ty = 0; ty<2; ty++)
						for (int tx = 0; tx < 2; tx++) {
							sChunkTextureData* dstColor = &dynDataTexture[iy+ty][ix+tx];
							dstColor->red = FMath::Lerp(dstColor->red, newColor->red, lerpFactor);
							dstColor->green = FMath::Lerp(dstColor->green, newColor->green, lerpFactor);
							dstColor->blue = FMath::Lerp(dstColor->blue, newColor->blue, lerpFactor);
							dstColor->reserved = FMath::Lerp(dstColor->reserved, (uint8)0, lerpFactor);
						}					
				}
			}
		}
	}
		

	if (terraformedColormap != nullptr) {
		TextureChanged = true;
	}
	HeightmapChanged = true;
	return terraformingCounter;
}

void sTerraformaGridChunk::ApplyColoring(int dstX, int dstY, int dstWidth, int dstHeight, int srcX, int srcY, const FsTerraformaTemplate& colormap, uint16 ApplyRangeMin, uint16 ApplyRangeMax)
{
	for (int y = 0; y < dstHeight; y++) {
		sChunkHeightData* dstHeightmap = &dynDataHeightMap[y/2 + dstY/2][dstX/2];
		sChunkTextureData* dstColormap = &dynDataTexture[y + dstY][dstX];
		sChunkTextureData* srcData = (sChunkTextureData*)(&colormap.RawData.GetData()[colormap.getIndex(srcX,srcY+y)]);
		for (int x = 0; x < dstWidth; x++) {		
			if (dstHeightmap[x/2].height >= ApplyRangeMin && dstHeightmap[x/2].height <= ApplyRangeMax) {
				sChunkTextureData* dstColor = &dstColormap[x];
				sChunkTextureData& srcColor = srcData[x];// (sChunkTextureData*)(&colormap.RawData.GetData()[colormap.getIndex(srcX + x, srcY + y)]);

				float lerpFactor = srcColor.reserved / 255.f;
				dstColor->red = FMath::Lerp(dstColor->red, srcColor.red, lerpFactor);
				dstColor->green = FMath::Lerp(dstColor->green, srcColor.green, lerpFactor);
				dstColor->blue = FMath::Lerp(dstColor->blue, srcColor.blue, lerpFactor);
				dstColor->reserved = FMath::Lerp(dstColor->reserved, (uint8)0, lerpFactor);
			}
		}
	}


	TextureChanged = true;
}





cTerraformaGrid::cTerraformaGrid() {
	m_Width = 0;
	m_Height = 0;
	m_Map = nullptr;
};

void cTerraformaGrid::init(int width, int height) {
	if (width == m_Width && height == m_Height && m_Map != nullptr)
		return;
	if (m_Map != nullptr) {
		delete m_Map;
		m_Map = nullptr;
	}

	m_Width = width;
	m_Height = height;
	if (width == 0 || height == 0)
		return;
		

	m_Map = new sTerraformaGridChunk[m_Width*m_Height];	
}

const uint32 TFA_FILE_VERSION = 4;

bool cTerraformaGrid::loadFromArray(const TArray<uint8>& TFARawData) {
	FMemoryReader Reader(TFARawData);
	uint32 fileVersion;
	Reader.Serialize(&fileVersion, sizeof(uint32));
	if (fileVersion != TFA_FILE_VERSION) {
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::Red, "Incorrect file version in terraforma landscape ");
		return false;
	}
	uint32 chunksXCount;
	uint32 chunksYCount;
	Reader.Serialize(&chunksXCount, sizeof(uint32));
	Reader.Serialize(&chunksYCount, sizeof(uint32));
	init(chunksXCount, chunksYCount);
	int i = 0;
	for (int y = 0; y<m_Height; y++)
		for (int x = 0; x < m_Width; x++) {
			m_Map[i].x = x;
			m_Map[i].y = y;
			m_Map[i].minHeight = 0;
			m_Map[i].maxHeight = 0;
			Reader.Serialize(m_Map[i].dynDataHeightMap, sizeof(sChunkHeightData)*(CHUNK_H_RESOLUTION + 2)*(CHUNK_H_RESOLUTION + 2));
			for (int ix = 0; ix < CHUNK_H_RESOLUTION + 2; ix++)
				for (int iy = 0; iy<CHUNK_H_RESOLUTION + 2; iy++) {
					uint16 height = m_Map[i].dynDataHeightMap[iy][ix].height;
					if (m_Map[i].minHeight > height)
						m_Map[i].minHeight = height;
					if (m_Map[i].maxHeight < height)
						m_Map[i].maxHeight = height;
				}

			m_Map[i].HeightmapChanged = true;
			i++;
		}
	i = 0;
	for (int y = 0; y<m_Height; y++)
		for (int x = 0; x < m_Width; x++) {
			Reader.Serialize(m_Map[i].dynDataTexture, sizeof(sChunkTextureData)*(CHUNK_T_RESOLUTION + 2)*(CHUNK_T_RESOLUTION + 2));

			m_Map[i].TextureChanged = true;
			i++;
		}

	i = 0;
	for (int y = 0; y<m_Height; y++)
		for (int x = 0; x < m_Width; x++) {
			Reader.Serialize(m_Map[i].dynDataNormalMap, sizeof(sChunkNormalData)*(CHUNK_H_RESOLUTION + 2)*(CHUNK_H_RESOLUTION + 2));
			i++;
		}

	return true;
}

bool cTerraformaGrid::loadFromFile(FString FileName) {
	TArray<uint8> tfaRAW;
	if (!FFileHelper::LoadFileToArray(tfaRAW, *FileName))
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::Red, "Cant open terraforma landscape from " + FileName);
		return false;
	}

	return loadFromArray(tfaRAW);
}

int cTerraformaGrid::ApplyCutTerraforming(int x, int y, uint16 targetHeight, const FsTerraformaTemplate& cutHeightmap, uint8 heightmapFactor, uint16 ApplyRangeMin, uint16 ApplyRangeMax) {
	int terraformingCounter = 0;

	int startX = x - cutHeightmap.Size / 2;
	int startY = y - cutHeightmap.Size / 2;
	int endX = startX + cutHeightmap.Size - 1;
	int endY = startY + cutHeightmap.Size - 1;
	int startXChunk = FMath::Max(0, (startX - 1) / CHUNK_H_RESOLUTION);
	int endXChunk = FMath::Min(m_Width - 1, (endX + 1) / CHUNK_H_RESOLUTION);
	int startYChunk = FMath::Max(0, (startY - 1) / CHUNK_H_RESOLUTION);
	int endYChunk = FMath::Min(m_Height - 1, (endY + 1) / CHUNK_H_RESOLUTION);

	for (int cy = startYChunk; cy <= endYChunk; cy++) {
		int chunkIndex = getIndex(startXChunk, cy);
		for (int cx = startXChunk; cx <= endXChunk; cx++) {
			sTerraformaGridChunk& chunk = m_Map[chunkIndex]; chunkIndex++;

			int chunkLeft = cx*CHUNK_H_RESOLUTION - 1;
			int chunkRight = cx*CHUNK_H_RESOLUTION + CHUNK_H_RESOLUTION + 1;
			int chunkTop = cy*CHUNK_H_RESOLUTION - 1;
			int chunkBottom = cy*CHUNK_H_RESOLUTION + CHUNK_H_RESOLUTION + 1;

			int placeX = chunkLeft;
			if (placeX < startX)
				placeX = startX;
			int placeY = chunkTop;
			if (placeY < startY)
				placeY = startY;

			int placeXR = chunkRight;
			if (placeXR > endX)
				placeXR = endX;
			int placeYB = chunkBottom;
			if (placeYB > endY)
				placeYB = endY;

			terraformingCounter += chunk.ApplyCutTerraforming(placeX, placeY, targetHeight,
				placeX - chunkLeft, placeY - chunkTop, placeXR - placeX + 1, placeYB - placeY + 1,
				placeX - startX, placeY - startY, cutHeightmap, heightmapFactor,
				ApplyRangeMin,ApplyRangeMax);
		}
	}

	return terraformingCounter;
}

int cTerraformaGrid::ApplyTerraforming(int x, int y, const FsTerraformaTemplate& heightmap, uint8 heightmapFactor, const FsTerraformaTemplate* terraformedColormap, float colorFactor, uint16 ApplyRangeMin, uint16 ApplyRangeMax) {
	int terraformingCounter = 0;

	int startX = x - heightmap.Size / 2;
	int startY = y - heightmap.Size / 2;
	int endX = startX + heightmap.Size -1;
	int endY = startY + heightmap.Size -1;
	int startXChunk = FMath::Max(0,(startX-1) / CHUNK_H_RESOLUTION);
	int endXChunk = FMath::Min(m_Width-1,(endX+1) / CHUNK_H_RESOLUTION);
	int startYChunk = FMath::Max(0,(startY-1) / CHUNK_H_RESOLUTION);
	int endYChunk = FMath::Min(m_Height-1,(endY+1) / CHUNK_H_RESOLUTION);

	for (int cy = startYChunk; cy <= endYChunk; cy++) {
		int chunkIndex = getIndex(startXChunk,cy);
		for (int cx = startXChunk; cx <= endXChunk; cx++) {
			sTerraformaGridChunk& chunk = m_Map[chunkIndex]; chunkIndex++;

			int chunkLeft = cx*CHUNK_H_RESOLUTION - 1;
			int chunkRight = cx*CHUNK_H_RESOLUTION + CHUNK_H_RESOLUTION + 1;
			int chunkTop = cy*CHUNK_H_RESOLUTION - 1;
			int chunkBottom = cy*CHUNK_H_RESOLUTION + CHUNK_H_RESOLUTION + 1;

			int placeX = chunkLeft;
			if (placeX < startX)
				placeX = startX;
			int placeY = chunkTop;
			if (placeY < startY)
				placeY = startY;

			int placeXR = chunkRight;
			if (placeXR > endX)
				placeXR = endX;
			int placeYB = chunkBottom;
			if (placeYB > endY)
				placeYB = endY;

			terraformingCounter += chunk.ApplyTerraforming(placeX, placeY, 
															placeX-chunkLeft,placeY-chunkTop,placeXR-placeX+1,placeYB-placeY+1,
															placeX-startX,placeY-startY,heightmap, heightmapFactor, 
				terraformedColormap, colorFactor,
				ApplyRangeMin, ApplyRangeMax);
		}
	}

	return terraformingCounter;
}

void cTerraformaGrid::ApplyColoring(int x, int y, const FsTerraformaTemplate& colormap, uint16 ApplyRangeMin, uint16 ApplyRangeMax) {
	int startX = x*2 - colormap.Size / 2;
	int startY = y*2 - colormap.Size / 2;
	int endX = startX + colormap.Size - 1;
	int endY = startY + colormap.Size - 1;
	int startXChunk = FMath::Max(0, (startX - 1) / CHUNK_T_RESOLUTION);
	int endXChunk = FMath::Min(m_Width - 1, (endX + 1) / CHUNK_T_RESOLUTION);
	int startYChunk = FMath::Max(0, (startY - 1) / CHUNK_T_RESOLUTION);
	int endYChunk = FMath::Min(m_Height - 1, (endY + 1) / CHUNK_T_RESOLUTION);

	for (int cy = startYChunk; cy <= endYChunk; cy++) {
		int chunkIndex = getIndex(startXChunk, cy);
		for (int cx = startXChunk; cx <= endXChunk; cx++) {
			sTerraformaGridChunk& chunk = m_Map[chunkIndex]; chunkIndex++;

			int chunkLeft = cx*CHUNK_T_RESOLUTION - 1;
			int chunkRight = cx*CHUNK_T_RESOLUTION + CHUNK_T_RESOLUTION + 1;
			int chunkTop = cy*CHUNK_T_RESOLUTION - 1;
			int chunkBottom = cy*CHUNK_T_RESOLUTION + CHUNK_T_RESOLUTION + 1;

			int placeX = chunkLeft;
			if (placeX < startX)
				placeX = startX;
			int placeY = chunkTop;
			if (placeY < startY)
				placeY = startY;

			int placeXR = chunkRight;
			if (placeXR > endX)
				placeXR = endX;
			int placeYB = chunkBottom;
			if (placeYB > endY)
				placeYB = endY;

			chunk.ApplyColoring(placeX - chunkLeft, placeY - chunkTop, placeXR - placeX + 1, placeYB - placeY + 1, placeX - startX, placeY - startY, colormap,
				ApplyRangeMin, ApplyRangeMax);
		}
	}
}
























/*
inline uint8 getPixelInLayer(int chunkX, int chunkY, int x, int y, uint8* data, int layerOffset, int layerResolution) {
	int index = chunkX*CHUNK_RESOLUTION + x + chunkY*layerResolution*CHUNK_RESOLUTION + y*layerResolution;
	if (index < 0)
		return 0;
	if (index >= layerResolution*layerResolution)
		return 0;
	return data[index + layerOffset];
}

void cTerraformaGrid::convertVMPtoTFA(FString Path, FString FileName) {
	FArchive* Reader = IFileManager::Get().CreateFileReader(*FileName, 0);
	if (Reader)
	{
		delete Reader;
		return;
	}
	
	FString VMPFileName = Path + "\\output.vmp";
	TArray<uint8> VMPRAW;
	if (!FFileHelper::LoadFileToArray(VMPRAW, *VMPFileName))
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::Red, "Cant open world "+ VMPFileName);
		return;
	}

	int Resolution = 0;
	if (VMPRAW.Num() == 16777236) {
		Resolution = 2048;
	}
	else
	if (VMPRAW.Num() == 67108884) {
		Resolution = 4096;
	}
	else {
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::Red, "Incorrect size in world " + FileName);
		return;
	}

	FString PaletteFileName = Path + "\\inDam.act";
	TArray<uint8> Palette;
	if (!FFileHelper::LoadFileToArray(Palette, *PaletteFileName))
	{
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::Red, "Cant open palette " + PaletteFileName);
		return;
	}

	if (Palette.Num() != 768) {
		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 5.5f, FColor::Red, "Incorrect size in palette" + PaletteFileName);
		return;
	};

	int LayerShift[4];
	LayerShift[0] = 19;
	LayerShift[1] = LayerShift[0] + Resolution*Resolution;
	LayerShift[2] = LayerShift[1] + Resolution*Resolution;
	LayerShift[3] = LayerShift[2] + Resolution*Resolution;

	uint8* vmp = VMPRAW.GetData();
	uint8* pal = Palette.GetData();

	TArray<uint8> tfaRAW;
	FMemoryWriter Writer(tfaRAW);
	Writer.Serialize((void*)&TFA_FILE_VERSION, sizeof(uint32));
	uint32 chunksCount = Resolution / CHUNK_RESOLUTION;
	Writer.Serialize(&chunksCount, sizeof(uint32));
	Writer.Serialize(&chunksCount, sizeof(uint32));
	for (unsigned int y = 0; y<chunksCount; y++)
		for (unsigned int x = 0; x < chunksCount; x++) {

			for (int iy = -1; iy<CHUNK_RESOLUTION+1; iy++)
				for (int ix = -1; ix < CHUNK_RESOLUTION + 1; ix++) {
					uint8 h1 = getPixelInLayer(x,y,ix,iy, vmp, LayerShift[1], Resolution);
					uint8 h2 = getPixelInLayer(x, y, ix, iy, vmp, LayerShift[2], Resolution);
					sChunkHeightData heightmap;
					heightmap.height = (h1 << 5) + (h2 & 0x1f);
					Writer.Serialize(&heightmap, sizeof(sChunkHeightData));
				}
		}
	for (unsigned int y = 0; y<chunksCount; y++)
		for (unsigned int x = 0; x < chunksCount; x++) {

			for (int iy = -1; iy<CHUNK_RESOLUTION + 1; iy++)
				for (int ix = -1; ix < CHUNK_RESOLUTION + 1; ix++) {
					uint8 colorIndex = getPixelInLayer(x, y, ix, iy, vmp, LayerShift[3], Resolution);
					sChunkTextureData color;
					color.red = pal[colorIndex * 3 + 0];
					color.green = pal[colorIndex * 3 + 1];
					color.blue = pal[colorIndex * 3 + 2];
					color.reserved = 0;

					Writer.Serialize(&color, sizeof(sChunkTextureData));
				}
		}
	FFileHelper::SaveArrayToFile(tfaRAW, *FileName);
}
*/