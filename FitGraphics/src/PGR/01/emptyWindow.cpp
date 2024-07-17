#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<framework/methodRegister.hpp>
#include<PGR/01/emptyWindow.hpp>

using namespace ge::gl;

namespace emptyWindow{

void onInit(vars::Vars&){
  glClearColor(0.3,0.3,0.3,1);
}

void onDraw(vars::Vars&){
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

void onResize(vars::Vars&vars){
  auto width  = vars.getUint32("event.resizeX");
  auto height = vars.getUint32("event.resizeY");

  glViewport(0,0,width,height);
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw   = onDraw  ;
  clbs.onInit   = onInit  ;
  clbs.onResize = onResize;
  MethodRegister::get().manager.registerMethod("pgr01.emptyWindow",clbs);
};


}

