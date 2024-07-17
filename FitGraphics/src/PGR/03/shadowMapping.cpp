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

namespace shadowMapping{

GLuint shadowMap;
GLuint fbo;

std::string const groundSource = R".(
#line 23
#ifdef VERTEX_SHADER
uniform mat4 view  = mat4(1.f);
uniform mat4 proj  = mat4(1.f);
uniform mat4 model = mat4(1.f);

out vec2 vTexCoord   ;

void main(){
  vTexCoord = vec2(gl_VertexID&1,gl_VertexID>>1);
  vec4 pos  = vec4(vTexCoord.x*10.f-5.f,-1,vTexCoord.x*10.f-5.f,1.f);

  gl_Position  = proj*view*model*pos;
}
#endif

#ifdef FRAGMENT_SHADER

layout(location=0)out vec4 fColor;

void main(){
  fColor = vec4(0,0.5,0,1);
}
#endif
).";

std::string const shadowMappingFS = R".(

uniform vec3  lightPosition = vec3(30,30,30)   ;
uniform vec3  lightColor    = vec3(1,1,1)      ;
uniform vec3  lightAmbient  = vec3(0.3,0.1,0.0);
uniform float shininess     = 60.f             ;

layout(location=0)out vec4 fColor;

layout(binding=0)uniform sampler2DShadow shadowMap;

in vec4 vShadowPos;
in vec3 vPosition;
in vec3 vNormal;
in vec3 vCamPosition;

void main(){
  float litValue = textureProj(shadowMap,vShadowPos).x;

  vec3 diffuseColor = vec3(.4,1,.4);

  vec3 finalColor = phongLighting(
      vPosition          ,
      normalize(vNormal) ,
      lightPosition      ,
      vCamPosition       ,
      lightColor*litValue,
      lightAmbient       ,
      diffuseColor       ,
      shininess          ,
      1.f                );


  fColor = vec4(finalColor,1);
}

).";

std::string const shadowGroundVS = R".(

uniform mat4 view  = mat4(1.f);
uniform mat4 proj  = mat4(1.f);
uniform mat4 model = mat4(1.f);

uniform mat4 lightView = mat4(1.f);
uniform mat4 lightProj = mat4(1.f);
uniform mat4 lightBias = mat4(1.f);

out vec4 vShadowPos;
out vec3 vPosition;
out vec3 vNormal;
out vec3 vCamPosition;

void main(){
  vec4 pos = vec4((gl_VertexID&1)*10.f-5.f,-1,(gl_VertexID>>1)*10.f-5.f,1.f);

  vPosition    = vec3(pos);
  vNormal      = vec3(0,1,0);
  vCamPosition = vec3(inverse(view)*vec4(0,0,0,1));

  gl_Position  =                proj*     view*model*pos;
  vShadowPos   = lightBias*lightProj*lightView*model*pos;
}
).";

std::string const bunnyShadowMappingVS = R".(
uniform mat4 view  = mat4(1.f);
uniform mat4 proj  = mat4(1.f);
uniform mat4 model = mat4(1.f);

uniform mat4 lightView = mat4(1.f);
uniform mat4 lightProj = mat4(1.f);
uniform mat4 lightBias = mat4(1.f);

layout(location = 0)in vec3 position;
layout(location = 1)in vec3 normal  ;

out vec4 vShadowPos;
out vec3 vPosition;
out vec3 vNormal;
out vec3 vCamPosition;

void main(){
  vec4 pos = vec4(position,1.f);

  vPosition    = vec3(pos);
  vNormal      = normal;
  vCamPosition = vec3(inverse(view)*vec4(0,0,0,1));

  gl_Position  =                proj*     view*model*pos;
  vShadowPos   = lightBias*lightProj*lightView*model*pos;

}

).";

void initGround(vars::Vars&vars){
  FUNCTION_PROLOGUE("method.ground");
  vars.reCreate<Program>("method.ground.prg",
      std::make_shared<Shader>(GL_VERTEX_SHADER  ,"#version 460\n#define   VERTEX_SHADER\n"  +groundSource),
      std::make_shared<Shader>(GL_FRAGMENT_SHADER,"#version 460\n#define FRAGMENT_SHADER\n"  +groundSource)
      );

  vars.reCreate<Program>("method.ground.shadow_prg",
      std::make_shared<Shader>(GL_VERTEX_SHADER  ,"#version 460",shadowGroundVS),
      std::make_shared<Shader>(GL_FRAGMENT_SHADER,"#version 460",phong::phongLightingShader+shadowMappingFS)
      );

  auto vao = vars.reCreate<VertexArray>("method.ground.vao");
}

void drawGround(vars::Vars&vars){
  initGround(vars);

  phong::setScene(vars,"method.ground.vao","method.ground.prg",GL_TRIANGLE_STRIP,4,false);
  phong::drawScene(vars);
}

glm::mat4 getLightView      (vars::Vars&vars){
  auto&lightPosition = *vars.get<glm::vec3>("method.lightPosition");
  return glm::lookAt(lightPosition,glm::vec3(0.f),glm::vec3(0.f,1.f,0.f));
}

glm::mat4 getLightProj(vars::Vars&vars){
  auto fovy = vars.addOrGetFloat("method.shadowFovy",0.2f);
  return glm::perspective(fovy,1.f,0.1f,5000.f);
}

void setShadowMappingUniforms(vars::Vars&vars,std::string const&prgName){
  auto prg = vars.get<Program>(prgName);

  auto lightView = getLightView(vars);
  auto lightProj = getLightProj(vars);
  auto bias      = glm::mat4(1.f);
  bias[0][0] = bias[1][1] = bias[2][2] = bias[3][0] = bias[3][1] = bias[3][2] = 0.5;

  prg
    ->setMatrix4fv("lightView",glm::value_ptr(lightView))
    ->setMatrix4fv("lightProj",glm::value_ptr(lightProj))
    ->setMatrix4fv("lightBias",glm::value_ptr(bias     ))
    ->set3fv      ("lightPosition",glm::value_ptr(*vars.get<glm::vec3>("method.lightPosition")));

}

void drawShadowedGround(vars::Vars&vars){
  initGround(vars);
  phong::setScene(vars,"method.ground.vao","method.ground.shadow_prg",GL_TRIANGLE_STRIP,4,false);
  setShadowMappingUniforms(vars,"method.ground.shadow_prg");
  phong::drawScene(vars);
}

void initShadowedBunny(vars::Vars&vars){
  FUNCTION_PROLOGUE("method.bunny");
  vars.reCreate<Program>("method.bunny.shadow_prg",
      std::make_shared<Shader>(GL_VERTEX_SHADER  ,"#version 460\n",bunnyShadowMappingVS),
      std::make_shared<Shader>(GL_FRAGMENT_SHADER,"#version 460\n",phong::phongLightingShader+shadowMappingFS)
      );
}

void drawShadowedBunny(vars::Vars&vars){
  initShadowedBunny(vars);
  phong::setScene(vars,"method.bunny.vao","method.bunny.shadow_prg",GL_TRIANGLES,vars.getUint32("method.bunny.nofIndices"),true);
  setShadowMappingUniforms(vars,"method.bunny.shadow_prg");
  phong::drawScene(vars);
}


void onInit(vars::Vars&vars){
  vars.addUint32("method.shadowMapResolution",1024);
  vars.add<glm::vec3>("method.lightPosition",glm::vec3(30.f));
  model::setUpCamera(vars);
  vars.get<basicCamera::OrbitCamera>("method.view")->setDistance(5.f);

  glCreateTextures(GL_TEXTURE_2D,1,&shadowMap);
  glTextureStorage2D(shadowMap,1,GL_DEPTH24_STENCIL8,1024,1024);
  glTextureParameteri(shadowMap,GL_TEXTURE_COMPARE_FUNC,GL_LESS);
  glTextureParameteri(shadowMap,GL_TEXTURE_COMPARE_MODE,GL_COMPARE_R_TO_TEXTURE);

  glCreateFramebuffers(1,&fbo);
  glNamedFramebufferTexture(fbo,GL_DEPTH_ATTACHMENT ,shadowMap,0);
  if(glCheckNamedFramebufferStatus(fbo,GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
    std::cerr << "framebuffer incomplete" << std::endl;

  glClearColor(0.1,0.1,0.1,1);
  glEnable(GL_DEPTH_TEST);
}

void drawScene(vars::Vars&vars){
  drawGround(vars);
  phong::drawBunny(vars);
}

void renderShadowMap(vars::Vars&vars){
  glBindFramebuffer(GL_FRAMEBUFFER,fbo);
  glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
  glClear(GL_DEPTH_BUFFER_BIT);
  *vars.addOrGet<glm::mat4>("method.scene.view") = getLightView(vars);
  *vars.addOrGet<glm::mat4>("method.scene.proj") = getLightProj(vars);

  auto res = vars.getUint32("method.shadowMapResolution");
  glViewport(0,0,res,res);
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(2.5f,10.f);

  drawScene(vars);

  auto width  = vars.getUint32("event.resizeX");
  auto height = vars.getUint32("event.resizeY");
  glViewport(0,0,width,height);

  glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
  glBindFramebuffer(GL_FRAMEBUFFER,0);
}

void drawShadowedScene(vars::Vars&vars){
  glClearColor(0.2,0.2,0.2,1);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  *vars.addOrGet<glm::mat4>("method.scene.view") = vars.getReinterpret<basicCamera::CameraTransform >("method.view")->getView      ();
  *vars.addOrGet<glm::mat4>("method.scene.proj") = vars.getReinterpret<basicCamera::CameraProjection>("method.proj")->getProjection();
  glBindTextureUnit(0,shadowMap);
  drawShadowedGround(vars);
  drawShadowedBunny(vars);
  //phong::drawBunny(vars);
}

void onDraw(vars::Vars&vars){
  model::computeProjectionMatrix(vars);

  renderShadowMap(vars);

  drawShadowedScene(vars);
}

void onQuit(vars::Vars&vars){
  glDeleteTextures(1,&shadowMap);
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
  MethodRegister::get().manager.registerMethod("pgr03.shadowMapping",clbs);
};

}
