#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<geGL/geGL.h>
#include<framework/methodRegister.hpp>
#include<PGR/01/compileShaders.hpp>
#include<PGP/lect/compute.hpp>

using namespace compileShaders;
using namespace ge::gl;

namespace compute{

GLuint prg;
GLuint data;

void onInit(vars::Vars&){

  auto src = R".(
  #version 460
  #line 20

  layout(local_size_x=256)in;//1D WG
  
  layout(binding=0,std430)buffer Data{uint data[];};

  shared uint sharedData[256];

  void main(){
    sharedData[gl_LocalInvocationID.x] = data[gl_GlobalInvocationID.x];

    barrier();

    uint maxVal = 0;
    //if(gl_LocalInvocationID.x == 0){
    //  for(uint i=0;i<256;++i)
    //    maxVal = max(maxVal,shaderData[i]);
    //  data[gl_GlobalInvocationID.x] = maxVal;
    //}


    if(gl_LocalInvocationID.x<128)
      sharedData[gl_LocalInvocationID.x*2+0] = 
        max(
          sharedData[gl_LocalInvocationID.x*2+0],
          sharedData[gl_LocalInvocationID.x*2+1]
          );

    barrier();

    if(gl_LocalInvocationID.x<64)
      sharedData[gl_LocalInvocationID.x*4+0] = 
        max(
          sharedData[gl_LocalInvocationID.x*4+0],
          sharedData[gl_LocalInvocationID.x*4+2]
          );

    barrier();

    if(gl_LocalInvocationID.x<32)
      sharedData[gl_LocalInvocationID.x*8+0] = 
        max(
          sharedData[gl_LocalInvocationID.x*8+0],
          sharedData[gl_LocalInvocationID.x*8+4]
          );

    barrier();

    if(gl_LocalInvocationID.x<16)
      sharedData[gl_LocalInvocationID.x*16+0] = 
        max(
          sharedData[gl_LocalInvocationID.x*16+0],
          sharedData[gl_LocalInvocationID.x*16+8]
          );

    barrier();

    if(gl_LocalInvocationID.x<8)
      sharedData[gl_LocalInvocationID.x*32+0] = 
        max(
          sharedData[gl_LocalInvocationID.x*32+0],
          sharedData[gl_LocalInvocationID.x*32+16]
          );

    barrier();

    if(gl_LocalInvocationID.x<4)
      sharedData[gl_LocalInvocationID.x*64+0] = 
        max(
          sharedData[gl_LocalInvocationID.x*64+0],
          sharedData[gl_LocalInvocationID.x*64+32]
          );

    barrier();

    if(gl_LocalInvocationID.x<2)
      sharedData[gl_LocalInvocationID.x*128+0] = 
        max(
          sharedData[gl_LocalInvocationID.x*128+0],
          sharedData[gl_LocalInvocationID.x*128+64]
          );

    barrier();

    if(gl_LocalInvocationID.x<1)
      sharedData[gl_LocalInvocationID.x*256+0] = 
        max(
          sharedData[gl_LocalInvocationID.x*256+0],
          sharedData[gl_LocalInvocationID.x*256+128]
          );

  }
  ).";


  prg = createProgram({createShader(GL_COMPUTE_SHADER,src)});

  glCreateBuffers(1,&data);

  auto random = [](uint32_t x){
    return uint32_t(x*(x*(x*(x*(x+1)+1)+1)+1)+1);
  }; 

  std::vector<uint32_t>initData(1024);
  for(uint32_t i=0;i<1024;++i)
    initData[i] = random(i+100223)%32+ (i%5==0?1000:0);

  glNamedBufferStorage(data,1024*sizeof(uint32_t),initData.data(),GL_DYNAMIC_STORAGE_BIT);

  glClearColor(0.3,0.3,0.3,1);
}


void onDraw(vars::Vars&){
  glUseProgram(prg);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0,data);
  glDispatchCompute(4,1,1);



  glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT);

  std::vector<uint32_t>d(1024);
  glGetNamedBufferSubData(data,0,d.size()*sizeof(uint32_t),d.data());

  for(uint32_t i=0;i<256;++i)
    std::cerr << d[i] << std::endl;




  exit(0);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

void onResize(vars::Vars&vars){
  auto width  = vars.getUint32("event.resizeX");
  auto height = vars.getUint32("event.resizeY");

  glViewport(0,0,width,height);
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onInit   = onInit;
  clbs.onDraw   = onDraw;
  clbs.onResize = onResize;
  MethodRegister::get().manager.registerMethod("pgp.compute",clbs);
};

}

