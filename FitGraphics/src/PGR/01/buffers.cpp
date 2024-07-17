#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<framework/methodRegister.hpp>
#include<PGR/01/emptyWindow.hpp>
#include<PGR/01/buffers.hpp>

using namespace ge::gl;
using namespace emptyWindow;

namespace buffers{
  GLuint vbo;
  void onInit(vars::Vars&);
  void onQuit(vars::Vars&);
}

void buffers::onInit(vars::Vars&){
  std::vector<float>data = {0,1,2,3,4,5,6};

  glCreateBuffers(1,&vbo);

  glNamedBufferData(
      vbo,
      sizeof(decltype(data)::value_type)*data.size(),
      0,
      GL_STATIC_DRAW);
}

void buffers::onQuit(vars::Vars&){
  glDeleteBuffers(1,&vbo);
}

namespace buffers{struct Caller{Caller(){
  methodManager::Callbacks clbs;
  clbs.onInit   = onInit;
  clbs.onQuit   = onQuit;
  clbs.onDraw   = emptyWindow::onDraw;
  clbs.onResize = emptyWindow::onResize;
  MethodRegister::get().manager.registerMethod("pgr01.buffers",clbs);
}}main;}

