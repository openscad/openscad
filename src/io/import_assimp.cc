#include "import.h"
#include "PolySet.h"
#include "PolySetBuilder.h"
#include "printutils.h"
#include "AST.h"

#include <fstream>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>


std::unique_ptr<Geometry> import_assimp(const std::string& filename, const Location& loc) {

  Assimp::Importer importer;

  // In combination w/ aiProcess_RemoveComponent below, skips reading stuff we don't need.
  importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS,
          aiComponent_NORMALS                  |
          aiComponent_TANGENTS_AND_BITANGENTS  |
          aiComponent_BONEWEIGHTS              |
          aiComponent_ANIMATIONS               |
          // aiComponent_TEXTURES                 |
          aiComponent_LIGHTS                   |
          aiComponent_CAMERAS);

  // And have it read the given file with some example postprocessing
  // Usually - if speed is not the most important aspect for you - you'll
  // probably to request more postprocessing than we do in this example.
  auto scene = importer.ReadFile(filename,
    // aiProcess_JoinIdenticalVertices  |
    // aiProcess_DropNormals |
    aiProcess_RemoveComponent |
    aiProcess_Triangulate |
    aiProcess_ValidateDataStructure
  );
  if (!scene) {
    LOG(message_group::Error, "Error loading file '%1$s' with Assimp: %2$s", filename, importer.GetErrorString());
    return nullptr;
  }
  std::function<std::unique_ptr<Geometry>(const aiNode*)> processNode = [&](const aiNode* node) -> std::unique_ptr<Geometry> {
    std::vector<std::unique_ptr<Geometry>> children;

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
        if (mesh->mMaterialIndex >= 0) {
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
            return std::unique_ptr<Geometry>();
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
        children.push_back(std::move(polySet));
      }
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
      auto childGeom = processNode(node->mChildren[i]);
      if (childGeom) {
        children.push_back(std::move(childGeom));
      }
    }
    
    if (children.empty()) {
      return std::unique_ptr<Geometry>();
    } else if (children.size() == 1) {
      return std::move(children[0]);
    } else {
      Geometry::Geometries gchildren;
      for (auto& child : children) {
        gchildren.emplace_back(nullptr, std::move(child));
      }
      return std::make_unique<GeometryList>(gchildren);
    }
  };

  return processNode(scene->mRootNode);
}