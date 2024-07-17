#include<geGL/StaticCalls.h>
#include<geGL/geGL.h>
#include<framework/methodRegister.hpp>
#include<PGR/01/emptyWindow.hpp>
#include<PGR/01/compileShaders.hpp>

using namespace ge::gl;
using namespace compileShaders;

namespace geometryShaders{

GLuint vao = 0;

std::shared_ptr<ge::gl::Program>prg;

void onInit(vars::Vars&vars){
  auto vsSrc = R".(
  #version 460

  uniform uint nofVertices = 1;
  void main(){

    float angle = radians(float(gl_VertexID) / float(nofVertices) * 360.f);
    gl_Position = vec4(cos(angle)*.3,sin(angle)*.3,0,1);
  }
  ).";

  auto gsSrc = R".(
  #version 460

  layout(points)in;
  layout(triangle_strip,max_vertices=4)out;

  uniform float size = 0.1f;

  void main(){
    gl_Position = gl_in[0].gl_Position + vec4(-size,-size,0,0);
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + vec4(+size,-size,0,0);
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + vec4(-size,+size,0,0);
    EmitVertex();
    gl_Position = gl_in[0].gl_Position + vec4(+size,+size,0,0);
    EmitVertex();
  }
  ).";
  
  auto fsSrc = R".(
  #version 460
  layout(location=0)out vec4 fColor;
  void main(){
    fColor = vec4(1);
  }
  ).";

  glCreateVertexArrays(1,&vao);

  prg = std::make_shared<ge::gl::Program>(
      std::make_shared<ge::gl::Shader>(GL_VERTEX_SHADER  ,vsSrc),
      std::make_shared<ge::gl::Shader>(GL_GEOMETRY_SHADER,gsSrc),
      std::make_shared<ge::gl::Shader>(GL_FRAGMENT_SHADER,fsSrc)
      );

  glClearColor(0.3,0.0,0.3,1);
}

void onDraw(vars::Vars&vars){
  auto nofVertices = vars.addOrGetUint32("method.nofVertices",10);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glBindVertexArray(vao);
  prg->use();
  prg->set1ui("nofVertices",nofVertices);
  prg->set1f ("size",vars.addOrGetFloat("method.size",0.1f));

  glDrawArrays(GL_POINTS,0,nofVertices);
}

void onQuit(vars::Vars&vars){
  prg = nullptr;
  glDeleteVertexArrays(1,&vao);
  vao = 0;
  glBindVertexArray(vao);
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw   = onDraw  ;
  clbs.onInit   = onInit  ;
  clbs.onQuit   = onQuit  ;
  clbs.onResize = emptyWindow::onResize;
  MethodRegister::get().manager.registerMethod("pgr01.geometryShaders",clbs);
};

}
