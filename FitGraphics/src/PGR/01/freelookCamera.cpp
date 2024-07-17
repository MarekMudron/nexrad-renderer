#include "glm/geometric.hpp"
#include<glm/glm.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<SDL.h>
#include<Vars/Vars.h>
#include<geGL/geGL.h>
#include<geGL/StaticCalls.h>
#include<imguiDormon/imgui.h>
#include<imguiVars/addVarsLimits.h>
#include<framework/FunctionPrologue.h>
#include<framework/methodRegister.hpp>
#include<framework/makeProgram.hpp>
#include<PGR/01/emptyWindow.hpp>
#include<PGR/01/compileShaders.hpp>

using namespace ge::gl;
using namespace compileShaders;
using namespace std;

namespace freelookcamera{

/**
 * Mathematics of free look camera
 */

glm::mat4 proj         = glm::mat4(1.f); /// final projection matrix
glm::mat4 view         = glm::mat4(1.f); /// final view matrix
glm::mat4 viewRotation = glm::mat4(1.f); /// rotation part of view
glm::vec3 position     = glm::vec3(0.f); /// camera position
float     angleX       = 0.f           ; /// camera angle around x axis
float     angleY       = 0.f           ; /// camera angle around y axis

/**
 * @brief This function computes projection matrix
 *
 * @param vars input/output variables
 */
void computeProjectionMatrix(vars::Vars&vars){
  auto width  = vars.getUint32("event.resizeX");
  auto height = vars.getUint32("event.resizeY");
  auto near   = vars.getFloat ("method.near"  );
  auto far    = vars.getFloat ("method.far"   );

  auto aspect = static_cast<float>(width) / static_cast<float>(height);
  proj = glm::perspective(glm::half_pi<float>(),aspect,near,far);
}

/**
 * @brief This function computes free look view matrix
 *
 * @param vars input/output variables
 */
void computeFreelookMatrix(vars::Vars&vars){
  viewRotation = 
    glm::rotate(glm::mat4(1.f),angleX,glm::vec3(1.f,0.f,0.f))*
    glm::rotate(glm::mat4(1.f),angleY,glm::vec3(0.f,1.f,0.f));

  view = 
    viewRotation*
    glm::translate(glm::mat4(1.f),position);
}

/**
 * Controls of the free look camera
 */

std::map<int,bool>keys; /// which keys are down

void onKeyDown(vars::Vars&vars){
  auto key = vars.getInt32("event.key");
  keys[key] = true;
}

void onKeyUp(vars::Vars&vars){
  auto key = vars.getInt32("event.key");
  keys[key] = false;
}

void onMouseMotion(vars::Vars&vars){
  auto xrel = vars.getInt32("event.mouse.xrel");
  auto yrel = vars.getInt32("event.mouse.yrel");

  auto sensitivity = vars.getFloat("method.sensitivity"    );

  if(vars.getBool("event.mouse.middle")){
    angleX += sensitivity * yrel;
    angleY += sensitivity * xrel;

    angleX = glm::clamp(angleX,-glm::half_pi<float>(),glm::half_pi<float>());
  }
}

void onUpdate(vars::Vars&vars){
  auto dt = vars.getFloat("event.dt");

  auto keyVector = glm::vec3(
      keys[SDLK_a     ]-keys[SDLK_d    ],
      keys[SDLK_LSHIFT]-keys[SDLK_SPACE],
      keys[SDLK_w     ]-keys[SDLK_s    ]);

  position += dt * keyVector*glm::mat3(viewRotation);
  computeFreelookMatrix(vars);
}



shared_ptr<Program>prg;
shared_ptr<VertexArray>vao;

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



void onInit(vars::Vars&vars){
  vars.addFloat("method.sensitivity"    ,  0.01f);
  vars.addFloat("method.near"           ,  0.10f);
  vars.addFloat("method.far"            ,100.00f);
  vars.addFloat("method.orbit.angleX"   ,  0.50f);
  vars.addFloat("method.orbit.angleY"   ,  0.50f);
  vars.addFloat("method.orbit.distance" ,  4.00f);
  vars.addFloat("method.orbit.zoomSpeed",  0.10f);

  prg = makeProgram(source);
  vao = make_shared<VertexArray>();

  glClearColor(0.0,0.3,0,1);
  glEnable(GL_DEPTH_TEST);

  computeProjectionMatrix(vars);
}

void onDraw(vars::Vars&vars){
  computeProjectionMatrix(vars);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  vao->bind();
  prg->use();

  prg->setMatrix4fv("view",glm::value_ptr(view));
  prg->setMatrix4fv("proj",glm::value_ptr(proj));

  glDrawArrays(GL_TRIANGLES,0,6*2*3);
}

void onQuit(vars::Vars&vars){
  prg = nullptr;
  vao = vao;
  vars.erase("method");
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw        =              onDraw       ;
  clbs.onInit        =              onInit       ;
  clbs.onQuit        =              onQuit       ;
  clbs.onResize      = emptyWindow::onResize     ;
  clbs.onKeyDown     =              onKeyDown    ;
  clbs.onKeyUp       =              onKeyUp      ;
  clbs.onMouseMotion =              onMouseMotion;
  clbs.onUpdate      =              onUpdate     ;
  MethodRegister::get().manager.registerMethod("pgr01.freelookCamera",clbs);
};

}
