#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<geGL/geGL.h>
#include<framework/methodRegister.hpp>
#include<PGR/01/emptyWindow.hpp>


using namespace ge::gl;

namespace uv{

std::string const source = R".(

#ifdef VERTEX_SHADER
out vec2 vCoord;
void main(){
  vCoord = vec2(gl_VertexID&1,gl_VertexID>>1);
  gl_Position = vec4(vCoord*2.f-1.f,0.f,1.f);
}
#endif

#ifdef FRAGMENT_SHADER
in vec2 vCoord;
layout(location=0)out vec4 fColor;
void main(){
  fColor = vec4(vCoord,0,1);
}
#endif
).";

void onInit(vars::Vars&vars){

  vars.reCreate<ge::gl::Program>("method.prg",
      std::make_shared<ge::gl::Shader>(GL_VERTEX_SHADER    ,"#version 460\n#define   VERTEX_SHADER\n"  +source),
      std::make_shared<ge::gl::Shader>(GL_FRAGMENT_SHADER  ,"#version 460\n#define FRAGMENT_SHADER\n"  +source)
      );

  vars.reCreate<ge::gl::VertexArray>("method.vao");

  glClearColor(0.4,0.0,0.3,1);
  glEnable(GL_DEPTH_TEST);
}

void onDraw(vars::Vars&vars){
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  auto prg = vars.get<ge::gl::Program    >("method.prg");
  auto vao = vars.get<ge::gl::VertexArray>("method.vao");

  prg->use();
  vao->bind();

  glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

void onQuit(vars::Vars&vars){
  vars.erase("method");
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw        =              onDraw       ;
  clbs.onInit        =              onInit       ;
  clbs.onQuit        =              onQuit       ;
  clbs.onResize      = emptyWindow::onResize     ;
  MethodRegister::get().manager.registerMethod("pgr02.uv",clbs);
};

}
