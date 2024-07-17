///\todo Homework 3.
/// Transfrom the cube to sphere using tessellation.
///
/// resources/images/pgp/homework03before.png
/// resources/images/pgp/homework03after.png

#include<geGL/geGL.h>
#include<glm/glm.hpp>
#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<imguiDormon/imgui.h>
#include<imguiVars/addVarsLimits.h>

#include<framework/methodRegister.hpp>
#include<framework/defineGLSLVersion.hpp>
#include<framework/makeProgram.hpp>
#include<framework/Timer.hpp>
#include<PGR/01/emptyWindow.hpp>

using namespace emptyWindow;
using namespace ge::gl;
using namespace std;

namespace pgp::homework3{

shared_ptr<Program    >program;
shared_ptr<VertexArray>vao    ;
shared_ptr<Buffer     >vbo    ;
Timer<float>timer;

void onInit(vars::Vars&vars){
  auto programSrc = R".(
  uniform float iTime       = 0.f          ;
  uniform vec2  iResolution = vec2(512,512);

  mat4 perspective(float fy,float a,float n,float f){
    mat4 R = mat4(0);
    float tfy = tan(fy/2.f);
    R[0][0] = 1.f/tfy;
    R[1][1] = R[0][0]*a;
    R[2][2] = -(f+n)/(f-n);
    R[3][2] = -2.f*f*n/(f-n);
    R[2][3] = -1.f;
    return R;
  }

  #define ROT(i,q,w)           \
  mat4 R##i(float a){          \
    mat4 R = mat4(1.f);        \
    R[q][q] = R[w][w] = cos(a);\
    R[q][w] = sin(a);          \
    R[w][q] = -R[q][w];        \
    return R;                  \
  }

  ROT(x,1,2);
  ROT(y,0,2);
  ROT(z,0,1);

  mat4 T(vec3 t){
    mat4 T = mat4(1.f);
    T[3] = vec4(t,1.f);
    return T;
  }
  mat4 T(float x,float y,float z){return T(vec3(x,y,z));}
  mat4 T(  int x,  int y,  int z){return T(vec3(x,y,z));}
  mat4 T( uint x, uint y, uint z){return T(vec3(x,y,z));}

  mat4 lookAt(vec3 p,vec3 f,vec3 up){
    mat4 R = mat4(1);
    vec3 z = normalize(p-f);
    vec3 x = normalize(cross(up,z));
    vec3 y = normalize(cross(z ,x));
    R[0] = vec4(x,1);
    R[1] = vec4(y,1);
    R[2] = vec4(z,1);
    return transpose(R)*T(-p);
  }



  #ifdef  VERTEX_SHADER
  layout(location=0)in vec2 position;

  mat4 proj = perspective(radians(90.),iResolution.x/iResolution.y,0.1f,1000.f);
  mat4 view = T(0,0,-3);
  mat4 model = Ry(iTime);
  
  
  void main() {

    uint indices[] = uint[](
      0u,1u,2u,2u,1u,3u,
      4u,5u,6u,6u,5u,7u,
      0u,4u,2u,2u,4u,6u,
      1u,5u,3u,3u,5u,7u,
      0u,1u,4u,4u,1u,5u,
      2u,3u,6u,6u,3u,7u
    );

    if(gl_VertexID>=indices.length())return;

    vec3 pos;
    for(uint i=0u;i<3u;++i)
      pos[i] = float((indices[gl_VertexID]>>i)&1u);


    gl_Position = proj*view*model*vec4(2.f*pos-1.f,1);
  }
  #endif//VERTEX_SHADER



  #ifdef  CONTROL_SHADER
  layout(vertices=3)out;
  
  void main(){
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    gl_TessLevelOuter[0] = 1;
    gl_TessLevelOuter[1] = 1;
    gl_TessLevelOuter[2] = 1;
    gl_TessLevelOuter[3] = 1;
    gl_TessLevelInner[0] = 1;
    gl_TessLevelInner[1] = 1;
  }
  #endif//CONTROL_SHADER



  #ifdef  EVALUATION_SHADER
  layout(triangles)in;
  
  void main(){
    gl_Position = 
      gl_in[0].gl_Position*gl_TessCoord.x+
      gl_in[1].gl_Position*gl_TessCoord.y+
      gl_in[2].gl_Position*gl_TessCoord.z;
  }
  #endif//EVALUATION_SHADER



  #ifdef  FRAGMENT_SHADER
  out vec4 fColor;
  
  void main(){
    fColor = vec4(1,0,0,1);
  }
  #endif//FRAGMENT_SHADER
  ).";
  
  
  //create shader program
  program = makeProgram(programSrc);
  program->setNonexistingUniformWarning(false);
  vao     = make_shared<VertexArray>();

  glClearColor(0,0,0,1);
  glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  timer.reset();
}

void onDraw(vars::Vars&vars){
  auto w = vars.getUint32("event.resizeX");
  auto h = vars.getUint32("event.resizeY");

  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  program->use();
  program->set1f("iTime",timer.elapsedFromStart());
  program->set2f("iResolution",w,h);
  vao->bind();

  glPatchParameteri(GL_PATCH_VERTICES,3);
  glDrawArrays(GL_PATCHES,0,6*2*3);

  vao->unbind();
}

void onQuit(vars::Vars&vars){
  vbo     = nullptr;
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
  MethodRegister::get().manager.registerMethod("pgp.homework3",clbs);
};

}

