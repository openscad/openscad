#include "glview/ShaderUtils.h"

#include <sstream>
#include <string>
#include <fstream>

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

ShaderResource compileShaderProgram(const std::string& vs_str, const std::string& fs_str)
{
  int shaderstatus;
  const char *vs_source = vs_str.c_str();
  const char *fs_source = fs_str.c_str();
  // Compile the shaders
  GL_CHECKD(auto vertex_shader = glCreateShader(GL_VERTEX_SHADER));
  glShaderSource(vertex_shader, 1, (const GLchar **)&vs_source, nullptr);
  glCompileShader(vertex_shader);
  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &shaderstatus);
  if (shaderstatus != GL_TRUE) {
    int loglen;
    char logbuffer[1000];
    glGetShaderInfoLog(vertex_shader, sizeof(logbuffer), &loglen, logbuffer);
    // FIXME: Use OpenCAD log to error instead of stderr
    fprintf(stderr, __FILE__ ": OpenGL vertex shader Error:\n%.*s\n\n", loglen, logbuffer);
    return {};
  }

  GL_CHECKD(auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER));
  glShaderSource(fragment_shader, 1, (const GLchar **)&fs_source, nullptr);
  glCompileShader(fragment_shader);
  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &shaderstatus);
  if (shaderstatus != GL_TRUE) {
    int loglen;
    char logbuffer[1000];
    glGetShaderInfoLog(fragment_shader, sizeof(logbuffer), &loglen, logbuffer);
    // FIXME: Use OpenCAD log to error instead of stderr
    fprintf(stderr, __FILE__ ": OpenGL fragment shader Error:\n%.*s\n\n", loglen, logbuffer);
    return {};
  }

  // Link
  auto shader_prog = glCreateProgram();
  glAttachShader(shader_prog, vertex_shader);
  glAttachShader(shader_prog, fragment_shader);
  GL_CHECKD(glLinkProgram(shader_prog));

  GLint status;
  glGetProgramiv(shader_prog, GL_LINK_STATUS, &status);
  if (status == GL_FALSE) {
    int loglen;
    char logbuffer[1000];
    glGetProgramInfoLog(shader_prog, sizeof(logbuffer), &loglen, logbuffer);
    // FIXME: Use OpenCAD log to error instead of stderr
    fprintf(stderr, __FILE__ ": OpenGL Program Linker Error:\n%.*s\n\n", loglen, logbuffer);
    return {};
  } else {
    glValidateProgram(shader_prog);
    glGetProgramiv(shader_prog, GL_VALIDATE_STATUS, &status);
    if (!status) {
      int loglen;
      char logbuffer[1000];
      glGetProgramInfoLog(shader_prog, sizeof(logbuffer), &loglen, logbuffer);
      // FIXME: Use OpenCAD log to error instead of stderr
      fprintf(stderr, __FILE__ ": OpenGL Program Validation results:\n%.*s\n\n", loglen, logbuffer);
      return {};
    }
  }

  return {
    .shader_program = shader_prog,
    .vertex_shader = vertex_shader,
    .fragment_shader = fragment_shader,
  };
}

}  // namespace ShaderUtils
