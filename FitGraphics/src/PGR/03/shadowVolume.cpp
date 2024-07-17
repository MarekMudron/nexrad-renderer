#include<BasicCamera/Camera.h>
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
#include<framework/makeProgram.hpp>



using namespace ge::gl;

namespace shadowVolume{

GLuint shadowMap;
GLuint fbo;

std::string const stencilSrc = R".(
#line 23
#ifdef VERTEX_SHADER
uniform mat4 model = mat4(1.f);

out vec3 vColor;
out vec3 vNormal;
out vec3 vPosition;

void main(){
  vec3 positions[] = vec3[](
    vec3(-5,  0,-5),
    vec3(+5,  0,-5),
    vec3(-5,  0,+5),
    vec3(-5,  0,+5),
    vec3(+5,  0,-5),
    vec3(+5,  0,+5),
    vec3(  0,  1,  0),
    vec3(  1,  1,  0),
    vec3(  0,  1,  1)
  );

  if(gl_VertexID<6)vColor = vec3(0.5);
  else vColor = vec3(0.5,0.5,0);
  vNormal = vec3(0,1,0);
  vPosition = positions[gl_VertexID];

  gl_Position = model*vec4(positions[gl_VertexID],1);
}


#endif


#ifdef GEOMETRY_SHADER
layout(triangles)in;
layout(triangle_strip,max_vertices=6)out;

uniform mat4 view  = mat4(1.f);
uniform mat4 proj  = mat4(1.f);
uniform vec3 lightPosition = vec3(10,10,10);

void main(){
  

  gl_Position = proj*view*gl_in[0].gl_Position;EmitVertex();
  gl_Position = proj*view*gl_in[1].gl_Position;EmitVertex();
  gl_Position = proj*view*vec4(gl_in[0].gl_Position.xyz-lightPosition,0);EmitVertex();
  gl_Position = proj*view*vec4(gl_in[1].gl_Position.xyz-lightPosition,0);EmitVertex();
}

#endif


#ifdef FRAGMENT_SHADER
out vec4 fColor;
void main(){
  fColor = vec4(1);
}
#endif
).";


//TODO: modify shaders to support shadow mapping
std::string const sceneSrc = R".(
#line 23
#ifdef VERTEX_SHADER
uniform mat4 model = mat4(1.f);
uniform mat4 view  = mat4(1.f);
uniform mat4 proj  = mat4(1.f);

out vec3 vColor;
out vec3 vNormal;
out vec3 vPosition;

void main(){
  vec3 positions[] = vec3[](
    vec3(-5,  0,-5),
    vec3(+5,  0,-5),
    vec3(-5,  0,+5),
    vec3(-5,  0,+5),
    vec3(+5,  0,-5),
    vec3(+5,  0,+5),
    vec3(  0,  1,  0),
    vec3(  1,  1,  0),
    vec3(  0,  1,  1)
  );

  if(gl_VertexID<6)vColor = vec3(0.5);
  else vColor = vec3(0.5,0.5,0);
  vNormal = vec3(0,1,0);
  vPosition = positions[gl_VertexID];

  gl_Position = proj*view*model*vec4(positions[gl_VertexID],1);
}
#endif

#ifdef FRAGMENT_SHADER

in vec3 vPosition;
in vec3 vNormal;
in vec3 vColor;

layout(location=0)out vec4 fColor;

uniform mat4 view  = mat4(1.f);
uniform vec3 lightPosition = vec3(10,10,10);
uniform vec3 lightColor    = vec3(1);

void main(){
  vec3 cameraPosition = vec3(inverse(view)*vec4(0,0,0,1));
  vec3 L = normalize(lightPosition-vPosition);
  vec3 N = normalize(vNormal);
  vec3 V = normalize(cameraPosition-vPosition);
  vec3 R = reflect(-V,N);
  float s = 10;
  float refl = 0.1;

  float dF = max(dot(L,N),0);

  float sF = pow(max(dot(R,L),0),s);

  vec3 diffuse = vColor * lightColor * dF;
  vec3 specular = lightColor * sF * refl;

  vec3 lambert = diffuse + specular;

  fColor = vec4(lambert,1);
}
#endif
).";

std::shared_ptr<Program>scenePrg;
std::shared_ptr<Program>stencilProgram;
std::shared_ptr<VertexArray>sceneVAO;

void initScene(vars::Vars&vars){
  FUNCTION_PROLOGUE("method.ground");

  scenePrg = makeProgram(sceneSrc);
  stencilProgram = makeProgram(stencilSrc);

  scenePrg->setNonexistingUniformWarning(false);

  sceneVAO = std::make_shared<VertexArray>();
}

void drawScene(vars::Vars&vars,glm::mat4 const&model,glm::mat4 const&view,glm::mat4 const&proj,glm::vec3 const&lightPosition){

  sceneVAO->bind();
  //glEnable(GL_STENCIL_TEST);
  //glStencilFunc(GL_ALWAYS,1,0xff);
  //glStencilOp(GL_KEEP,GL_KEEP,GL_INCR);
  //glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
  //glDepthMask(GL_FALSE);

  stencilProgram->use();

  stencilProgram
    ->setMatrix4fv("model",glm::value_ptr(model))
    ->setMatrix4fv("view" ,glm::value_ptr(view ))
    ->setMatrix4fv("proj" ,glm::value_ptr(proj ))
    ->set3fv("lightPosition",glm::value_ptr(lightPosition));
  glDrawArrays(GL_TRIANGLES,0,9);
  sceneVAO->unbind();
  

  scenePrg->use();
  sceneVAO->bind();
  scenePrg
    ->setMatrix4fv("model",glm::value_ptr(model))
    ->setMatrix4fv("view" ,glm::value_ptr(view ))
    ->setMatrix4fv("proj" ,glm::value_ptr(proj ))
    ->set3fv("lightPosition",glm::value_ptr(lightPosition));

  //glEnable(GL_STENCIL_TEST);
  //glStencilFunc(GL_EQUAL,1,0xff);
  //glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);

  //glDepthMask(GL_TRUE);
  //glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
  glDrawArrays(GL_TRIANGLES,0,9);
}

void onInit(vars::Vars&vars){
  initScene(vars);

  vars.addUint32("method.shadowMapResolution",1024);
  vars.add<glm::vec3>("method.lightPosition",glm::vec3(30.f));
  model::setUpCamera(vars);
  vars.get<basicCamera::OrbitCamera>("method.view")->setDistance(5.f);
  vars.get<basicCamera::PerspectiveCamera>("method.proj")->setNear(0.01);
  vars.get<basicCamera::PerspectiveCamera>("method.proj")->setFar(10000);

  glCreateTextures(GL_TEXTURE_2D,1,&shadowMap);
  glTextureStorage2D(shadowMap,1,GL_DEPTH24_STENCIL8,1024,1024);
  glTextureParameteri(shadowMap,GL_TEXTURE_COMPARE_FUNC,GL_LESS);
  glTextureParameteri(shadowMap,GL_TEXTURE_COMPARE_MODE,GL_COMPARE_R_TO_TEXTURE);

  glCreateFramebuffers(1,&fbo);
  glNamedFramebufferTexture(fbo,GL_DEPTH_ATTACHMENT ,shadowMap,0);
  if(glCheckNamedFramebufferStatus(fbo,GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE)
    std::cerr << "framebuffer incomplete" << std::endl;

  glClearColor(0.1,0.4,0.1,1);
  glEnable(GL_DEPTH_TEST);
}


void onDraw(vars::Vars&vars){
  model::computeProjectionMatrix(vars);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  auto model = glm::mat4(1.f);
  auto view  = vars.get<basicCamera::OrbitCamera      >("method.view")->getView();
  auto proj  = vars.get<basicCamera::PerspectiveCamera>("method.proj")->getProjection();
  auto lightPosition = *vars.get<glm::vec3>("method.lightPosition");

  auto lightView = glm::lookAt(lightPosition,glm::vec3(0.f),glm::vec3(0.f,1.f,0.f));
  auto lightProjection = glm::perspective(glm::half_pi<float>(),1.f,0.1f,5000.f);
  auto bias      = glm::mat4(1.f);
  bias[0][0] = bias[1][1] = bias[2][2] = bias[3][0] = bias[3][1] = bias[3][2] = 0.5;
  //To draw shadows you need to render passes: shadow map pass and camera pass.
  //Shadow map pass renders the scene into shadow map
  //Camera pass utilizes shadow map to render the scene with shadows.
  //
  //Matrices in camera pass:
  //gl_Position = proj*view*model*position
  //
  //Matrices in shadow map pass:
  //gl_Position = proj*view*model*position;
  //vShadowPos = bias*lightProj*lightView*model*position;

  drawScene(vars,model,view,proj,lightPosition);
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
  MethodRegister::get().manager.registerMethod("pgr03.shadowVolume",clbs);
};

}
