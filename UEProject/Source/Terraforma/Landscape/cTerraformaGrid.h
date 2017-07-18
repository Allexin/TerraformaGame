#pragma once
#include "sTerraformaTemplate.h"


/*
chunk texture size is CHUNK_RESOLUTION +4
+1 - is joint point with next chunk
+1 ... +1 - filtration border
+1 - parity
*/
const int CHUNK_H_RESOLUTION = 64;
const int CHUNK_T_RESOLUTION = 128;
const float CHUNK_H_STEP = 1.0f / (CHUNK_H_RESOLUTION + 4); 
const float CHUNK_T_STEP = 1.0f / (CHUNK_T_RESOLUTION + 4); 

struct sChunkHeightData {
	uint16 height;
};

struct sChunkTextureData {
	uint8 red;
	uint8 green;
	uint8 blue;
	uint8 reserved;
};

struct sChunkNormalData {
	uint8 x;
	uint8 y;
	uint8 z;
	uint8 reserved;
};

struct sTechInfo {
	int ChangedValue;
	sTechInfo() {
		ChangedValue = 0;
	}
};

struct sTerraformaGridChunk {	
	uint16 minHeight;
	uint16 maxHeight;
	int chunkX;
	int chunkY;
	
	sChunkHeightData	dynDataHeightMap[CHUNK_H_RESOLUTION + 4][CHUNK_H_RESOLUTION + 4];
	sChunkNormalData	dynDataNormalMap[CHUNK_H_RESOLUTION + 4][CHUNK_H_RESOLUTION + 4];
	sChunkTextureData	dynDataTexture[CHUNK_T_RESOLUTION + 4][CHUNK_T_RESOLUTION + 4];
	sTechInfo			TechData[CHUNK_H_RESOLUTION + 4][CHUNK_H_RESOLUTION + 4];
	bool HeightmapChanged;
	bool TextureChanged;

	int ApplyCutTerraforming(uint16 targetHeight, int dstX, int dstY, int dstWidth, int dstHeight, int srcX, int srcY, const FsTerraformaTemplate& heightmap, uint8 heightmapFactor, uint16 ApplyRangeMin = 0, uint16 ApplyRangeMax = MAX_uint16);
	int ApplyTerraforming(int dstX, int dstY, int dstWidth, int dstHeight, int srcX, int srcY, const FsTerraformaTemplate& heightmap, uint8 heightmapFactor, uint16 ApplyRangeMin = 0, uint16 ApplyRangeMax = MAX_uint16);
	void ApplyColoring(int dstX, int dstY, int dstWidth, int dstHeight, int srcX, int srcY, const FsTerraformaTemplate& colormap, const FsTerraformaTemplate* terraformedColormap, float colorFactor, uint16 ApplyRangeMin = 0, uint16 ApplyRangeMax = MAX_uint16);

	void Recalc(const FsTerraformaTemplate* terraformedColormap, float colorFactor, bool forceNormalRecalc = false);
protected:
	struct sHeightInfo {
		uint16 height;
		FVector vertex;
		bool available;
	};

	sHeightInfo GetHeight(int ix, int iy);
	FVector GetNormal(int ix, int iy, uint16 centerHeight);
};

class cTerraformaGrid {
protected:
	sTerraformaGridChunk* m_Map;
	int m_Width;
	int m_Height;
	void init(int width, int height);
	inline int getIndex(int x, int y) { return x + y*m_Width; }
public:
	cTerraformaGrid();	

	int width() const { return m_Width; }
	int height() const { return m_Height; }
	sTerraformaGridChunk* getData(int chunkX, int chunkY) { return &m_Map[getIndex(chunkX, chunkY)]; }
	sTerraformaGridChunk* getData(int index) { return &m_Map[index]; }

	/*DO NOT USE IT TO ITERATE THROUGH LANDSCAPE, GET CHUNKS INSTEAD*/
	uint16 getHeightDirect(int x, int y) { 
		int chunkX = x / CHUNK_H_RESOLUTION;
		int chunkY = y / CHUNK_H_RESOLUTION;
		return m_Map[getIndex(chunkX, chunkY)].dynDataHeightMap[y % CHUNK_H_RESOLUTION +1][x % CHUNK_H_RESOLUTION + 1].height;
	}

	bool loadFromFile(FString FileName);
	bool loadFromArray(const TArray<uint8>& TFARawData);

	int ApplyCutTerraforming(int x, int y, uint16 targetHeight, const FsTerraformaTemplate& cutHeightmap, uint8 heightmapFactor, uint16 ApplyRangeMin = 0, uint16 ApplyRangeMax = MAX_uint16);
	int ApplyTerraforming(int x, int y, const FsTerraformaTemplate& heightmap, uint8 heightmapFactor, uint16 ApplyRangeMin = 0, uint16 ApplyRangeMax = MAX_uint16);
	void ApplyColoring(int x, int y, const FsTerraformaTemplate& colormap, const FsTerraformaTemplate* terraformedColormap, float colorFactor, uint16 ApplyRangeMin = 0, uint16 ApplyRangeMax = MAX_uint16);




public:
	//static void convertVMPtoTFA(FString Path, FString FileName);
};