#pragma once

#include <framework/methodManager.hpp>
#include <geGL/OpenGL.h>

namespace compileShaders{
  void error(std::string const&name,std::string const&msg);
  GLuint createShader(GLuint type,std::string const&src);
  GLuint createProgram(std::vector<GLuint>const&shaders);
  void onInit(vars::Vars&vars);
  void onDraw(vars::Vars&vars);
  void onQuit(vars::Vars&vars);
}
