///Use atomic instruction inside fragment shader and
///count the number of rasterized fragments.
///Write the number of rasterized fragments of this program
///into nofRasterizedSamples variable inside onDraw function.
///
///You may have to create and clear buffer
///and use atomic instruction for addition.
///
///Help:
///glCreateBuffers
///glClearNamedBufferData
///glBindBufferBase
///glGetNamedBufferSubData
///layout(binding=0,std430)buffer Counter{uint counter;};
///atomicAdd
///
///
/// resources/images/pgp/homework04before.png
/// just compute the counter online...
///

#include<geGL/geGL.h>
#include<glm/glm.hpp>
#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<imguiDormon/imgui.h>
#include<imguiVars/addVarsLimits.h>

#include<framework/methodRegister.hpp>
#include<framework/defineGLSLVersion.hpp>
#include<PGR/01/emptyWindow.hpp>
#include<framework/makeProgram.hpp>
#include<framework/Timer.hpp>

using namespace emptyWindow;
using namespace ge::gl;
using namespace std;

namespace pgp::homework4{

shared_ptr<Program    >program;
shared_ptr<VertexArray>vao    ;
Timer<float>timer;

void onInit(vars::Vars&vars){
  auto programSrc = R".(

#ifdef VERTEX_SHADER
precision highp float;

uniform float iTime;
uniform vec2  iResolution = vec2(512,512);

flat out int vM;
out vec2 vCoord;
out vec3 vNormal;
out vec3 vPosition;

int drawQuad(int offset,mat4 vp,mat4 mm,int m){
  vec2 verts[] = vec2[](
    vec2(0,0),
    vec2(1,0),
    vec2(0,1),
    vec2(0,1),
    vec2(1,0),
    vec2(1,1)
  );

  int vID = gl_VertexID - offset;

  if(vID < 0 || vID>=verts.length())return verts.length();

  gl_Position = vp * mm * vec4(verts[vID]*2-1,0.f,1.f);
  vM = m;
  vCoord = verts[vID];
  vPosition =  vec3(mm * vec4(verts[vID]*2-1,0.f,1.f));  
  vNormal = vec3(inverse(transpose(mm))*vec4(0,0,1,0));
  return verts.length();
}

int drawCube(int offset,mat4 vp,mat4 mm,int m){
  uint indices[] = uint[](
    0u,1u,2u,2u,1u,3u,
    4u,5u,6u,6u,5u,7u,
    0u,4u,2u,2u,4u,6u,
    1u,5u,3u,3u,5u,7u,
    0u,1u,4u,4u,1u,5u,
    2u,3u,6u,6u,3u,7u
  );
 
  int vID = gl_VertexID - offset;
 
  if(vID < 0 || vID>=indices.length())return indices.length();

  vec3 pos;
  for(uint i=0u;i<3u;++i)
    pos[i] = float((indices[vID]>>i)&1u);

  vec3 nor[] = vec3[](
    vec3(0,0,-1),
    vec3(0,0,+1),
    vec3(-1,0,0),
    vec3(+1,0,0),
    vec3(0,-1,0),
    vec3(0,+1,0)
  );

  gl_Position = vp * mm *vec4(pos*2.0f-1.f,1.f);
  vM = m;
  vPosition =  vec3( mm *vec4(pos*2.0f-1.f,1.f));
  vNormal = nor[vID/6];
  
  
  return indices.length();
}

int drawSphere(in int offset,in mat4 vp,in mat4 mm,in int nCols,in int nRows,int m){
  int nCells = nCols * nRows;
  int verticesPerCell = 6;
  int nVertices = nCells * verticesPerCell;
  int verticesPerRow = nCols * verticesPerCell;

  int vID = gl_VertexID-offset;
  if(vID < 0 || vID >= nVertices)return nVertices;

  int cellID = vID / verticesPerCell;
  int verID  = vID % verticesPerCell;

  int rowID = cellID / nCols;
  int colID = cellID % nCols;

  ivec2 verOffset[] = ivec2[](
    ivec2(0,0),
    ivec2(1,0),
    ivec2(0,1),
    ivec2(0,1),
    ivec2(1,0),
    ivec2(1,1)
  );

  float zAngleNorm = float(colID + verOffset[verID].x * 1.f) / float(nCols);
  float yAngleNorm = float(rowID + verOffset[verID].y * 1.f) / float(nRows);


  float yAngle = radians(yAngleNorm * 180.f - 90.f);
  float zAngle = radians(zAngleNorm * 360.f);

  vec2 xyCircle = vec2(         cos(zAngle),sin(zAngle));
  vec3 sphere   = vec3(xyCircle*cos(yAngle),sin(yAngle));

  vCoord = vec2(zAngleNorm,yAngleNorm);
  vNormal   = sphere;
  gl_Position = vp * mm * vec4(sphere,1.f);
  vM = m;

  return nVertices;
}

mat4 T(float x,float y,float z){
  mat4 r = mat4(1);
  r[3] = vec4(x,y,z,1);
  return r;
}


mat4 Ry(float a){
  mat4 r = mat4(1);
  float ca = cos(a);
  float sa = sin(a);
  r[0][0] = ca;
  r[0][2] = sa;
  r[2][0] = -sa;
  r[2][2] = ca;
  
  return r;
}

mat4 Rx(float a){
  mat4 r = mat4(1);
  float ca = cos(a);
  float sa = sin(a);
  r[1][1] = ca;
  r[1][2] = sa;
  r[2][1] = -sa;
  r[2][2] = ca;
  
  return r;
}

mat4 Rz(float a){
  mat4 r = mat4(1);
  float ca = cos(a);
  float sa = sin(a);
  r[0][0] = ca;
  r[0][1] = sa;
  r[1][0] = -sa;
  r[1][1] = ca;
  
  return r;
}


mat4 S(float x,float y,float z){
  mat4 r = mat4(1);
  r[0][0] = x;
  r[1][1] = y;
  r[2][2] = z;
  return r;
}

mat4 S(vec3 s){
  mat4 r = mat4(1);
  r[0][0] = s.x;
  r[1][1] = s.y;
  r[2][2] = s.z;
  return r;
}

mat4 S(float x){
  mat4 r = mat4(1);
  r[0][0] = x;
  r[1][1] = x;
  r[2][2] = x;
  return r;
}

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

mat4 proj = perspective(radians(90.),iResolution.x/iResolution.y,0.1f,1000.f);
mat4 view = T(0,0,-10);
mat4 model = Ry(iTime);
  

int parrotHead(int offset,mat4 vp,mat4 mm,float o){
  float sp = 1.1;
    

  offset += drawCube(offset,vp,mm*S(.7,1,.4),0);
  offset += drawCube(offset,vp,mm*T(-.7/1.4,1,0)*S(1.2,.3,.4),0);
  offset += drawCube(offset,vp,mm*T(-.7,.3,0)*S(.7,.5,.3),1);
  offset += drawCube(offset,vp,mm*T(-.99,.3,0)*S(.7,.5,.2),1);
  offset += drawCube(offset,vp,mm*T(-.3,.5,+.5)*S(.2),1);
  offset += drawCube(offset,vp,mm*T(-.3,.5,-.5)*S(.2),1);
  offset += drawCube(offset,vp,mm*T(-.5,0,.3)*S(.21),2);
  offset += drawCube(offset,vp,mm*T(-.5,0,-.3)*S(.21),2);
  offset += drawCube(offset,vp,mm*T(-1.58,-.2,0)*S(.1,.2,.2),1);
  
  offset += drawCube(offset,vp,mm*T(-0.38,1.5,0)*Rz(2.7)*S(.2),0);
  offset += drawCube(offset,vp,mm*T(+0.2,2.6,0)*Rz(2.7)*S(1,4,1)*S(.2),0);
  offset += drawCube(offset,vp,mm*T(+0.38,1.5,0)*Rz(2.7)*S(.2),0);
  offset += drawCube(offset,vp,mm*T(+1.3,2.2,0)*Rz(2.2)*S(1,4,1)*S(.2),0);
  return offset;
}

int parrotBody(int offset,mat4 vp,mat4 mm,float o){
  float sp = 1.1;
    

  offset += drawCube(offset,vp,mm*T(1.5,-2.5,0)*Rz(radians(35.))*S(1.1,2,1),0);
  offset += drawCube(offset,vp,mm*T(1.7,-2.3,1)*Rz(radians(45.))*S(1.,2,.3),3);
  offset += drawCube(offset,vp,mm*T(1.7,-2.3,-1)*Rz(radians(45.))*S(1.,2,.3),3);
  offset += drawCube(offset,vp,mm*T(3.5,-2.9,0)*Rz(radians(75.))*S(0.5,2,.5),0);
  offset += drawCube(offset,vp,mm*T(1.5,-3.3,.35)*S(.2,2,.2),4);
  offset += drawCube(offset,vp,mm*T(1.5,-3.3,-.35)*S(.2,2,.2),4);
  offset += drawCube(offset,vp,mm*T(1.5,-5.1,-.35)*S(.6,.2,.2),4);
  offset += drawCube(offset,vp,mm*T(1.5,-5.1,+.35)*S(.6,.2,.2),4);
  return offset;
}



void main(){
  gl_Position = vec4(0.f,0.f,0.f,1.f);

  mat4 vp = proj * view * model;

  //int qID = gl_VertexID/6;

  int offset = 0;

  //for(int i=-5;i<5;++i)
  int i=0;
  offset += parrotHead(offset,vp,Ry(sin(iTime)),i);
  offset += parrotBody(offset,vp,mat4(1),i);

}
#endif



#ifdef FRAGMENT_SHADER
precision highp float;

uniform float iTime;

flat in int vM;
in vec2 vCoord;
in vec3 vNormal;
in vec3 vPosition;
out vec4 fColor;


#line 344


void main(){
  vec4 color = vec4(0,0,0,1);
  if(vM==0)color = vec4(0,0,.8,1);
  if(vM==1)color = vec4(0,0,0,1);
  if(vM==2)color = vec4(1,1,0,1);
  if(vM==3)color = vec4(0,0,.7,1);
  if(vM==4)color = vec4(.7,.5,.5,1);
  fColor = vec4(vNormal*.5+.5,1);
  
  vec3 ll = vec3(100);
  vec3 L = normalize(ll-vPosition);
  vec3 N = normalize(vNormal);
  float dF = clamp(dot(L,N),0,1)+0.5;
  fColor = vec4(color.xyz*dF,color.w);
}
#endif




  ).";
  
  
  //create shader program
  program = makeProgram(programSrc);
  program->setNonexistingUniformWarning(false);
  vao     = make_shared<VertexArray>();

  glClearColor(0,0.3,0,1);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_DEPTH_TEST);

  timer.reset();
}

void onDraw(vars::Vars&vars){
  uint32_t nofRasterizedSamples = 0;
  auto w = vars.getUint32("event.resizeX");
  auto h = vars.getUint32("event.resizeY");

  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  program->use();
  program->set1f("iTime",timer.elapsedFromStart());
  program->set2f("iResolution",w,h);
  vao->bind();

  glDrawArrays(GL_TRIANGLES,0,3000);

  vao->unbind();

  vars.addOrGetUint32("method.rasterizedSamples") = nofRasterizedSamples;
}

void onQuit(vars::Vars&vars){
  program = nullptr;
  vao     = nullptr;
  vars.erase("method");
}


EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw        =              onDraw       ;
  clbs.onInit        =              onInit       ;
  clbs.onQuit        =              onQuit       ;
  clbs.onResize      = emptyWindow::onResize     ;
  MethodRegister::get().manager.registerMethod("pgp.homework4",clbs);
};

}

