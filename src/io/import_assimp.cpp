#include "import.h"
#include "PolySet.h"
#include "PolySetBuilder.h"
#include "printutils.h"
#include "AST.h"

#include <fstream>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>      // C++ importer interface


#if !defined(BOOST_ENDIAN_BIG_BYTE_AVAILABLE) && !defined(BOOST_ENDIAN_LITTLE_BYTE_AVAILABLE)
#error Byte order undefined or unknown. Currently only BOOST_ENDIAN_BIG_BYTE and BOOST_ENDIAN_LITTLE_BYTE are supported.
#endif


std::unique_ptr<PolySet> import_assimp(const std::string& filename, const Location& loc) {

  Assimp::Importer importer;

  // And have it read the given file with some example postprocessing
  // Usually - if speed is not the most important aspect for you - you'll
  // probably to request more postprocessing than we do in this example.
  auto scene = std::unique_ptr<aiScene>(importer.ReadFile(filename,
    aiProcess_RemoveComponent       |
    aiProcess_Triangulate            |
    aiProcess_JoinIdenticalVertices  |
    aiProcess_ValidateDataStructure  |
    aiProcess_DropNormals
  );
  if (!scene) {
    LOG(message_group::Error, "Error loading file '%1$s' with Assimp: %2$s", filename, importer.GetErrorString());
    return nullptr;
  }
  // Create a GeometryList to hold the imported geometry
  auto geomList = std::make_unique<GeometryList>();

  std::function<std::make_unique<Geometry>(const aiNode*)> processNode = [&](const aiNode* node) {
    auto geom = std::make_unique<GeometryList>();
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
      const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
      if (!mesh) {
        LOG(message_group::Error, "Error loading mesh %1$d from file '%2$s' with Assimp", i, filename);
        continue;
      }

      // Create a PolySetBuilder to hold the mesh data
      auto builder = std::make_unique<PolySetBuilder>(3);

      // Iterate over all vertices in the mesh
      for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
        builder->addVertex(Vector3d(mesh->mVertices[j].x, mesh->mVertices[j].y, mesh->mVertices[j].z));
      }

      // Iterate over all faces in the mesh
      for (unsigned int j = 0; j < mesh->mNumFaces; ++j) {
        const aiFace& face = mesh->mFaces[j];
        if (face.mNumIndices != 3) {
          LOG(message_group::Error, "Error loading face %1$d from mesh %2$d in file '%3$s' with Assimp: only triangular faces are supported", j, i, filename);
          return nullptr;
        }
        builder->addFace(face.mIndices[0], face.mIndices[1], face.mIndices[2]);
      }

      // Build the PolySet from the PolySetBuilder
      auto polySet = builder->build();
      if (!polySet) {
        LOG(message_group::Error, "Error building PolySet from mesh %1$d in file '%2$s' with Assimp", i, filename);
        continue;
      }

      geom->children.push_back(std::move(polySet));
    }
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
      auto childGeom = processNode(node->mChildren[i]);
      if (childGeom) {
        geom->children.push_back(std::move(childGeom));
      }
    }
    return geom;
  };

  return processNode(scene->mRootNode);
}