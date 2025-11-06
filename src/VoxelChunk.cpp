#include "VoxelChunk.h"
#include <glad/glad.h>
#include <algorithm>

VoxelChunk::VoxelChunk(int chunkX, int chunkY, int chunkZ)
    : chunkPosition(chunkX, chunkY, chunkZ)
    , VAO(0)
    , VBO(0)
    , EBO(0)
    , glInitialized(false)
{
    // Initialisiere alle Blöcke als Luft
    blocks.fill(BlockType::Air);
}

VoxelChunk::~VoxelChunk() {
  cleanup();
}

void VoxelChunk::cleanup() {
    if (glInitialized) {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
     glInitialized = false;
 }
}

int VoxelChunk::getIndex(int x, int y, int z) const {
    // Bounds checking
  if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE || z < 0 || z >= CHUNK_SIZE) {
   return -1;
    }
    return x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
}

void VoxelChunk::setBlock(int x, int y, int z, BlockType type) {
    int index = getIndex(x, y, z);
    if (index >= 0) {
        blocks[index] = type;
    }
}

BlockType VoxelChunk::getBlock(int x, int y, int z) const {
    int index = getIndex(x, y, z);
    if (index < 0) {
        return BlockType::Air;
    }
    return blocks[index];
}

bool VoxelChunk::isBlockSolid(int x, int y, int z) const {
    BlockType type = getBlock(x, y, z);
    return type != BlockType::Air;
}

bool VoxelChunk::isBlockSolidWithNeighbor(int x, int y, int z,
 const VoxelChunk* north, const VoxelChunk* south,
    const VoxelChunk* east, const VoxelChunk* west,
    const VoxelChunk* top, const VoxelChunk* bottom) const 
{
    // Innerhalb des Chunks
    if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE) {
        return isBlockSolid(x, y, z);
    }
    
    // Prüfe Nachbar-Chunks
    if (x < 0 && west) {
        return west->isBlockSolid(CHUNK_SIZE - 1, y, z);
    }
    if (x >= CHUNK_SIZE && east) {
    return east->isBlockSolid(0, y, z);
    }
    if (y < 0 && bottom) {
        return bottom->isBlockSolid(x, CHUNK_SIZE - 1, z);
    }
    if (y >= CHUNK_SIZE && top) {
      return top->isBlockSolid(x, 0, z);
    }
    if (z < 0 && south) {
        return south->isBlockSolid(x, y, CHUNK_SIZE - 1);
    }
    if (z >= CHUNK_SIZE && north) {
        return north->isBlockSolid(x, y, 0);
 }
  
    return false;
}

glm::vec2 VoxelChunk::getAtlasUV(int atlasX, int atlasY) const {
    float u = static_cast<float>(atlasX) / ATLAS_SIZE;
    float v = static_cast<float>(atlasY) / ATLAS_SIZE;
    return glm::vec2(u, v);
}

BlockTextures VoxelChunk::getBlockTextures(BlockType type, FaceDirection face) const {
    BlockTextures textures;
    float texelSize = 1.0f / ATLAS_SIZE;
    
    // Definiere Textur-Positionen im Atlas für jeden Block-Typ
    // Format: [atlasX, atlasY] für jede Seite
    int atlasX = 0, atlasY = 0;
    
    switch (type) {
        case BlockType::Stone:
   atlasX = 0; atlasY = 0;
            break;
        case BlockType::Grass:
            if (face == FaceDirection::Top) {
  atlasX = 1; atlasY = 0; // Gras-Textur oben
            } else if (face == FaceDirection::Bottom) {
        atlasX = 2; atlasY = 0; // Erde unten
            } else {
                atlasX = 3; atlasY = 0; // Gras-Seite
      }
       break;
  case BlockType::Dirt:
      atlasX = 2; atlasY = 0;
          break;
        case BlockType::Wood:
            if (face == FaceDirection::Top || face == FaceDirection::Bottom) {
 atlasX = 4; atlasY = 0; // Holz-Ringe
            } else {
 atlasX = 5; atlasY = 0; // Holz-Rinde
            }
            break;
      case BlockType::Sand:
       atlasX = 6; atlasY = 0;
      break;
        case BlockType::Water:
atlasX = 7; atlasY = 0;
            break;
        default:
            atlasX = 0; atlasY = 0;
   break;
    }
 
    // Berechne UV-Koordinaten
    glm::vec2 uvMin = getAtlasUV(atlasX, atlasY);
    glm::vec2 uvMax = getAtlasUV(atlasX + 1, atlasY + 1);
    
    // Setze UV-Koordinaten für alle Seiten
    textures.top[0] = uvMin;
    textures.top[1] = uvMax;
    textures.bottom[0] = uvMin;
    textures.bottom[1] = uvMax;
    textures.north[0] = uvMin;
    textures.north[1] = uvMax;
    textures.south[0] = uvMin;
    textures.south[1] = uvMax;
    textures.east[0] = uvMin;
    textures.east[1] = uvMax;
    textures.west[0] = uvMin;
    textures.west[1] = uvMax;
    
    return textures;
}

void VoxelChunk::addFace(FaceDirection direction, int x, int y, int z, BlockType type) {
    BlockTextures textures = getBlockTextures(type, direction);
    
 // Berechne Weltposition
  float worldX = static_cast<float>(chunkPosition.x * CHUNK_SIZE + x);
    float worldY = static_cast<float>(chunkPosition.y * CHUNK_SIZE + y);
    float worldZ = static_cast<float>(chunkPosition.z * CHUNK_SIZE + z);
 
    unsigned int baseIndex = static_cast<unsigned int>(vertices.size() / 8);
    
    glm::vec2 uvMin, uvMax;
    glm::vec3 normal;
    
    // Vertex-Positionen und UVs basierend auf der Richtung
    switch (direction) {
    case FaceDirection::Top: // +Y
    uvMin = textures.top[0];
      uvMax = textures.top[1];
         normal = glm::vec3(0, -1, 0);  // Invertiert: war (0, 1, 0)
    
            // Vertex 0: (-0.5, 0.5, -0.5)
        // Format: x,y,z, nx,ny,nz, u,v
          vertices.insert(vertices.end(), {
   worldX - 0.5f, worldY + 0.5f, worldZ - 0.5f,
   normal.x, normal.y, normal.z,
    uvMin.x, uvMax.y
     });
            // Vertex 1: (0.5, 0.5, -0.5)
         vertices.insert(vertices.end(), {
  worldX + 0.5f, worldY + 0.5f, worldZ - 0.5f,
           normal.x, normal.y, normal.z,
 uvMax.x, uvMax.y
    });
       // Vertex 2: (0.5, 0.5, 0.5)
     vertices.insert(vertices.end(), {
       worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f,
  normal.x, normal.y, normal.z,
    uvMax.x, uvMin.y
 });
     // Vertex 3: (-0.5, 0.5, 0.5)
   vertices.insert(vertices.end(), {
           worldX - 0.5f, worldY + 0.5f, worldZ + 0.5f,
     normal.x, normal.y, normal.z,
          uvMin.x, uvMin.y
     });
 break;
  
        case FaceDirection::Bottom: // -Y
    uvMin = textures.bottom[0];
          uvMax = textures.bottom[1];
  normal = glm::vec3(0, 1, 0);  // Invertiert: war (0, -1, 0)
   
  vertices.insert(vertices.end(), {
 worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f,
        normal.x, normal.y, normal.z,
     uvMin.x, uvMin.y
            });
 vertices.insert(vertices.end(), {
    worldX + 0.5f, worldY - 0.5f, worldZ + 0.5f,
       normal.x, normal.y, normal.z,
    uvMax.x, uvMin.y
   });
            vertices.insert(vertices.end(), {
        worldX + 0.5f, worldY - 0.5f, worldZ - 0.5f,
   normal.x, normal.y, normal.z,
      uvMax.x, uvMax.y
            });
        vertices.insert(vertices.end(), {
     worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f,
         normal.x, normal.y, normal.z,
 uvMin.x, uvMax.y
            });
 break;
    
case FaceDirection::North: // +Z
      uvMin = textures.north[0];
    uvMax = textures.north[1];
 normal = glm::vec3(0, 0, -1);  // Invertiert: war (0, 0, 1)
   
   vertices.insert(vertices.end(), {
 worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f,
   normal.x, normal.y, normal.z,
     uvMin.x, uvMax.y
   });
vertices.insert(vertices.end(), {
          worldX - 0.5f, worldY + 0.5f, worldZ + 0.5f,
        normal.x, normal.y, normal.z,
             uvMin.x, uvMin.y
     });
     vertices.insert(vertices.end(), {
      worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f,
         normal.x, normal.y, normal.z,
     uvMax.x, uvMin.y
         });
      vertices.insert(vertices.end(), {
    worldX + 0.5f, worldY - 0.5f, worldZ + 0.5f,
     normal.x, normal.y, normal.z,
     uvMax.x, uvMax.y
       });
     break;
 
  case FaceDirection::South: // -Z
 uvMin = textures.south[0];
         uvMax = textures.south[1];
     normal = glm::vec3(0, 0, 1);  // Invertiert: war (0, 0, -1)
   
            vertices.insert(vertices.end(), {
       worldX + 0.5f, worldY - 0.5f, worldZ - 0.5f,
    normal.x, normal.y, normal.z,
           uvMin.x, uvMax.y
       });
      vertices.insert(vertices.end(), {
 worldX + 0.5f, worldY + 0.5f, worldZ - 0.5f,
normal.x, normal.y, normal.z,
    uvMin.x, uvMin.y
 });
 vertices.insert(vertices.end(), {
       worldX - 0.5f, worldY + 0.5f, worldZ - 0.5f,
      normal.x, normal.y, normal.z,
         uvMax.x, uvMin.y
    });
     vertices.insert(vertices.end(), {
           worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f,
           normal.x, normal.y, normal.z,
    uvMax.x, uvMax.y
    });
            break;
   
  case FaceDirection::East: // +X
   uvMin = textures.east[0];
uvMax = textures.east[1];
        normal = glm::vec3(-1, 0, 0);  // Invertiert: war (1, 0, 0)
  
        vertices.insert(vertices.end(), {
  worldX + 0.5f, worldY - 0.5f, worldZ + 0.5f,
    normal.x, normal.y, normal.z,
       uvMin.x, uvMax.y
         });
  vertices.insert(vertices.end(), {
 worldX + 0.5f, worldY + 0.5f, worldZ + 0.5f,
        normal.x, normal.y, normal.z,
 uvMin.x, uvMin.y
  });
         vertices.insert(vertices.end(), {
  worldX + 0.5f, worldY + 0.5f, worldZ - 0.5f,
   normal.x, normal.y, normal.z,
          uvMax.x, uvMin.y
      });
          vertices.insert(vertices.end(), {
        worldX + 0.5f, worldY - 0.5f, worldZ - 0.5f,
 normal.x, normal.y, normal.z,
 uvMax.x, uvMax.y
            });
   break;
       
  case FaceDirection::West: // -X
            uvMin = textures.west[0];
          uvMax = textures.west[1];
      normal = glm::vec3(1, 0, 0);  // Invertiert: war (-1, 0, 0)
 
  vertices.insert(vertices.end(), {
                worldX - 0.5f, worldY - 0.5f, worldZ - 0.5f,
    normal.x, normal.y, normal.z,
          uvMin.x, uvMax.y
         });
            vertices.insert(vertices.end(), {
  worldX - 0.5f, worldY + 0.5f, worldZ - 0.5f,
     normal.x, normal.y, normal.z,
        uvMin.x, uvMin.y
    });
            vertices.insert(vertices.end(), {
     worldX - 0.5f, worldY + 0.5f, worldZ + 0.5f,
  normal.x, normal.y, normal.z,
            uvMax.x, uvMin.y
   });
       vertices.insert(vertices.end(), {
        worldX - 0.5f, worldY - 0.5f, worldZ + 0.5f,
        normal.x, normal.y, normal.z,
       uvMax.x, uvMax.y
    });
    break;
    }
 
    // Füge Indices für zwei Dreiecke hinzu (Quad)
    indices.insert(indices.end(), {
        baseIndex, baseIndex + 1, baseIndex + 2,
 baseIndex, baseIndex + 2, baseIndex + 3
    });
}

void VoxelChunk::generateMesh() {
    generateMeshWithNeighbors(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}

void VoxelChunk::generateMeshWithNeighbors(
    const VoxelChunk* north,
    const VoxelChunk* south,
    const VoxelChunk* east,
    const VoxelChunk* west,
    const VoxelChunk* top,
    const VoxelChunk* bottom)
{
    // Lösche alte Mesh-Daten
    vertices.clear();
    indices.clear();
    
    // Iteriere über alle Blöcke im Chunk
  for (int y = 0; y < CHUNK_SIZE; ++y) {
        for (int z = 0; z < CHUNK_SIZE; ++z) {
            for (int x = 0; x < CHUNK_SIZE; ++x) {
   BlockType blockType = getBlock(x, y, z);
      
   // Überspringe Luft-Blöcke
      if (blockType == BlockType::Air) {
          continue;
  }
  
          // Face Culling: Füge nur Faces hinzu, die an Luft grenzen
 // Top Face (+Y)
         if (!isBlockSolidWithNeighbor(x, y + 1, z, north, south, east, west, top, bottom)) {
       addFace(FaceDirection::Top, x, y, z, blockType);
       }
        
     // Bottom Face (-Y)
      if (!isBlockSolidWithNeighbor(x, y - 1, z, north, south, east, west, top, bottom)) {
             addFace(FaceDirection::Bottom, x, y, z, blockType);
      }
       
        // North Face (+Z)
  if (!isBlockSolidWithNeighbor(x, y, z + 1, north, south, east, west, top, bottom)) {
           addFace(FaceDirection::North, x, y, z, blockType);
     }
           
 // South Face (-Z)
      if (!isBlockSolidWithNeighbor(x, y, z - 1, north, south, east, west, top, bottom)) {
            addFace(FaceDirection::South, x, y, z, blockType);
         }
          
 // East Face (+X)
if (!isBlockSolidWithNeighbor(x + 1, y, z, north, south, east, west, top, bottom)) {
               addFace(FaceDirection::East, x, y, z, blockType);
         }
    
        // West Face (-X)
            if (!isBlockSolidWithNeighbor(x - 1, y, z, north, south, east, west, top, bottom)) {
   addFace(FaceDirection::West, x, y, z, blockType);
         }
            }
        }
    }
}

void VoxelChunk::setupOpenGL() {
    if (vertices.empty()) {
        return;
    }
    
    // Lösche alte OpenGL-Objekte falls vorhanden
    if (glInitialized) {
        cleanup();
  }
 
    // Generiere VAO, VBO, EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    // Lade Vertex-Daten
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
 // Lade Index-Daten
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    // Vertex-Format: x,y,z, nx,ny,nz, u,v (8 floats pro Vertex)
 
    // Vertex-Attribute 0: Position (x,y,z)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    
    // Vertex-Attribute 1: Normal (nx,ny,nz)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    
    // Vertex-Attribute 2: TexCoords (u,v)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    
    glBindVertexArray(0);
    
    glInitialized = true;
}

void VoxelChunk::render() const {
    if (!glInitialized || indices.empty()) {
        return;
    }
    
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

bool VoxelChunk::isEmpty() const {
    return std::all_of(blocks.begin(), blocks.end(), 
        [](BlockType type) { return type == BlockType::Air; });
}

void VoxelChunk::clear() {
    blocks.fill(BlockType::Air);
  vertices.clear();
    indices.clear();
}

void VoxelChunk::fill(BlockType type) {
    blocks.fill(type);
}
