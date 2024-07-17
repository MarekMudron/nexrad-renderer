#pragma once

#include<vector>
#include<string>
#include<iostream>
#include<glm/glm.hpp>

struct Texture{
  uint8_t const* data     = nullptr;///< pointer to data
  uint32_t       width    = 0      ;///< width of the texture
  uint32_t       height   = 0      ;///< height of the texture
  uint32_t       channels = 3      ;///< number of channels of the texture
};

enum class IndexType{
  UINT8  = 1, ///< uin8_t type
  UINT16 = 2, ///< uin16_t type
  UINT32 = 4, ///< uint32_t type
};

enum class AttributeType{
  EMPTY = 0, ///< disabled attribute
  FLOAT = 1, ///< 1x 32-bit float
  VEC2  = 2, ///< 2x 32-bit floats
  VEC3  = 3, ///< 3x 32-bit floats
  VEC4  = 4, ///< 4x 32-bit floats
};

struct VertexAttrib{
  int32_t       bufferId      = -1;
  uint64_t      stride        =  0;///< stride in bytes
  uint64_t      offset        =  0;///< offset in bytes
  AttributeType type          =  AttributeType::EMPTY;///< type of attribute
};

/**
 * @brief This struct represents a mesh
 */
//! [Mesh]
struct Mesh{
  int32_t      indexBuffer =                -1;
  uint64_t     indexOffset =                 0;
  IndexType    indexType   = IndexType::UINT32;
  uint32_t     nofIndices  =                 0;

  VertexAttrib position                       ;///< position vertex attribute
  VertexAttrib normal                         ;///< normal vertex attribute
  VertexAttrib texCoord                       ;///< tex. coord vertex attribute
  glm::vec4    diffuseColor = glm::vec4(1.f)  ;///< default diffuseColor (if there is no texture)
  int          diffuseTexture = -1            ;///< diffuse texture or -1 (no texture)
};
//! [Mesh]

struct Buffer{
  void*  ptr  = nullptr;
  size_t size = 0      ;
};

/**
 * @brief This structure represents node in tree structure of model
 */
//! [Node]
struct Node{
  glm::mat4        modelMatrix = glm::mat4(1.f);///< model transformation matrix
  int32_t          mesh = -1;                   ///< id of mesh or -1 if no mesh
  std::vector<Node>children;                    ///< list of children nodes
};
//! [Node]

/**
 * @brief This struct represent model
 */
//! [Model]
struct Model{
  std::vector<Mesh   >meshes  ;///< list of all meshes in model
  std::vector<Node   >roots   ;///< list of roots of node trees
  std::vector<Texture>textures;///< list of all textures in model
  std::vector<Buffer >buffers ;
};
//! [Model]

class ModelDataImpl;
class ModelData{
  public:
    ModelData();
    void load(std::string const&fileName);
    ~ModelData();
    Model getModel();
  private:
    friend class ModelDataImpl;
    ModelDataImpl*impl = nullptr;
};
