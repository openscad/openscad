#pragma once

#include "VertexState.h"

#include <unordered_map>
#include <string>
#include "glview/system-gl.h"

namespace ShaderUtils {

enum class ShaderType {
  NONE,
  EDGE_RENDERING,
  SELECT_RENDERING,
};

std::string loadShaderSource(const std::string& name);

class Shader
{
public:
  Shader(const std::string& vs_str, const std::string& fs_str, ShaderType type);
  Shader(const Shader&) = delete;
  Shader & operator=(const Shader&) = delete;
  ~Shader();

  void use() const;
  void unuse() const;
  void draw(const std::shared_ptr<VertexState> &vertex_state) const;
  GLint attributes(const std::string& name) const;
  void set3f(const std::string& name, GLfloat v0, GLfloat v1, GLfloat v2) const;
  void set1i(const std::string& name, GLint v0) const;

  const ShaderType type;

private:
  GLuint shader_program;
  GLuint vertex_shader;
  GLuint fragment_shader;
};

}  // namespace ShaderUtils
