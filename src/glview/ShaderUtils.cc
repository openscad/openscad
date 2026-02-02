#include "glview/ShaderUtils.h"

#include "VertexState.h"

#include <cstdio>
#include <sstream>
#include <string>
#include <fstream>

#include "utils/printutils.h"
#include "platform/PlatformUtils.h"

namespace ShaderUtils {

std::string loadShaderSource(const std::string& name)
{
  std::string shaderPath = (PlatformUtils::resourcePath("shaders") / name).string();
  std::ostringstream buffer;
  const std::ifstream f(shaderPath);
  if (f.is_open()) {
    buffer << f.rdbuf();
  } else {
    LOG(message_group::UI_Error, "Cannot open shader source file: '%1$s'", shaderPath);
  }
  return buffer.str();
}

Shader::Shader(const std::string& vs_str, const std::string& fs_str, const ShaderType type) : type(type)
{
  int shaderstatus;
  const std::string vs = loadShaderSource(vs_str);
  const std::string fs = loadShaderSource(fs_str);
  const char *vs_source = vs.c_str();
  const char *fs_source = fs.c_str();
  // Compile the shaders
  GL_CHECKD(vertex_shader = glCreateShader(GL_VERTEX_SHADER));
  glShaderSource(vertex_shader, 1, (const GLchar **)&vs_source, nullptr);
  glCompileShader(vertex_shader);
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &shaderstatus);
  if (shaderstatus != GL_TRUE) {
    int loglen;
    char logbuffer[1000];
    glGetShaderInfoLog(vertex_shader, sizeof(logbuffer), &loglen, logbuffer);
    // FIXME: Use OpenCAD log to error instead of stderr
    fprintf(stderr, __FILE__ " %s: OpenGL vertex shader Error:\n%.*s\n\n", vs_str.c_str(), loglen, logbuffer);
    return;
  }

  GL_CHECKD(fragment_shader = glCreateShader(GL_FRAGMENT_SHADER));
  glShaderSource(fragment_shader, 1, (const GLchar **)&fs_source, nullptr);
  glCompileShader(fragment_shader);
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &shaderstatus);
  if (shaderstatus != GL_TRUE) {
    int loglen;
    char logbuffer[1000];
    glGetShaderInfoLog(fragment_shader, sizeof(logbuffer), &loglen, logbuffer);
    // FIXME: Use OpenCAD log to error instead of stderr
    fprintf(stderr, __FILE__ " %s: OpenGL fragment shader Error:\n%.*s\n\n", fs_str.c_str(), loglen, logbuffer);
    return;
  }

  // Link
  shader_program = glCreateProgram();
  glAttachShader(shader_program, vertex_shader);
  glAttachShader(shader_program, fragment_shader);
  GL_CHECKD(glLinkProgram(shader_program));

  GLint status;
  glGetProgramiv(shader_program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    int loglen;
    char logbuffer[1000];
    glGetProgramInfoLog(shader_program, sizeof(logbuffer), &loglen, logbuffer);
    // FIXME: Use OpenCAD log to error instead of stderr
    fprintf(stderr, __FILE__ ": OpenGL Program Linker Error:\n%.*s\n\n", loglen, logbuffer);
    return;
  }

  glValidateProgram(shader_program);
  glGetProgramiv(shader_program, GL_VALIDATE_STATUS, &status);
  if (!status) {
    int loglen;
    char logbuffer[1000];
    glGetProgramInfoLog(shader_program, sizeof(logbuffer), &loglen, logbuffer);
    // FIXME: Use OpenCAD log to error instead of stderr
    fprintf(stderr, __FILE__ ": OpenGL Program Validation results:\n%.*s\n\n", loglen, logbuffer);
    return;
  }
}

Shader::~Shader()
{
  if (shader_program) {
    glDeleteProgram(shader_program);
  }
  if (vertex_shader) {
    glDeleteShader(vertex_shader);
  }
  if (fragment_shader) {
    glDeleteShader(fragment_shader);
  }
}

void Shader::use() const
{
  glUseProgram(shader_program);
  if (type == ShaderType::EDGE_RENDERING)
    glEnableVertexAttribArray(attributes("barycentric"));
}

void Shader::unuse() const
{
  if (type == ShaderType::EDGE_RENDERING)
    glDisableVertexAttribArray(attributes("barycentric"));
  glUseProgram(0);
}

GLint Shader::attributes(const std::string& name) const
{
  return glGetAttribLocation(shader_program, name.c_str());
}

// TODO: template this?
void Shader::set3f(const std::string& name, GLfloat v0, GLfloat v1, GLfloat v2) const
{
  glUniform3f(glGetUniformLocation(shader_program, name.c_str()), v0, v1, v2);
}

void Shader::set1i(const std::string& name, GLint v0) const
{
  glUniform1i(glGetUniformLocation(shader_program, name.c_str()), v0);
}

}  // namespace ShaderUtils
