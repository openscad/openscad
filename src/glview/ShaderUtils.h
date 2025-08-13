#pragma once

#include <unordered_map>
#include <string>
#include "glview/system-gl.h"

namespace ShaderUtils {

enum class ShaderType {
  NONE,
  EDGE_RENDERING,
  SELECT_RENDERING,
};

struct ShaderResource {
  GLuint shader_program;
  GLuint vertex_shader;
  GLuint fragment_shader;
};

// Shader attribute identifiers
struct ShaderInfo {
  ShaderResource resource;
  ShaderType type;
  std::unordered_map<std::string, int> uniforms;
  std::unordered_map<std::string, int> attributes;
};

std::string loadShaderSource(const std::string& name);
ShaderResource compileShaderProgram(const std::string& vs_str, const std::string& fs_str);

}  // namespace ShaderUtils
