#pragma once
#include "sTerraformaTemplate.h"

const int CHUNK_RESOLUTION = 64;
const float CHUNK_TEXTURE_STEP = 1.0f / (CHUNK_RESOLUTION + 2);

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
	bool Changed;
	sTechInfo() {
		Changed = false;
	}
};

struct sTerraformaGridChunk {
	uint16 minHeight;
	uint16 maxHeight;
	int x;
	int y;
	sChunkHeightData dynDataHeightMap[CHUNK_RESOLUTION+2][CHUNK_RESOLUTION + 2];
	sChunkNormalData	dynDataNormalMap[CHUNK_RESOLUTION + 2][CHUNK_RESOLUTION + 2];
	sChunkTextureData	dynDataTexture[CHUNK_RESOLUTION + 2][CHUNK_RESOLUTION + 2];
	sTechInfo			TechData[CHUNK_RESOLUTION + 2][CHUNK_RESOLUTION + 2];
	bool HeightmapChanged;
	bool TextureChanged;

	int ApplyCutTerraforming(int globalX, int globalY, uint16 targetHeight, int dstX, int dstY, int dstWidth, int dstHeight, int srcX, int srcY, const FsTerraformaTemplate& heightmap, uint8 heightmapFactor, uint16 ApplyRangeMin = 0, uint16 ApplyRangeMax = MAX_uint16);
	int ApplyTerraforming(int globalX, int globalY, int dstX, int dstY, int dstWidth, int dstHeight, int srcX, int srcY, const FsTerraformaTemplate& heightmap, uint8 heightmapFactor, const FsTerraformaTemplate* terraformedColormap, float colorFactor, uint16 ApplyRangeMin = 0, uint16 ApplyRangeMax = MAX_uint16);
	void ApplyColoring(int dstX, int dstY, int dstWidth, int dstHeight, int srcX, int srcY, const FsTerraformaTemplate& colormap, uint16 ApplyRangeMin = 0, uint16 ApplyRangeMax = MAX_uint16);
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
		int chunkX = x / CHUNK_RESOLUTION;
		int chunkY = y / CHUNK_RESOLUTION;
		return m_Map[getIndex(chunkX, chunkY)].dynDataHeightMap[y % CHUNK_RESOLUTION +1][x % CHUNK_RESOLUTION + 1].height;
	}

	bool loadFromFile(FString FileName);
	bool loadFromArray(const TArray<uint8>& TFARawData);

	int ApplyCutTerraforming(int x, int y, uint16 targetHeight, const FsTerraformaTemplate& cutHeightmap, uint8 heightmapFactor, uint16 ApplyRangeMin = 0, uint16 ApplyRangeMax = MAX_uint16);
	int ApplyTerraforming(int x, int y, const FsTerraformaTemplate& heightmap, uint8 heightmapFactor, const FsTerraformaTemplate* terraformedColormap, float colorFactor, uint16 ApplyRangeMin = 0, uint16 ApplyRangeMax = MAX_uint16);
	void ApplyColoring(int x, int y, const FsTerraformaTemplate& colormap, uint16 ApplyRangeMin = 0, uint16 ApplyRangeMax = MAX_uint16);




public:
	static void convertVMPtoTFA(FString Path, FString FileName);
};