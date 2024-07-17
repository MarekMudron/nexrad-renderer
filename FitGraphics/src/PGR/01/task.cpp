#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<framework/methodRegister.hpp>
#include<framework/makeProgram.hpp>
#include<PGR/01/vertexArrays.hpp>
#include<glm/glm.hpp>
#include<glm/gtc/type_ptr.hpp>

using namespace ge::gl;

namespace pgr_task01{
void(*a)();

std::shared_ptr<VertexArray>vao;
std::shared_ptr<Buffer>vbo;

void onInit(vars::Vars&){
  glClearColor(0.1f,0.2f,0.2f,1.0f);
}

void onDraw(vars::Vars&vars){
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}


void onQuit(vars::Vars&vars){
  vars.erase("method");
}

void onResize(vars::Vars&vars){
  auto width  = vars.getUint32("event.resizeX");
  auto height = vars.getUint32("event.resizeY");

  glViewport(0,0,width,height);
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onInit   = onInit;
  clbs.onQuit   = onQuit;
  clbs.onDraw   = onDraw;
  clbs.onResize = onResize;
  MethodRegister::get().manager.registerMethod("pgr01.task",clbs);
};

}
