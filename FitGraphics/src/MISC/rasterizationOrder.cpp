#include<Vars/Vars.h>

#include<geGL/geGL.h>
#include<geGL/StaticCalls.h>
#include<imguiDormon/imgui.h>
#include<imguiVars/addVarsLimits.h>

#include<framework/methodRegister.hpp>
#include<framework/defineGLSLVersion.hpp>
#include<PGR/01/emptyWindow.hpp>

using namespace emptyWindow;
using namespace ge::gl;

namespace rasterizationOrder{

std::shared_ptr<Program    >prg    ;
std::shared_ptr<Buffer     >counter;
std::shared_ptr<VertexArray>vao    ;

void onInit(vars::Vars&vars){
  auto const vsSrc = R".(
  flat out uint vTriangleID;
  void main(){
    if((gl_VertexID%3) == 0)gl_Position = vec4(-1,-1,0,1);
    if((gl_VertexID%3) == 1)gl_Position = vec4(+1,-1,0,1);
    if((gl_VertexID%3) == 2)gl_Position = vec4(+1,+1,0,1);
    vTriangleID = gl_VertexID/3;
  }
  ).";
  auto const fsSrc = R".(
  layout(binding=0,std430)buffer Counter{uint counter[];};
  uniform uint maxFragment = 0;
  uniform uint maxFragmentK = 0;
  uniform uint nofTriangles = 2;
  flat in uint vTriangleID;
  out vec4 fColor;
  void main(){
    uint fragmentId = atomicAdd(counter[0],1);
    if(fragmentId >= maxFragment + maxFragmentK*1000){
      discard;
      return;
    }
    float t = float(vTriangleID)/float(nofTriangles-1);
    fColor = vec4(t,1-t,0,1);
  }
  ).";

  auto vs = std::make_shared<ge::gl::Shader>(GL_VERTEX_SHADER,
      defineGLSLVersion(),vsSrc);
  auto fs = std::make_shared<ge::gl::Shader>(GL_FRAGMENT_SHADER,
      defineGLSLVersion(),fsSrc);

  prg     = std::make_shared<ge::gl::Program>(vs,fs);
  counter = std::make_shared<ge::gl::Buffer>(sizeof(uint32_t));
  vao     = std::make_shared<ge::gl::VertexArray>();

  vars.addUint32("method.nofTriangles",2);
  vars.addUint32("method.maxFragment",1000);
  vars.addUint32("method.maxFragmentK",0);

}

void onDraw(vars::Vars&vars){
  glClearColor(0.1f,0.1f,0.1f,1.f);
  glClear(GL_COLOR_BUFFER_BIT);
  vao->bind();
  counter->clear(GL_R32UI,GL_RED_INTEGER,GL_UNSIGNED_INT);
  glMemoryBarrier(GL_ALL_BARRIER_BITS);
  counter->bindBase(GL_SHADER_STORAGE_BUFFER,0);
  prg
    ->set1ui("maxFragment" ,vars.getUint32("method.maxFragment"))
    ->set1ui("maxFragmentK",vars.getUint32("method.maxFragmentK"))
    ->set1ui("nofTriangles",vars.getUint32("method.nofTriangles"))
    ->use();
  glDrawArrays(GL_TRIANGLES,0,vars.getUint32("method.nofTriangles")*3);

  vao->unbind();
}

void onQuit(vars::Vars&vars){
  vars.erase("method");
  prg     = nullptr;
  vao     = nullptr;
  counter = nullptr;
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw   = onDraw  ;
  clbs.onInit   = onInit  ;
  clbs.onQuit   = onQuit  ;
  clbs.onResize = onResize;
  MethodRegister::get().manager.registerMethod("misc.rasterizationOrder",clbs);
};

}

