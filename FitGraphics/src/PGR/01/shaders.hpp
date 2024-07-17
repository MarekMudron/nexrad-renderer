#pragma once

#include <framework/methodManager.hpp>
#include <geGL/OpenGL.h>

namespace shaders{
  void onInit(vars::Vars&vars);
  void onDraw(vars::Vars&vars);
  void onQuit(vars::Vars&vars);
}
