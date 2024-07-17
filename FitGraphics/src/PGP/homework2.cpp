///Homework 2. - Geometry Shaders
///
///Replace point rendering with Czech flag rendering.
///Change shaders in order to replace points with Czech flags.
///Do not touch glDrawArrays draw call!
///Do not touch vertex shader!
///
///Vulkan version is not necessary - you are only working with shaders.
///But if you have to...
///
/// resources/images/pgp/homework02before.png
/// resources/images/pgp/homework02after.png

#include<geGL/geGL.h>
#include<glm/glm.hpp>
#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<imguiDormon/imgui.h>
#include<imguiVars/addVarsLimits.h>

#include<framework/methodRegister.hpp>
#include<framework/makeProgram.hpp>
#include<PGR/01/emptyWindow.hpp>

using namespace emptyWindow;
using namespace ge::gl;
using namespace std;

namespace pgp::homework2{

shared_ptr<Program    >program;
shared_ptr<VertexArray>vao    ;

void onInit(vars::Vars&vars){
  auto src = R".(
  #ifdef  VERTEX_SHADER
  void main() {
    if(gl_VertexID == 0)gl_Position = vec4(-.5,-.5,0,1);
    if(gl_VertexID == 1)gl_Position = vec4(+.5,-.5,0,1);
    if(gl_VertexID == 2)gl_Position = vec4(-.5,+.5,0,1);
    if(gl_VertexID == 3)gl_Position = vec4(+.5,+.5,0,1);
  }
  #endif//VERTEX_SHADER
  


  #ifdef  GEOMETRY_SHADER
  ///\todo Homework 2. Reimplement geometry shader.
  /// The geometry shader should replace an input point
  /// with flag of Czech Republic.
  /// every point should be replaced with Czech Republic flag.

  layout(points)in;
  layout(points,max_vertices=1)out;
  
  void main(){
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();
    EndPrimitive();
  }
  #endif//GEOMETRY_SHADER


  #ifdef FRAGMENT_SHADER
  out vec4 fColor;
  
  void main(){
    fColor = vec4(1,1,1,1);
  }
  #endif//FRAGMENT_SHADER
  ).";

  //create shader program
  program = makeProgram(src);
  vao     = make_shared<ge::gl::VertexArray>();
  
  glClearColor(0,0,0,1);
}

void onDraw(vars::Vars&vars){
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  program->use();
  vao->bind();
  glDrawArrays(GL_POINTS,0,4);
  vao->unbind();
}

void onQuit(vars::Vars&vars){
  vao     = nullptr;
  program = nullptr;
  vars.erase("method");
}


EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw        =              onDraw       ;
  clbs.onInit        =              onInit       ;
  clbs.onQuit        =              onQuit       ;
  clbs.onResize      = emptyWindow::onResize     ;
  MethodRegister::get().manager.registerMethod("pgp.homework2",clbs);
};

}

