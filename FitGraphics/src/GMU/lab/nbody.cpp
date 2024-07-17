#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<geGL/geGL.h>
#include<imguiVars/imguiVars.h>
#include<imguiVars/addVarsLimits.h>
#include<imguiDormon/imgui.h>
#include<framework/Barrier.h>
#include<framework/FunctionPrologue.h>
#include<framework/methodRegister.hpp>
#include<PGR/01/emptyWindow.hpp>

using namespace ge::gl;

namespace nbody{

template<typename T>
T divRoundUp(T a,T b){
  return (a/b) + static_cast<T>((a%b)!=0);
}

void createResetProgram(vars::Vars&vars){
  FUNCTION_PROLOGUE("method");

  std::string const src = R".(
  #version 460

  layout(local_size_x=256)in;

  layout(binding=0,std430)buffer Particles{float particles[];};

  uniform uint nofParticles = 1;

  uint randU(uint x,uint s){
    uint ra = x*(x*(x*(x*(x+13+s)+111+s*3)+1177+s*7)+9331237+s*13)+1337;
    return ra;
  }

  float randF(uint x,uint s){
    return float(randU(x,s)) / float(0x7fffffff) - 1.f;
  }

  vec2 randVel(uint x){
    return vec2(randF(x,117),randF(x,3337));
  }

  void main(){
    uint gid = gl_GlobalInvocationID.x;
    if(gid >= nofParticles)return;

    uint width = uint(sqrt(float(nofParticles))+1);

    float x = float(gid%width) / float(width) * 2 - 1;
    float y = float(gid/width) / float(width) * 2 - 1;

    vec2 vel = randVel(gid)*0.01;

    particles[gid*5+0] = x;
    particles[gid*5+1] = y;
    particles[gid*5+2] = vel.x;
    particles[gid*5+3] = vel.y;
    particles[gid*5+4] = (randF(gid,19993)*.5+.5)*5;
  }

  ).";

  vars.reCreate<Program>("method.resetProgram",std::make_shared<Shader>(GL_COMPUTE_SHADER,src));
}

void resetParticles(vars::Vars&vars){
  FUNCTION_CALLER();
  createResetProgram(vars);

  auto prg          = vars.get<Program>("method.resetProgram");
  auto nofParticles = vars.getUint32   ("method.nofParticles");
  auto buf          = vars.get<Buffer> ("method.buffer");

  buf->bindBase(GL_SHADER_STORAGE_BUFFER,0);

  prg->use();
  prg->set1ui("nofParticles",nofParticles);
  
  uint32_t nofWorkgroups = divRoundUp(nofParticles,256u);

  glDispatchCompute(nofWorkgroups,1,1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

}

void allocateParticles(vars::Vars&vars){
  FUNCTION_PROLOGUE("method","method.nofParticles");
  
  auto const nofParticles = vars.getUint32("method.nofParticles");
  size_t floatsPerPartices = 2 + 2 + 1;//position velocity mass

  vars.reCreate<Buffer>("method.buffer",sizeof(float)*floatsPerPartices*nofParticles);

  resetParticles(vars);
}

void createDrawProgram(vars::Vars&vars){
  FUNCTION_PROLOGUE("method");

  auto const vs = R".(
  #version 460

  layout(binding=0,std430)buffer Particles{float particles[];};

  out vec2 vVel;
  out vec2 vCoord;

  uniform float scale = 0.01f;

  void main(){
    uint id = gl_VertexID / 6;
    uint vid = gl_VertexID % 6;

    vec2 position = vec2(particles[id*5+0],particles[id*5+1]);
    vec2 velocity = vec2(particles[id*5+2],particles[id*5+3]);
    float mass    = particles[id*5+4];

    const vec2 offset[6] = {
      vec2(-1,-1),vec2(+1,-1),vec2(-1,+1),
      vec2(-1,+1),vec2(+1,-1),vec2(+1,+1)
    };

    vCoord = offset[vid];


    gl_Position = vec4(position+offset[vid]*scale*mass,0,1);
    vVel = velocity;
    gl_PointSize = mass;
  }
  ).";

  auto const fs = R".(
  #version 460

  layout(location=0)out vec4 fColor;

  in vec2 vVel;
  in vec2 vCoord;

  uniform int prettyMode = 1;

  void main(){
    if(prettyMode == 1){
      float v = length(vVel);

      float r = clamp(length(vCoord),0,1);

      float a = 1;
      if(r>1)a=0;

      fColor = vec4(v,v,v,(1-r));
    }else{
      fColor = vec4(1);
    }
  }
  ).";

  vars.reCreate<Program>("method.drawProgram",
      std::make_shared<Shader>(GL_VERTEX_SHADER,vs),
      std::make_shared<Shader>(GL_FRAGMENT_SHADER,fs)
      );
}

void createDrawVAO(vars::Vars&vars){
  FUNCTION_PROLOGUE("method");
  vars.reCreate<VertexArray>("method.vao");
}

void drawParticles(vars::Vars&vars){
  FUNCTION_CALLER();
  allocateParticles(vars);
  createDrawProgram(vars);
  createDrawVAO(vars);

  auto vao          = vars.get<VertexArray>("method.vao"         );
  auto prg          = vars.get<Program    >("method.drawProgram" );
  auto nofParticles = vars.getUint32       ("method.nofParticles");
  auto buf          = vars.get<Buffer     >("method.buffer"      );


  glEnable(GL_PROGRAM_POINT_SIZE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE);

  //resetParticles(vars);
  //std::vector<float>data;
  //buf->getData(data);
  //for(uint32_t i=0;i<100;++i){
  //  for(uint32_t j=0;j<5;++j)
  //    std::cerr << data[i*5+j] << " ";
  //  std::cerr << std::endl;
  //}
  //exit(0);

  vao->bind();
  prg->use();
  prg->set1f("scale",vars.addOrGetFloat("particleScale",0.01f));
  prg->set1i("prettyMode",vars.addOrGetBool("prettyMode",false));
  buf->bindBase(GL_SHADER_STORAGE_BUFFER,0);
  glDrawArrays(GL_TRIANGLES,0,nofParticles*6);
  vao->unbind();
}

void createUpdatePositionProgram(vars::Vars&vars){
  FUNCTION_PROLOGUE("method");

  std::string const src = R".(
  #version 460

  layout(local_size_x=256)in;

  layout(binding=0,std430)buffer Particles{float particles[];};

  uniform uint nofParticles = 1;
  uniform float dt = 0.001f;

  void main(){
    uint gid = gl_GlobalInvocationID.x;
    if(gid >= nofParticles)return;

    vec2 pos = vec2(particles[gid*5+0],particles[gid*5+1]);
    vec2 vel = vec2(particles[gid*5+2],particles[gid*5+3]);
    float mass = particles[gid*5+4];

    vec2 newPos = pos + vel*dt;

    if(newPos.x<-1.f || newPos.x>+1.f){
      newPos = pos;
      vel.x*=-1.f;
    }
    if(newPos.y<-1.f || newPos.y>+1.f){
      newPos = pos;
      vel.y*=-1.f;
    }

    particles[gid*5+0] = newPos.x;
    particles[gid*5+1] = newPos.y;
    particles[gid*5+2] = vel.x;
    particles[gid*5+3] = vel.y;
  
  }

  ).";

  vars.reCreate<Program>("method.updatePositionProgram",std::make_shared<Shader>(GL_COMPUTE_SHADER,src));
}

void updatePosition(vars::Vars&vars){
  auto prg = vars.get<Program>("method.updatePositionProgram");
  auto nofParticles = vars.getUint32("method.nofParticles");

  prg->use();
  prg->set1ui("nofParticles",nofParticles);
  prg->set1f("dt",vars.getFloat("method.dt"));

  auto nofWorkgroups = divRoundUp(nofParticles,256u);
  glDispatchCompute(nofWorkgroups,1,1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void createUpdateVelocityProgram(vars::Vars&vars){
  FUNCTION_PROLOGUE("method");

  std::string const src = 
    R".(
    #version 460

    layout(local_size_x=256)in;

    layout(binding=0,std430)buffer Particles{float particles[];};

    uniform uint nofParticles = 1;
    uniform float dt = 0.001f;
    uniform float kappa = 0.00001f;

    void main(){
      uint gid = gl_GlobalInvocationID.x;
      if(gid >= nofParticles)return;

    }
    ).";

  auto prg = vars.reCreate<Program>("method.updateVelocityProgram",std::make_shared<Shader>(GL_COMPUTE_SHADER,src));
  prg->setNonexistingUniformWarning(false);
}

void updateVelocities(vars::Vars&vars){
  auto prg = vars.get<Program>("method.updateVelocityProgram");
  auto nofParticles = vars.getUint32("method.nofParticles");

  prg->use();
  prg->set1ui("nofParticles",nofParticles);
  prg->set1f("dt",vars.getFloat("method.dt"));
  prg->set1f("kappa",vars.addOrGetFloat("kappa",0.00001f));

  auto nofWorkgroups = divRoundUp(nofParticles,256u);
  glDispatchCompute(nofWorkgroups,1,1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void simulate(vars::Vars&vars){
  createUpdatePositionProgram(vars);
  createUpdateVelocityProgram(vars);
  allocateParticles(vars);



  if(vars.addOrGetBool("updateVelocities",true))
    updateVelocities(vars);

  updatePosition(vars);
}


void onInit(vars::Vars&vars){
  glClearColor(0.,0.,0.,1);

  vars.addUint32("method.nofParticles",1000);
  vars.addFloat("method.dt",0.01f);
}

void onDraw(vars::Vars&vars){
  FUNCTION_CALLER();

  if(vars.addOrGetBool("clearColorBuffer",true))
    glClear(GL_COLOR_BUFFER_BIT);
  glClear(GL_DEPTH_BUFFER_BIT);

  drawParticles(vars);
  simulate(vars);

  ImGui::Begin("vars");
  if(ImGui::Button("reset"))
    resetParticles(vars);
  ImGui::End();

}

void onResize(vars::Vars&vars){
  auto width  = vars.getUint32("event.resizeX");
  auto height = vars.getUint32("event.resizeY");

  glViewport(0,0,width,height);
}

void onQuit(vars::Vars&vars){
  vars.erase("method");
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onInit   = onInit  ;
  clbs.onDraw   = onDraw  ;
  clbs.onResize = onResize;
  clbs.onQuit   = onQuit  ;
  MethodRegister::get().manager.registerMethod("gmu.nbody",clbs);
};

}

