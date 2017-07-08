#pragma once

const int CHUNK_RESOLUTION = 64;
const float CHUNK_TEXTURE_STEP = 1.0f / (CHUNK_RESOLUTION + 2);

struct sChunkHeightmapData {
	uint16 height;
};

struct sChunkTextureData {
	uint8 red;
	uint8 green;
	uint8 blue;
	uint8 reserved;
};

struct sTerraformaGridChunk {
	uint16 minHeight;
	uint16 maxHeight;
	int x;
	int y;
	sChunkHeightmapData dynDataHeightMap[CHUNK_RESOLUTION+2][CHUNK_RESOLUTION + 2];
	sChunkTextureData	dynDataTexture[CHUNK_RESOLUTION + 2][CHUNK_RESOLUTION + 2];
	bool HeightmapChanged;
	bool TextureChanged;
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
	sTerraformaGridChunk* getData(int x, int y) { return &m_Map[getIndex(x,y)]; }
	sTerraformaGridChunk* getData(int index) { return &m_Map[index]; }

	bool loadFromFile(FString FileName);

	static void convertVMPtoTFA(FString Path, FString FileName);
};