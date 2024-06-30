#include "cgalutils.h"
#include "import.h"
#include "PolySet.h"
#include "CGALHybridPolyhedron.h"
#include "PolySetBuilder.h"
#include "PolySetUtils.h"
#include "printutils.h"
#include "AST.h"
#include "Feature.h"
#include "manifoldutils.h"

#include <fstream>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>


std::unique_ptr<Geometry> import_assimp(const std::string& filename, const Location& loc) {

  LOG("Importing with Assimp");

  Assimp::Importer importer;

  // In combination w/ aiProcess_RemoveComponent below, skips reading stuff we don't need.
  importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
          aiComponent_NORMALS                  |
          aiComponent_TANGENTS_AND_BITANGENTS  |
          aiComponent_BONEWEIGHTS              |
          aiComponent_ANIMATIONS               |
          aiComponent_TEXTURES                 |
          aiComponent_LIGHTS                   |
          aiComponent_CAMERAS);

  auto scene = importer.ReadFile(filename,
    aiProcess_DropNormals |
    aiProcess_RemoveComponent |
    aiProcess_Triangulate |
    aiProcess_ValidateDataStructure
  );
  if (!scene) {
    LOG(message_group::Error, "Error loading file '%1$s' with Assimp: %2$s", filename, importer.GetErrorString());
    return nullptr;
  }

  std::vector<std::unique_ptr<PolySet>> parts;
  std::function<void(const aiNode*)> visitNode = [&](const aiNode* node) {

    if (node->mNumMeshes) {
      // Each of the meshes may have a different color, and may repeat vertices from the others.
      auto builder = std::make_unique<PolySetBuilder>(3);
      for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        if (!mesh) {
          LOG(message_group::Error, "Error loading mesh %1$d from file '%2$s' with Assimp", i, filename);
          continue;
        }
        bool hasVertexColors = mesh->HasVertexColors(0);

        Color4f meshColor;
        if (mesh->mMaterialIndex < scene->mNumMaterials) {
          const aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
          aiColor4D color;
          if ((material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) ||
              (material->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) ||
              (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)) {
            meshColor = {color.r, color.g, color.b, color.a};
          }
        }
        std::vector<Color4f> vertexColors;
        if (hasVertexColors) {
          vertexColors.resize(mesh->mNumVertices);
          for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
            vertexColors[j] = {mesh->mColors[0][j].r, mesh->mColors[0][j].g, mesh->mColors[0][j].b, mesh->mColors[0][j].a};
          }
        }
        for (unsigned int j = 0; j < mesh->mNumFaces; ++j) {
          const aiFace& face = mesh->mFaces[j];
          if (face.mNumIndices != 3) {
            LOG(message_group::Error, "Error loading face %1$d from mesh %2$d in file '%3$s' with Assimp: only triangular faces are supported", j, i, filename);
            return;
          }
          auto i1 = face.mIndices[0];
          auto i2 = face.mIndices[1];
          auto i3 = face.mIndices[2];
          builder->appendPolygon({
            Vector3d(mesh->mVertices[i1].x, mesh->mVertices[i1].y, mesh->mVertices[i1].z),
            Vector3d(mesh->mVertices[i2].x, mesh->mVertices[i2].y, mesh->mVertices[i2].z),
            Vector3d(mesh->mVertices[i3].x, mesh->mVertices[i3].y, mesh->mVertices[i3].z),
          }, hasVertexColors ? vertexColors[i1] : meshColor);
        }
        auto polySet = builder->build();
        if (!polySet) {
          LOG(message_group::Error, "Error building PolySet from mesh %1$d in file '%2$s' with Assimp", i, filename);
          continue;
        }
        parts.push_back(std::move(polySet));
      }
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
      visitNode(node->mChildren[i]);
    }
  };

  visitNode(scene->mRootNode);

  if (parts.size() == 1) {
    return std::move(parts[0]);
  } else {
    Geometry::Geometries geoms;
    for (auto& solid : parts) {
      geoms.emplace_back(nullptr, std::move(solid));
    }

    return std::make_unique<GeometryList>(geoms);
  }
}
