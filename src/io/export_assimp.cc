/*
 *  OpenSCAD (www.openscad.org)
 *  Copyright (C) 2009-2016 Clifford Wolf <clifford@clifford.at> and
 *                          Marius Kintel <marius@kintel.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  As a special exception, you have permission to link this program
 *  with the CGAL library and distribute executables, as long as you
 *  follow the requirements of the GNU GPL in regard to all of the
 *  software in the executable aside from CGAL.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
../buildOpenSCAD.sh Debug -DCMAKE_MODULE_PATH=$HOME/Downloads && echo 'color("red") cube(); color("blue") translate([1, 0, 0]) sphere();' | ./buildDebug/OpenSCAD.app/Contents/MacOS/OpenSCAD - -o out.3mf --enable=lazy-union --enable=assimp
*/

#include "export.h"
#include "PolySet.h"
#include "PolySetBuilder.h"
#include "PolySetUtils.h"
#include "printutils.h"
#ifdef ENABLE_CGAL
#include "CGALHybridPolyhedron.h"
#include "cgal.h"
#include "cgalutils.h"
#include "CGAL_Nef_polyhedron.h"
#endif
#ifdef ENABLE_MANIFOLD
#include "ManifoldGeometry.h"
#endif

#ifdef ENABLE_ASSIMP

#include <assimp/scene.h>
#include <assimp/Exporter.hpp>

#include <algorithm>

const char *getAiFormatName(FileFormat fileFormat) {
  // Formats supported:
  // - Import: https://assimp-docs.readthedocs.io/en/latest/about/introduction.html
  // - Export: https://assimp-docs.readthedocs.io/en/latest/usage/use_the_lib.html#exporting-models
  switch (fileFormat) {
    case FileFormat::ASCIISTL:
      return "stl";
    case FileFormat::STL:
      return "stlb";
    case FileFormat::OBJ:
      return "obj";
    case FileFormat::_3MF:
      return "3mf";
    case FileFormat::COLLADA:
      return "collada";
    case FileFormat::STP:
      return "stp";
    case FileFormat::PLY:
      return "plyb";
    case FileFormat::GLTF:
      return "glb2";
    case FileFormat::X3D:
      return "x3d";
    default:
      return NULL;
    // case FileFormat::OFF:
  }
}

struct AiSceneBuilder {
  std::map<Color4f, int> colorMaterialMap;
  std::vector<aiMaterial*> materials;
  std::vector<aiMesh*> meshes;

  ~AiSceneBuilder() {
    for (auto material : materials) {
      delete material;
    }
    for (auto mesh : meshes) {
      delete mesh;
    }
  }

  int addColorMaterial(Color4f color) {
    auto it = colorMaterialMap.find(color);
    if (it != colorMaterialMap.end()) {
      return it->second;
    }
    auto material = new aiMaterial();

    aiColor4D diffuse {color[0], color[1], color[2], color[3]};
    material->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_DIFFUSE);
    material->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_SPECULAR);
    material->AddProperty(&diffuse, 1, AI_MATKEY_COLOR_AMBIENT);
    auto i = materials.size();
    materials.push_back(material);
    colorMaterialMap[color] = i;
    return i;
  }

  void addMesh(const PolySet& ps)
  {
    auto mesh = new aiMesh();
    mesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;
    
    if (ps.getColor().isValid()) {
      mesh->mMaterialIndex = addColorMaterial(ps.getColor());
    } else {
      mesh->mMaterialIndex = -1;
    }

    mesh->mNumVertices = ps.vertices.size();
    mesh->mVertices = new aiVector3D[ps.vertices.size()];
    for (int i = 0; i < ps.vertices.size(); i++) {
      mesh->mVertices[i] = aiVector3D(ps.vertices[i][0], ps.vertices[i][1], ps.vertices[i][2]);
    }

    mesh->mNumFaces = ps.indices.size();
    mesh->mFaces = new aiFace[ps.indices.size()];
    for (int i = 0, n = ps.indices.size(); i < n; i++) {
      auto & face = mesh->mFaces[i];
      face.mNumIndices = 3;
      face.mIndices = new unsigned int[3];
      face.mIndices[0] = ps.indices[i][0];
      face.mIndices[1] = ps.indices[i][1];
      face.mIndices[2] = ps.indices[i][2];
    }

    meshes.push_back(mesh);
  }

  std::unique_ptr<aiScene> toScene() {
    auto scene = std::make_unique<aiScene>();

    scene->mMaterials = new aiMaterial*[materials.size()];
    std::copy(materials.begin(), materials.end(), scene->mMaterials);
    scene->mNumMaterials = materials.size();

    scene->mMeshes = new aiMesh*[meshes.size()];
    std::copy(meshes.begin(), meshes.end(), scene->mMeshes);
    scene->mNumMeshes = meshes.size();

    scene->mRootNode = new aiNode();
    auto single_node = true;
    if (single_node) {
      scene->mRootNode->mMeshes = new unsigned int[meshes.size()];
      for (int i = 0; i < meshes.size(); i++) {
        scene->mRootNode->mMeshes[i] = i;
      }
      scene->mRootNode->mNumMeshes = meshes.size();
    } else {
      scene->mRootNode->mNumChildren = meshes.size();
      scene->mRootNode->mChildren = new aiNode*[meshes.size()];
      for (int i = 0; i < meshes.size(); i++) {
        auto node = new aiNode();
        node->mMeshes = new unsigned int[1];
        node->mMeshes[0] = i;
        node->mNumMeshes = 1;
        node->mParent = scene->mRootNode;
        scene->mRootNode->mChildren[i] = node;
      }
    }

    // Reset vectors so we don't delete them in the destructor: they're owned by the aiScene now.
    materials.clear();
    meshes.clear();

    return scene;
  }
};

bool export_assimp(const std::shared_ptr<const Geometry>& geom, std::ostream& output, FileFormat fileFormat)
{
  const char *formatName = getAiFormatName(fileFormat);
  if (!formatName) {
    return false;
  }

  AiSceneBuilder builder;
  std::function<bool(const Geometry &)> append_geom = [&](const Geometry& geom) {
    if (const auto list = dynamic_cast<const GeometryList *>(&geom)) {
      for (const auto& item : list->getChildren()) {
        if (!append_geom(*item.second)) return false;
      }
#ifdef ENABLE_CGAL
    } else if (const auto N = dynamic_cast<const CGAL_Nef_polyhedron *>(&geom)) {
      if (!N->p3) {
        LOG(message_group::Export_Error, "Export failed, empty geometry.");
        return false;
      }
      if (!N->p3->is_simple()) {
        LOG(message_group::Export_Warning, "Exported object may not be a valid 2-manifold and may need repair");
      }
      if (const auto ps = CGALUtils::createPolySetFromNefPolyhedron3(*N->p3)) {
        builder.addMesh(*ps);
        return true;
      }
      return false;
    } else if (const auto hybrid = dynamic_cast<const CGALHybridPolyhedron *>(&geom)) {
      builder.addMesh(*hybrid->toPolySet());
      return true;
#endif
#ifdef ENABLE_MANIFOLD
    } else if (const auto mani = dynamic_cast<const ManifoldGeometry *>(&geom)) {
      for (const auto & ps : mani->toPolySets()) {
        builder.addMesh(*ps);
      }
      return true;
#endif
    } else if (const auto ps = dynamic_cast<const PolySet *>(&geom)) {
      builder.addMesh(*PolySetUtils::tessellate_faces(*ps));
      return true;
    } else if (dynamic_cast<const Polygon2d *>(&geom)) { // NOLINT(bugprone-branch-clone)
      assert(false && "Unexpected 2D geom in 3D model");
    } else { // NOLINT(bugprone-branch-clone)
      assert(false && "Unsupported Geometry class");
    }

    return false;
  };

  append_geom(*geom);
  
  auto scene = builder.toScene();
  ::Assimp::Exporter exporter;
  // exporter.Export(scene.get(), formatName, "out_file.gltf");
  const aiExportDataBlob *blob = exporter.ExportToBlob(scene.get(), formatName);
  if (!blob) {
    LOG(message_group::Export_Error, "Assimp exporter failed: %1$s.", exporter.GetErrorString());
    return false;
  }
  output.write(reinterpret_cast<const char*>(blob->data), blob->size);
  return true;
}

#else // ENABLE_ASSIMP

bool export_assimp(const std::shared_ptr<const Geometry>& geom, std::ostream& output, FileFormat format)
{
  LOG("Export to assimp was not enabled when building the application.");
  return false;
}

#endif // ENABLE_ASSIMP
