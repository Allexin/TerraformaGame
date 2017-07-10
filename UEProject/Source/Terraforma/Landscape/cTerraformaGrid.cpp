#include "cTerraformaGrid.h"
#include "MemoryReader.h"

void packNormal(float x, float y, float z, sChunkNormalData& outNormal) {
	outNormal.x = 128 + (int)(x * 128);
	outNormal.y = 128 + (int)(y * 128);
	outNormal.z = 128 + (int)(z * 128);
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

const uint32 TFA_FILE_VERSION = 2;

bool cTerraformaGrid::loadFromArray(const TArray<uint8>& TFARawData) {
	FMemoryReader Reader(TFARawData);
	uint32 fileVersion;
	Reader.Serialize(&fileVersion, sizeof(uint32));
	if (fileVersion > TFA_FILE_VERSION) {
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
			Reader.Serialize(m_Map[i].dynDataHeightMap, sizeof(sChunkHeightData)*(CHUNK_RESOLUTION + 2)*(CHUNK_RESOLUTION + 2));
			for (int iy = 0; iy<CHUNK_RESOLUTION + 2; iy++)
				for (int ix = 0; ix < CHUNK_RESOLUTION + 2; ix++) {
					uint16 height = m_Map[i].dynDataHeightMap[ix][iy].height;
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
			Reader.Serialize(m_Map[i].dynDataTexture, sizeof(sChunkTextureData)*(CHUNK_RESOLUTION + 2)*(CHUNK_RESOLUTION + 2));

			m_Map[i].TextureChanged = true;
			i++;
		}

	if (fileVersion == 1) {
		for (int y = 0; y<m_Height; y++)
			for (int x = 0; x < m_Width; x++) {
				for (int iy = 0; iy<CHUNK_RESOLUTION+2; iy++)
					for (int ix = 0; ix < CHUNK_RESOLUTION + 2; ix++) {
						m_Map[i].dynDataNormalMap[ix][iy].x = 128;
						m_Map[i].dynDataNormalMap[ix][iy].y = 128;
						m_Map[i].dynDataNormalMap[ix][iy].z = 255;
					}
			}
	}
	else {
		i = 0;
		for (int y = 0; y<m_Height; y++)
			for (int x = 0; x < m_Width; x++) {
				Reader.Serialize(m_Map[i].dynDataNormalMap, sizeof(sChunkNormalData)*(CHUNK_RESOLUTION + 2)*(CHUNK_RESOLUTION + 2));
				i++;
			}
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