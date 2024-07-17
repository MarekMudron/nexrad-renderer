///\todo Homework 05.
/// Reimplement compute shader.
/// Each thread should compute mask value and store it into the buffer.
/// Look at reference images.
/// Hint: It is fractal image that is composed of this pattern:
/// ▯▯     
/// ▮▯
///
/// resources/images/pgp/homework05before.png
/// resources/images/pgp/homework05After.png
///

#include<geGL/geGL.h>
#include<glm/gtc/type_ptr.hpp>
#include<glm/glm.hpp>
#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<imguiDormon/imgui.h>
#include<imguiVars/addVarsLimits.h>

#include<framework/methodRegister.hpp>
#include<framework/defineGLSLVersion.hpp>
#include<framework/makeProgram.hpp>
#include<PGR/01/emptyWindow.hpp>

using namespace emptyWindow;
using namespace ge::gl;
using namespace std;

namespace pgp::homework5{

shared_ptr<Program    >computeProgram;
shared_ptr<Program    >drawProgram   ;
shared_ptr<VertexArray>vao           ;
shared_ptr<Buffer     >maskBuffer    ;
auto                   maskSize       = glm::ivec2(128,128);


void onInit(vars::Vars&vars){
  auto drawSrc = R".(
  #ifdef  VERTEX_SHADER
  void main() {
    gl_Position = vec4(-1+2*(gl_VertexID%2),-1+2*(gl_VertexID/2),0,1);
  }
  #endif//VERTEX_SHADER
 


  #ifdef FRAGMENT_SHADER
  layout(binding=0,std430)buffer Mask{uint mask[];};
  
  uniform ivec2 windowSize = ivec2(512,512);
  uniform ivec2 maskSize   = ivec2(128,128);
  

  out vec4 fColor;
  
  void main(){
    ivec2 pixelCoord = ivec2(gl_FragCoord.xy);
    ivec2 scale = windowSize / maskSize;
    ivec2 maskCoord = pixelCoord / scale;
  
    if(maskCoord.x >= maskSize.y || maskCoord.y >= maskSize.y){
      fColor = vec4(0);
      return;
    }
  
    fColor = vec4(float(mask[maskCoord.x + maskCoord.y * maskSize.x]));
  }
  #endif//FRAGMENT_SHADER

  ).";
  
  auto computeSrc =
  R".(
  #ifdef  COMPUTE_SHADER
  layout(local_size_x=16,local_size_y=16)in;
  
  layout(binding=0,std430)buffer Mask{uint mask[];};
  
  uniform ivec2 maskSize = ivec2(128,128);
  
  void main(){
    uvec2 coord = uvec2(gl_GlobalInvocationID.xy);
  
    mask[coord.x + coord.y * maskSize.x] = uint(coord.x == coord.y);
  }
  #endif//COMPUTE_SHADER
  ).";

  //create shader program
  drawProgram = makeProgram(drawSrc);
  vao         = make_shared<VertexArray>();

  computeProgram = makeProgram(computeSrc);

  maskBuffer = make_shared<Buffer>(maskSize.x*maskSize.y*sizeof(int32_t));

  glClearColor(0,0,0,1);
}

void onDraw(vars::Vars&vars){
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  maskBuffer->clear(GL_R32UI,GL_RED_INTEGER,GL_UNSIGNED_INT);
  computeProgram->use();
  computeProgram->set2iv("maskSize",glm::value_ptr(maskSize));
  GLint wgs[3];
  computeProgram->getComputeWorkGroupSize(wgs);
  computeProgram->bindBuffer("Mask",maskBuffer);
  glDispatchCompute(maskSize.x/wgs[0],maskSize.y/wgs[1],1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  auto w = vars.getUint32("event.resizeX");
  auto h = vars.getUint32("event.resizeY");
  auto windowSize = glm::ivec2(w,h);

  drawProgram->use();
  drawProgram->set2iv("windowSize",glm::value_ptr(windowSize));
  drawProgram->set2iv("maskSize",glm::value_ptr(maskSize));
  drawProgram->bindBuffer("Mask",maskBuffer);
  vao->bind();

  glDrawArrays(GL_TRIANGLE_STRIP,0,4);
  vao->unbind();
}

void onQuit(vars::Vars&vars){
  computeProgram = nullptr;
  drawProgram    = nullptr;
  maskBuffer     = nullptr;
  vao            = nullptr;
  vars.erase("method");
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw        =              onDraw       ;
  clbs.onInit        =              onInit       ;
  clbs.onQuit        =              onQuit       ;
  clbs.onResize      = emptyWindow::onResize     ;
  MethodRegister::get().manager.registerMethod("pgp.homework5",clbs);
};

}

