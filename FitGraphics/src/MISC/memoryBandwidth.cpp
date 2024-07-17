#include<sstream>
#include<Vars/Vars.h>

#include<geGL/geGL.h>
#include<geGL/StaticCalls.h>
#include<imguiDormon/imgui.h>
#include<imguiVars/addVarsLimits.h>

#include<framework/methodRegister.hpp>
#include<framework/defineGLSLVersion.hpp>
#include<framework/Barrier.h>
#include<framework/Timer.hpp>
#include<PGR/01/emptyWindow.hpp>

using namespace emptyWindow;
using namespace ge::gl;
using namespace std;

namespace memoryBandwidth{

void createReadProgram(vars::Vars&vars){
  if(notChanged(vars,"method.all",__FUNCTION__,{"method.floatsPerThread","method.registersPerThread","method.workGroupSize","method.useReadProgram"}))return;

  std::cerr << "need to recompile program" << std::endl;

  auto const floatsPerThread    = vars.getSizeT("method.floatsPerThread"   );
  auto const registersPerThread = vars.getSizeT("method.registersPerThread");
  auto const workGroupSize      = vars.getSizeT("method.workGroupSize"     );

  stringstream ss;
  ss << "#version                     " << 450                << endl;
  ss << "#line                        " << __LINE__           << endl;
  ss << "#define FLOATS_PER_THREAD    " << floatsPerThread    << endl;
  ss << "#define REGISTERS_PER_THREAD " << registersPerThread << endl;
  ss << "#define WORKGROUP_SIZE       " << workGroupSize      << endl;
  ss << R".(

  #define FLOAT_CHUNKS (FLOATS_PER_THREAD / REGISTERS_PER_THREAD)

  layout(local_size_x=WORKGROUP_SIZE)in;
  layout(binding=0,std430)buffer Data{float data[];};

  void main(){
    uint lid  = gl_LocalInvocationID .x;
    uint gid  = gl_GlobalInvocationID.x;
    uint wgs  = gl_WorkGroupSize     .x;
    uint wid  = gl_WorkGroupID       .x;
    uint nwgs = gl_NumWorkGroups     .x;
    const uint workGroupOffset = wid*FLOATS_PER_THREAD*WORKGROUP_SIZE;
    float accumulator = 0.f;

    #if REGISTERS_PER_THREAD != 0

      float registers[REGISTERS_PER_THREAD];
      for(uint r=0;r<REGISTERS_PER_THREAD;++r)
        registers[r] = 0.f;

      for(uint f=0;f<FLOAT_CHUNKS;++f)
        for(uint r=0;r<REGISTERS_PER_THREAD;++r)
          registers[r] += data[lid + (f*REGISTERS_PER_THREAD+r)*wgs + workGroupOffset];
      for(uint r=0;r<REGISTERS_PER_THREAD;++r)
        accumulator += registers[r];

    #else

      for(uint f=0;f<FLOATS_PER_THREAD;++f)
        accumulator += data[lid + f*wgs + workGroupOffset];

    #endif

    if(accumulator == 1.337f)
      data[gid] = 0.f;
  }
  ).";

  vars.reCreate<Program>("method.program",make_shared<Shader>(GL_COMPUTE_SHADER,ss.str()));
}

void createWriteProgram(vars::Vars&vars){
  if(notChanged(vars,"method.all",__FUNCTION__,{"method.floatsPerThread","method.workGroupSize","method.useReadProgram"}))return;
  std::cerr << "need to recompile program" << std::endl;

  auto const floatsPerThread    = vars.getSizeT("method.floatsPerThread"   );
  auto const workGroupSize      = vars.getSizeT("method.workGroupSize"     );

  stringstream ss;
  ss << "#version                  " << 450             << endl;
  ss << "#line                     " << __LINE__        << endl;
  ss << "#define FLOATS_PER_THREAD " << floatsPerThread << endl;
  ss << "#define WORKGROUP_SIZE    " << workGroupSize   << endl;
  ss << R".(

  layout(local_size_x=WORKGROUP_SIZE)in;
  layout(binding=0,std430)buffer Data{float data[];};

  void main(){
    uint lid  = gl_LocalInvocationID .x;
    uint gid  = gl_GlobalInvocationID.x;
    uint wgs  = gl_WorkGroupSize     .x;
    uint wid  = gl_WorkGroupID       .x;
    uint nwgs = gl_NumWorkGroups     .x;
    const uint workGroupOffset = wid*FLOATS_PER_THREAD*WORKGROUP_SIZE;

    for(uint f=0;f<FLOATS_PER_THREAD;++f){
      data[lid + f*wgs + workGroupOffset] = lid + f*wgs + workGroupOffset;
      //float v = data[lid + f*wgs + workGroupOffset];
      //data[nwgs*wgs*FLOATS_PER_THREAD-(lid + f*wgs + workGroupOffset)-1] = v*3.3f;
    }
  }
  ).";
  vars.reCreate<Program>("method.program",make_shared<Shader>(GL_COMPUTE_SHADER,ss.str()));
}

void createProgram(vars::Vars&vars){
  if(vars.getBool("method.useReadProgram"))
    createReadProgram(vars);
  else
    createWriteProgram(vars);
}

void createBuffer(vars::Vars&vars){
  if(notChanged(vars,"method.all",__FUNCTION__,{"method.floatsPerThread","method.workGroupSize","method.nofWorkGroups"}))return;
  std::cerr << "need to reallocate buffer" << std::endl;

  auto const workGroupSize   = vars.getSizeT("method.workGroupSize"  );
  auto const nofWorkGroups   = vars.getSizeT("method.nofWorkGroups"  );
  auto const floatsPerThread = vars.getSizeT("method.floatsPerThread");

  size_t const bufferSize = workGroupSize * nofWorkGroups * floatsPerThread;
  vars.reCreate<Buffer>("method.buffer",bufferSize);
}



GLuint query;

void onInit(vars::Vars&vars){
  vars.addBool ("method.useReadProgram"     ,false);
  vars.addSizeT("method.workGroupSize"     ,128  );
  vars.addSizeT("method.nofWorkGroups"     ,28   );
  vars.addSizeT("method.floatsPerThread"   ,1024 );
  vars.addSizeT("method.registersPerThread",0    );
  glCreateQueries(GL_TIME_ELAPSED,1,&query);
}

void onDraw(vars::Vars&vars){
  ge::gl::glClearColor(0.1f,0.1f,0.1f,1.f);
  ge::gl::glClear(GL_COLOR_BUFFER_BIT);

  createProgram(vars);
  createBuffer(vars);

  auto buffer  = vars.get<Buffer >("method.buffer" );
  auto program = vars.get<Program>("method.program");

  //auto qr = make_shared<AsynchronousQuery>(GL_TIME_ELAPSED,GL_QUERY_RESULT,AsynchronousQuery::UINT64);

  buffer->clear(GL_R32F,GL_RED,GL_FLOAT);
  program->use();
  program->bindBuffer("Data",buffer);
  glMemoryBarrier(GL_ALL_BARRIER_BITS);
  glFinish();

  auto timer = Timer<double>();
  timer.reset();
  glBeginQuery(GL_TIME_ELAPSED,query);
  program->dispatch(vars.getSizeT("method.nofWorkGroups"));
  glMemoryBarrier(GL_ALL_BARRIER_BITS);
  glFinish();
  glEndQuery(GL_TIME_ELAPSED);
  GLint64 tt;
  glGetQueryObjecti64v(query,GL_QUERY_RESULT,&tt);
  auto time = timer.elapsedFromStart();
  float ttt = (float)((double)tt / (double)1000000000);
  //std::cerr << "cpu: " << time << " query: " << ttt << std::endl;
  //std::vector<float>a;
  //a.resize(100);
  //buffer->getData(a);

  auto&nofWorkGroups      = vars.getSizeT("method.nofWorkGroups");
  auto&workGroupSize      = vars.getSizeT("method.workGroupSize");
  auto&floatsPerThread    = vars.getSizeT("method.floatsPerThread");
  auto&registersPerThread = vars.getSizeT("method.registersPerThread");
  auto&useReadProgram     = vars.getBool ("method.useReadProgram");

  uint64_t const nanoSecondsInSecond = 1e9;
  uint64_t const gigabyte = 1024*1024*1024;
  size_t minWorkGroupSize      = 1        ;
  size_t maxWorkGroupSize      = 1024     ;
  size_t minFloatsPerThread    = 1        ;
  size_t maxFloatsPerThread    = 100000   ;
  size_t minNofWorkGroups      = 1        ;
  size_t maxNofWorkGroups      = 1024*1024;
  size_t minRegistersPerThread = 0        ;
  size_t maxRegistersPerThread = 4096     ;


  double bandwidthInGigabytes = 0.;

  //time = static_cast<double>(timeInNanoseconds) / static_cast<double>(nanoSecondsInSecond);
  auto const bufferSize = nofWorkGroups * workGroupSize * floatsPerThread * sizeof(float);
  auto const bandwidth = bufferSize / time;
  bandwidthInGigabytes = bandwidth / static_cast<double>(gigabyte);

  /*
  ImGui::Checkbox("use read program",&useReadProgram);
  ImGui::DragScalar("floatsPerThread"     ,ImGuiDataType_U64,&floatsPerThread   ,1,&minFloatsPerThread   ,&maxFloatsPerThread   );
  ImGui::DragScalar("workGroupSize"       ,ImGuiDataType_U64,&workGroupSize     ,1,&minWorkGroupSize     ,&maxWorkGroupSize     );
  ImGui::DragScalar("number of workgroups",ImGuiDataType_U64,&nofWorkGroups     ,1,&minNofWorkGroups     ,&maxNofWorkGroups     );
  if(useReadProgram)
    ImGui::DragScalar("registers per thread",ImGuiDataType_U64,&registersPerThread,1,&minRegistersPerThread,&maxRegistersPerThread);
  // */

  ImGui::Begin("perf");
  ImGui::Text("buffer Size : %f [GB]"   ,static_cast<float>(workGroupSize * nofWorkGroups * floatsPerThread * sizeof(float))/static_cast<float>(gigabyte));
  ImGui::Text("time        : %lf [s]"   ,time);
  ImGui::Text("bandwidth   : %lf [GB/s]",bandwidthInGigabytes);
  ImGui::End();

}

void onQuit(vars::Vars&vars){
  vars.erase("method");
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw        =              onDraw       ;
  clbs.onInit        =              onInit       ;
  clbs.onQuit        =              onQuit       ;
  clbs.onResize      = emptyWindow::onResize     ;
  MethodRegister::get().manager.registerMethod("misc.memoryBandwidth",clbs);
};


}

