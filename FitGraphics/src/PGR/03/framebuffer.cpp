#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<geGL/geGL.h>
#include<framework/methodRegister.hpp>
#include<framework/FunctionPrologue.h>
#include<BasicCamera/OrbitCamera.h>
#include<BasicCamera/PerspectiveCamera.h>
#include<PGR/01/emptyWindow.hpp>
#include<PGR/02/model.hpp>
#include<PGR/03/phong.hpp>
#include<framework/bunny.hpp>



using namespace ge::gl;

namespace framebuffer{

GLuint colorTex;
GLuint depthTex;
GLuint fbo;

std::string const source = R".(
#line 16
#ifdef VERTEX_SHADER
uniform mat4 view  = mat4(1.f);
uniform mat4 proj  = mat4(1.f);
uniform mat4 model = mat4(1.f);

out vec2 vTexCoord   ;

void main(){
  vTexCoord = vec2(gl_VertexID&1,gl_VertexID>>1);
  gl_Position  = proj*view*model*vec4(vTexCoord*2.f-1.f,0.f,1.f);
}
#endif

#ifdef FRAGMENT_SHADER

layout(binding=0)uniform sampler2D tex;
layout(location=0)out vec4 fColor;

in vec2 vTexCoord;

void main(){
  fColor = texture(tex,vTexCoord);
}
#endif
).";

void initQuad(vars::Vars&vars){
  FUNCTION_PROLOGUE("method.quad");
  vars.reCreate<Program>("method.quad.prg",
      std::make_shared<Shader>(GL_VERTEX_SHADER  ,"#version 460\n#define   VERTEX_SHADER\n"  +source),
      std::make_shared<Shader>(GL_FRAGMENT_SHADER,"#version 460\n#define FRAGMENT_SHADER\n"  +source)
      );
  auto vao = vars.reCreate<VertexArray>("method.quad.vao");
}

void drawQuad(vars::Vars&vars){
  initQuad(vars);

  phong::setScene(vars,"method.quad.vao","method.quad.prg",GL_TRIANGLE_STRIP,4,false);
  phong::drawScene(vars);
}


void onInit(vars::Vars&vars){
  model::setUpCamera(vars);

  glCreateTextures   (GL_TEXTURE_2D,1,&colorTex);
  glTextureStorage2D (colorTex,1,GL_RGBA8,1024,1024);
  glTextureParameteri(colorTex,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
  glTextureParameteri(colorTex,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

  glCreateTextures(GL_TEXTURE_2D,1,&depthTex);
  glTextureStorage2D(depthTex,1,GL_DEPTH24_STENCIL8,1024,1024);

  glCreateFramebuffers(1,&fbo);
  glNamedFramebufferTexture(fbo,GL_COLOR_ATTACHMENT0,colorTex,0);
  glNamedFramebufferTexture(fbo,GL_DEPTH_ATTACHMENT ,depthTex,0);
  if(glCheckNamedFramebufferStatus(fbo,GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
    std::cerr << "framebuffer incomplete" << std::endl;

  glClearColor(0.1,0.1,0.1,1);
  glEnable(GL_DEPTH_TEST);
}

void fillFramebuffer(vars::Vars&vars){
  glBindFramebuffer(GL_FRAMEBUFFER,fbo);
  glViewport(0,0,1024,1024);
  glClearColor(0,0,0.5,1);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  *vars.addOrGet<glm::mat4>("method.scene.view") = glm::lookAt(glm::vec3(1.f),glm::vec3(0.f),glm::vec3(0.f,1.f,0.f));
  phong::drawBunny(vars);


  auto width  = vars.getUint32("event.resizeX");
  auto height = vars.getUint32("event.resizeY");
  glViewport(0,0,width,height);
  glBindFramebuffer(GL_FRAMEBUFFER,0);
}

void onDraw(vars::Vars&vars){
  model::computeProjectionMatrix(vars);

  fillFramebuffer(vars);

  glBindTextureUnit(0,colorTex);

  glClearColor(0.2,0.2,0.2,1);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  *vars.addOrGet<glm::mat4>("method.scene.view") = vars.getReinterpret<basicCamera::CameraTransform >("method.view")->getView      ();
  *vars.addOrGet<glm::mat4>("method.scene.proj") = vars.getReinterpret<basicCamera::CameraProjection>("method.proj")->getProjection();

  drawQuad(vars);
}

void onQuit(vars::Vars&vars){
  glDeleteTextures(1,&colorTex);
  glDeleteTextures(1,&depthTex);
  glDeleteFramebuffers(1,&fbo);
  vars.erase("method");
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw        =              onDraw       ;
  clbs.onInit        =              onInit       ;
  clbs.onQuit        =              onQuit       ;
  clbs.onResize      = emptyWindow::onResize     ;
  clbs.onMouseMotion = model      ::onMouseMotion;
  MethodRegister::get().manager.registerMethod("pgr03.framebuffer",clbs);
};

}
