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
#include<framework/makeProgram.hpp>

using namespace ge::gl;
using namespace compileShaders;

namespace bezierSurf{

std::shared_ptr<Program>prg;
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
  
  gl_Position = proj*view*vec4(float(gl_VertexID&3)-1.5f,sin(gl_VertexID),float(gl_VertexID>>2)-1.5f,1);


}
#endif

#ifdef CONTROL_SHADER
layout(vertices=16)out;
void main(){
  if(gl_InvocationID==0){
    gl_TessLevelInner[0] = 64;
    gl_TessLevelInner[1] = 64;
    gl_TessLevelOuter[0] = 64;
    gl_TessLevelOuter[1] = 64;
    gl_TessLevelOuter[2] = 64;
    gl_TessLevelOuter[3] = 64;
  }
  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
#endif

#ifdef EVALUATION_SHADER
layout(quads)in;

vec4 bernstein(float a){
  return vec4((1-a)*(1-a)*(1-a),3*(1-a)*(1-a)*a,3*(1-a)*a*a,a*a*a);
}

void main(){
  vec4 t = bernstein(gl_TessCoord.x);
  vec4 l = bernstein(gl_TessCoord.y);

  vec4 res = vec4(0);
  for(int y=0;y<4;++y)
    for(int x=0;x<4;++x)  
      res += gl_in[y*4+x].gl_Position * t[x] * l[y];

  gl_Position = res;

}
#endif

#ifdef FRAGMENT_SHADER
in vec3 vColor;
layout(location=0)out vec4 fColor;
void main(){
  fColor = vec4(1);
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

  prg = makeProgram(source);

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
  prg->use();

  prg->setMatrix4fv("view",glm::value_ptr(view));
  prg->setMatrix4fv("proj",glm::value_ptr(proj));
  glPointSize(4);
  glPatchParameteri(GL_PATCH_VERTICES,16);
  glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  glDrawArrays(GL_PATCHES,0,16);
}

void onQuit(vars::Vars&vars){
  prg = nullptr;
  glDeleteVertexArrays(1,&vao);
  prg = 0;
  vao = 0;
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
  MethodRegister::get().manager.registerMethod("pgr03.bezierSurf",clbs);
};

}
