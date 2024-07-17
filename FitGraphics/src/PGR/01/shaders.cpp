#include<functional>
#include<cstring>
#include<SDL.h>
#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<imguiDormon/imgui.h>
#include<imguiVars/addVarsLimits.h>
#include<framework/FunctionPrologue.h>
#include<framework/methodRegister.hpp>
#include<PGR/01/emptyWindow.hpp>
#include<PGR/01/shaders.hpp>
#include<PGR/01/compileShaders.hpp>

using namespace ge::gl;
using namespace compileShaders;

namespace shaders{

GLuint prg = 0;
GLuint vao = 0;

void recreateProgram(vars::Vars&vars);

auto source = R".(#ifdef VERTEX_SHADER
out vec3 vColor;
void main(){
  vColor = vec3(gl_VertexID==0,gl_VertexID==1,gl_VertexID==2);
  gl_Position = vec4(gl_VertexID&1,gl_VertexID>>1,0,1);
}
#endif

#ifdef FRAGMENT_SHADER
in vec3 vColor;
layout(location=0)out vec4 fColor;
void main(){
  fColor = vec4(vColor,1);
}
#endif
).";

void onInit(vars::Vars&vars){
  vars.addString("method.shaders"  ,source);
  hide(vars,"method.shaders");

  recreateProgram(vars);

  glCreateVertexArrays(1,&vao);

  glClearColor(0.0,0.0,0.0,1);
}

void recreateProgram(vars::Vars&vars){
  FUNCTION_PROLOGUE("method","method.shaders");

  glDeleteProgram(prg);

  auto source = vars.getString("method.shaders");

  prg = createProgram({
      createShader(GL_VERTEX_SHADER  ,"#version 460\n#define VERTEX_SHADER\n"  +source),
      createShader(GL_FRAGMENT_SHADER,"#version 460\n#define FRAGMENT_SHADER\n"+source)});
}

void editProgram(vars::Vars&vars){
  auto&vsSrc = vars.getString("method.shaders");
  static char text[1024*100];
  std::strcpy(text,vsSrc.data());

  ImGui::Begin("vars");
  static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
  ImGui::InputTextMultiline("##vert", text, IM_ARRAYSIZE(text), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 30), flags);
  vsSrc = text;

  if(ImGui::Button("compile")){
    vars.updateTicks("method.shaders"  );
  }
  ImGui::End();
}

void onDraw(vars::Vars&vars){
  recreateProgram(vars);

  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glBindVertexArray(vao);
  glUseProgram(prg);
  glDrawArrays(GL_TRIANGLES,0,vars.addOrGetUint32("method.vertices",3));

  editProgram(vars);
}

void onQuit(vars::Vars&vars){
  glDeleteProgram(prg);
  glDeleteVertexArrays(1,&vao);
  prg = 0;
  vao = 0;
  glUseProgram(prg);
  glBindVertexArray(vao);
  vars.erase("method");
}

void onKeyDown(vars::Vars&vars){
  if(vars.getInt32("event.key") == 'c')
    vars.updateTicks("method.shaders");
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw    =              onDraw   ;
  clbs.onInit    =              onInit   ;
  clbs.onQuit    =              onQuit   ;
  clbs.onResize  = emptyWindow::onResize ;
  clbs.onKeyDown =              onKeyDown;
  MethodRegister::get().manager.registerMethod("pgr01.shaders",clbs);
};

}
