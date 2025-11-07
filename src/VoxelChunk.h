#ifndef VOXEL_CHUNK_H
#define VOXEL_CHUNK_H

#include <vector>
#include <array>
#include <cstdint>
#include <glm/glm.hpp>

// Block-Typen
enum class BlockType : uint8_t {
    Air = 0,
    Stone = 1,
    Grass = 2,
  Dirt = 3,
    Wood = 4,
    Sand = 5,
 Water = 6
};

// Struktur für Texturkoordinaten pro Blockseite
struct BlockTextures {
    glm::vec2 top[2];      // Min/Max UV für obere Seite
    glm::vec2 bottom[2];   // Min/Max UV für untere Seite
    glm::vec2 north[2];    // Min/Max UV für Nord-Seite
    glm::vec2 south[2];    // Min/Max UV für Süd-Seite
    glm::vec2 east[2];     // Min/Max UV für Ost-Seite
  glm::vec2 west[2];     // Min/Max UV für West-Seite
};

// Richtungen für Face Culling
enum class FaceDirection {
    Top = 0,
    Bottom,
    North,
    South,
    East,
    West
};

class VoxelChunk {
public:
    static constexpr int CHUNK_SIZE = 16;
    static constexpr int CHUNK_VOLUME = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

    VoxelChunk(int chunkX, int chunkY, int chunkZ);
    ~VoxelChunk();

    // Block-Verwaltung
    void setBlock(int x, int y, int z, BlockType type);
    BlockType getBlock(int x, int y, int z) const;
    
    // Mesh-Generierung
    void generateMesh();
    void generateMeshWithNeighbors(
        const VoxelChunk* north,
        const VoxelChunk* south,
        const VoxelChunk* east,
        const VoxelChunk* west,
        const VoxelChunk* top,
     const VoxelChunk* bottom
    );

    // Daten-Zugriff
    const std::vector<float>& getVertices() const { return vertices; }
    const std::vector<unsigned int>& getIndices() const { return indices; }
    
    // Chunk-Position
    glm::ivec3 getChunkPosition() const { return chunkPosition; }
    
    // OpenGL Setup
    void setupOpenGL();
    void render() const;
    void cleanup();

    // Hilfsfunktionen
    bool isEmpty() const;
    void clear();
    void fill(BlockType type);

private:
    // 3D-Array für Block-Daten (flach gespeichert)
    std::array<BlockType, CHUNK_VOLUME> blocks;
    
    // Mesh-Daten
    std::vector<float> vertices;        // Format: x,y,z, u,v, nx,ny,nz
    std::vector<unsigned int> indices;
    
  // OpenGL-Objekte
    unsigned int VAO, VBO, EBO;
bool glInitialized;
    
  // Chunk-Position in der Welt
    glm::ivec3 chunkPosition;

    // Hilfsfunktionen
    int getIndex(int x, int y, int z) const;
    bool isBlockSolid(int x, int y, int z) const;
    bool isBlockSolidWithNeighbor(int x, int y, int z, 
        const VoxelChunk* north, const VoxelChunk* south,
        const VoxelChunk* east, const VoxelChunk* west,
        const VoxelChunk* top, const VoxelChunk* bottom) const;
    
    // Face-Generierung
    void addFace(FaceDirection direction, int x, int y, int z, BlockType type);
    BlockTextures getBlockTextures(BlockType type, FaceDirection face) const;
    
    // Textur-Atlas Konfiguration (4x4 Texturen im Atlas, 1024x1024px, 256x256px pro Tile)
    static constexpr float ATLAS_SIZE = 4.0f;
  glm::vec2 getAtlasUV(int atlasX, int atlasY) const;
};

#endif // VOXEL_CHUNK_H
