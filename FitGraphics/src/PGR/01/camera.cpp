#include<glm/glm.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<SDL.h>
#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<imguiDormon/imgui.h>
#include<imguiVars/addVarsLimits.h>
#include<framework/FunctionPrologue.h>
#include<framework/methodRegister.hpp>
#include<PGR/01/emptyWindow.hpp>
#include<PGR/01/compileShaders.hpp>

using namespace ge::gl;
using namespace compileShaders;

namespace camera{

GLuint prg = 0;
GLuint vao = 0;

glm::mat4 proj = glm::mat4(1.f);
glm::mat4 view = glm::mat4(1.f);

GLuint viewUniform;
GLuint projUniform;

std::string const source = R".(
#ifdef VERTEX_SHADER

uniform mat4 view = mat4(1.f);
uniform mat4 proj = mat4(1.f);

out vec3 vColor;
void main(){
  uint indices[] = uint[](
    0u,1u,2u,2u,1u,3u,
    4u,5u,6u,6u,5u,7u,
    0u,4u,2u,2u,4u,6u,
    1u,5u,3u,3u,5u,7u,
    0u,1u,4u,4u,1u,5u,
    2u,3u,6u,6u,3u,7u
  );
  if(gl_VertexID>=indices.length()){
    gl_Position = vec4(0.f,0.f,0.f,1.f);
    return;
  }

  vec3 pos;
  for(uint i=0u;i<3u;++i)
    pos[i] = float((indices[gl_VertexID]>>i)&1u);

  vColor = pos;

  gl_Position = proj*view*vec4(pos*2.f-1.f,1.f);

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

void computeProjectionMatrix(vars::Vars&vars){
  auto width  = vars.getUint32("event.resizeX");
  auto height = vars.getUint32("event.resizeY");
  auto near   = vars.getFloat ("method.near"  );
  auto far    = vars.getFloat ("method.far"   );

  float aspect = static_cast<float>(width) / static_cast<float>(height);
  proj = glm::perspective(glm::half_pi<float>(),aspect,near,far);
}

void computeViewMatrix(vars::Vars&vars){
  auto angleX   = vars.getFloat("method.orbit.angleX"  );
  auto angleY   = vars.getFloat("method.orbit.angleY"  );
  auto distance = vars.getFloat("method.orbit.distance");
  view = 
    glm::translate(glm::mat4(1.f),       glm::vec3(0.f,0.f,-distance))*
    glm::rotate   (glm::mat4(1.f),angleX,glm::vec3(1.f,0.f,      0.f))*
    glm::rotate   (glm::mat4(1.f),angleY,glm::vec3(0.f,1.f,      0.f));
}

void onInit(vars::Vars&vars){
  vars.addFloat("method.sensitivity"    ,  0.01f);
  vars.addFloat("method.near"           ,  0.10f);
  vars.addFloat("method.far"            ,100.00f);
  vars.addFloat("method.orbit.angleX"   ,  0.50f);
  vars.addFloat("method.orbit.angleY"   ,  0.50f);
  vars.addFloat("method.orbit.distance" ,  4.00f);
  vars.addFloat("method.orbit.zoomSpeed",  0.10f);

  prg = createProgram({
      createShader(GL_VERTEX_SHADER  ,"#version 460\n#define VERTEX_SHADER\n"  +source),
      createShader(GL_FRAGMENT_SHADER,"#version 460\n#define FRAGMENT_SHADER\n"+source),
      });

  viewUniform = glGetUniformLocation(prg,"view");
  projUniform = glGetUniformLocation(prg,"proj");

  glCreateVertexArrays(1,&vao);
  glClearColor(0.0,0.3,0,1);
  glEnable(GL_DEPTH_TEST);

  computeProjectionMatrix(vars);
}

void onDraw(vars::Vars&vars){
  computeProjectionMatrix(vars);
  computeViewMatrix(vars);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glBindVertexArray(vao);
  glUseProgram(prg);

  glProgramUniformMatrix4fv(prg,viewUniform,1,GL_FALSE,glm::value_ptr(view));
  glProgramUniformMatrix4fv(prg,projUniform,1,GL_FALSE,glm::value_ptr(proj));
  glDrawArrays(GL_TRIANGLES,0,6*2*3);
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
  auto sensitivity = vars.getFloat("method.sensitivity" );
  auto&angleX      = vars.getFloat("method.orbit.angleX");
  auto&angleY      = vars.getFloat("method.orbit.angleY");
  auto key = vars.getInt32("event.key");
  if(key == SDLK_a)angleY += sensitivity;
  if(key == SDLK_d)angleY -= sensitivity;
  if(key == SDLK_w)angleX += sensitivity;
  if(key == SDLK_s)angleX -= sensitivity;
}

void onMouseMotion(vars::Vars&vars){
  auto xrel = vars.getInt32("event.mouse.xrel");
  auto yrel = vars.getInt32("event.mouse.yrel");

  auto sensitivity = vars.getFloat("method.sensitivity"    );
  auto zoomSpeed   = vars.getFloat("method.orbit.zoomSpeed");
  auto&angleX      = vars.getFloat("method.orbit.angleX"   );
  auto&angleY      = vars.getFloat("method.orbit.angleY"   );
  auto&distance    = vars.getFloat("method.orbit.distance" );

  if(vars.getBool("event.mouse.middle")){
    angleX += sensitivity * yrel;
    angleY += sensitivity * xrel;

    angleX = glm::clamp(angleX,-glm::half_pi<float>(),glm::half_pi<float>());
  }
  if(vars.getBool("event.mouse.right")){
    distance += zoomSpeed * yrel;

    distance = glm::clamp(distance,0.f,100.f);
  }
}


EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw        =              onDraw       ;
  clbs.onInit        =              onInit       ;
  clbs.onQuit        =              onQuit       ;
  clbs.onResize      = emptyWindow::onResize     ;
  clbs.onKeyDown     =              onKeyDown    ;
  clbs.onMouseMotion =              onMouseMotion;
  MethodRegister::get().manager.registerMethod("pgr01.camera",clbs);
};

}
