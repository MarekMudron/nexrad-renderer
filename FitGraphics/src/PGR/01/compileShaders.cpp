#include<SDL.h>
#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<framework/methodRegister.hpp>
#include<PGR/01/compileShaders.hpp>
#include<PGR/01/emptyWindow.hpp>

using namespace ge::gl;

namespace compileShaders{

GLuint prg = 0;
GLuint vao = 0;

void error(std::string const&name,std::string const&msg){
  bool errorCmd = false;
  if(errorCmd){
    std::cerr << name << std::endl;
    std::cerr << msg << std::endl;
  }else{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,name.c_str(),msg.c_str(),nullptr);
  }
}

std::string shaderTypeToName(GLuint type){
  if(type==GL_VERTEX_SHADER         )return "vertex"    ;
  if(type==GL_FRAGMENT_SHADER       )return "fragment"  ;
  if(type==GL_GEOMETRY_SHADER       )return "geometry"  ;
  if(type==GL_TESS_CONTROL_SHADER   )return "control"   ;
  if(type==GL_TESS_EVALUATION_SHADER)return "evaluation";
  if(type==GL_COMPUTE_SHADER        )return "compute"   ;
  return "unknown";
}

GLuint createShader(GLuint type,std::string const&src){
  GLuint vs = glCreateShader(type);
  char const*vsSrc[1]={
    src.c_str()
  };

  glShaderSource(vs,1,vsSrc,nullptr);
  glCompileShader(vs);
  int compileStatus;
  glGetShaderiv(vs,GL_COMPILE_STATUS,&compileStatus);
  if(compileStatus != GL_TRUE){
    uint32_t const msgLen = 1<<11;
    char msg[msgLen];
    glGetShaderInfoLog(vs,msgLen,nullptr,msg);

    error(shaderTypeToName(type)+" shader compilation error",msg);
  }
  return vs;
}

GLuint createProgram(std::vector<GLuint>const&shaders){
  GLuint prg = glCreateProgram();

  for(auto const&shader:shaders)
    glAttachShader(prg,shader);

  glLinkProgram(prg);
  GLint linkStatus;
  glGetProgramiv(prg,GL_LINK_STATUS,&linkStatus);
  if(linkStatus != GL_TRUE){
    uint32_t const msgLen = 1<<11;
    char msg[msgLen];
    glGetProgramInfoLog(prg,msgLen,nullptr,msg);
    error("program linking error",msg);
  }

  for(auto const&shader:shaders)
    glDeleteShader(shader);

  return prg;
}


void onInit(vars::Vars&vars){
  auto vsSrc = R".(
  #version 460
  void main(){
    gl_Position = vec4(gl_VertexID&1,gl_VertexID>>1,0,1);
  }
  ).";
  
  auto fsSrc = R".(
  #version 460
  layout(location=0)out vec4 fColor;
  void main(){
    fColor = vec4(1);
  }
  ).";

  glCreateVertexArrays(1,&vao);

  prg = createProgram({
      createShader(GL_VERTEX_SHADER  ,vsSrc),
      createShader(GL_FRAGMENT_SHADER,fsSrc)});

  glClearColor(0.3,0.0,0.3,1);
}

void onDraw(vars::Vars&vars){
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glBindVertexArray(vao);
  glUseProgram(prg);
  glDrawArrays(GL_TRIANGLES,0,3);
}

void onQuit(vars::Vars&vars){
  glDeleteProgram(prg);
  glDeleteVertexArrays(1,&vao);
  prg = 0;
  vao = 0;
  glUseProgram(prg);
  glBindVertexArray(vao);
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw   = onDraw  ;
  clbs.onInit   = onInit  ;
  clbs.onQuit   = onQuit  ;
  clbs.onResize = emptyWindow::onResize;
  MethodRegister::get().manager.registerMethod("pgr01.compileShaders",clbs);
};

}
