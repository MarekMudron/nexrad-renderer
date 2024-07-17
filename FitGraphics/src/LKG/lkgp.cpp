#include<fstream>
#include<SDL.h>
#include<glm/glm.hpp>
#include<glm/gtc/matrix_transform.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<Vars/Vars.h>
#include<geGL/geGL.h>
#include<geGL/StaticCalls.h>
#include<framework/methodRegister.hpp>
#include<BasicCamera/OrbitCamera.h>
#include<BasicCamera/PerspectiveCamera.h>
#include<framework/Timer.hpp>
#include<framework/Barrier.h>
#include<framework/FunctionPrologue.h>
#include <imguiSDL2OpenGL/imgui.h>

using namespace ge::gl;

namespace lkgp{

bool errorCmd=false;

void error(std::string const&name,std::string const&msg){
  if(errorCmd){
    std::cerr << name << std::endl;
    std::cerr << msg << std::endl;
  }else{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,name.c_str(),msg.c_str(),nullptr);
  }
}

std::string loadTxtFile(std::string const&fileName){
  auto file = std::ifstream(fileName);
  if(!file.is_open()){
    error("cannot open file: "+fileName,"text file: "+fileName+" cannot be opened");
    return "";
  }
  std::string str((std::istreambuf_iterator<char>(file)),
                 std::istreambuf_iterator<char>());
  return str;
}

std::string shaderTypeToName(GLuint type){
  if(type==GL_VERTEX_SHADER  )return "vertex";
  if(type==GL_FRAGMENT_SHADER)return "fragment";
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

GLuint createProgram(GLuint vs,GLuint fs){
  GLuint prg = glCreateProgram();
  glAttachShader(prg,vs);
  glAttachShader(prg,fs);

  glLinkProgram(prg);
  GLint linkStatus;
  glGetProgramiv(prg,GL_LINK_STATUS,&linkStatus);
  if(linkStatus != GL_TRUE){
    uint32_t const msgLen = 1<<11;
    char msg[msgLen];
    glGetProgramInfoLog(prg,msgLen,nullptr,msg);
    error("program linking error",msg);
  }
  return prg;
}

struct Frame{
  std::vector<uint8_t>data;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t channels = 0;
};

void flipFrame(Frame&frame){
  for(uint32_t y=0;y<frame.height/2;++y)
    for(uint32_t x=0;x<frame.width;++x){
      for(uint32_t c=0;c<frame.channels;++c){
        auto src = (y*frame.width+x)*frame.channels+c;
        auto dst = ((frame.height-1-y)*frame.width+x)*frame.channels+c;
        auto z=frame.data.at(src);
        frame.data.at(src) = frame.data.at(dst);
        frame.data.at(dst) = z;
      }
    }
}

Frame dumpFrame(SDL_Window*window){
  Frame res;

  res.channels = 3;
  SDL_GetWindowSize(window,(int*)&res.width,(int*)&res.height);
  res.data.resize(res.channels*res.width*res.height);

  glReadPixels(0,0,res.width,res.height,GL_RGB,GL_UNSIGNED_BYTE,res.data.data());
  return res;
}

//void saveFrame(std::string const&file,Frame const&f){
//  auto frame = f;
//  flipFrame(frame);
//  stbi_write_png(file.c_str(),frame.width,frame.height,3,frame.data.data(),0);
//  std::cerr << "storing screenshot to: \"" << file << "\"" << std::endl;
//}
//
//Frame loadFrame(std::string const&file){
//  Frame res;
//  uint8_t* data = stbi_load(file.c_str(),(int*)&res.width,(int*)&res.height,(int*)&res.channels,0);
//  if(!data){
//    error("loadFrame","cannot load image: "+file);
//    return res;
//  }
//
//  res.data.resize(res.width*res.height*res.channels);
//  for(uint32_t i=0;i<res.width*res.height*res.channels;++i)
//    res.data.at(i) = data[i];
//
//  stbi_image_free(data);
//  flipFrame(res);
//  return res;
//}

float meanSquareError(Frame const&a,Frame const&b){
  if(a.channels    != b.channels   )return 1e10;
  if(a.width       != b.width      )return 1e10;
  if(a.height      != b.height     )return 1e10;
  if(a.data.size() != b.data.size())return 1e10;

  float meanSquareError = 0;
  for (uint32_t y = 0; y < a.height; ++y)
    for (uint32_t x = 0; x < a.width; ++x){
      for (uint32_t c = 0; c < a.channels; ++c) {
        auto pix = (y*a.width+x)*a.channels+c;
        uint8_t ucol = a.data[pix];
        uint8_t gcol = b.data[pix];
        float diff = glm::abs((float)ucol - (float)gcol);
        diff *= diff;
        meanSquareError += diff;
      }
    }

  meanSquareError /= (float)(a.width * a.height * a.channels);
  return meanSquareError;
}

#ifndef CMAKE_ROOT_DIR
#define CMAKE_ROOT_DIR "."
#endif



basicCamera::OrbitCamera       orbitCamera      ;
basicCamera::PerspectiveCamera perspectiveCamera;
Timer<float>timer;

GLuint vao;

void getProgramInterface(vars::Vars&vars,std::string const&prgName){
  FUNCTION_PROLOGUE("remsub",prgName);

  auto prg = vars.get<ge::gl::Program>(prgName);
  auto info = prg->getInfo();
  vars.erase("method.uniforms");
  for(auto const&u:info->uniforms){
    auto const&name = u.first;
    if(name.find("[")!=std::string::npos)continue;
    auto const&unif = u.second;
    auto type = std::get<ge::gl::ProgramInfo::TYPE>(unif);

    auto vName = "method.uniforms."+name;
    switch(type){
      case GL_UNSIGNED_INT:
        {
          uint32_t v;
          ge::gl::glGetUniformuiv(prg->getId(),prg->getUniformLocation(name),&v);
          vars.addUint32(vName,v);break;
        }
      case GL_INT:
        {
          int v;
          ge::gl::glGetUniformiv(prg->getId(),prg->getUniformLocation(name),(int*)&v);
          vars.addInt32(vName,v);break;
        }
      case GL_INT_VEC2:
        {
          glm::ivec2 v;
          ge::gl::glGetUniformiv(prg->getId(),prg->getUniformLocation(name),(int*)&v);
          vars.add<glm::ivec2>(vName,v);break;
        }
      case GL_FLOAT       :
        {
          float v;
          ge::gl::glGetUniformfv(prg->getId(),prg->getUniformLocation(name),&v);
          vars.addFloat (vName,v);break;
        }
      case GL_BOOL:
        {
          bool v;
          ge::gl::glGetUniformiv(prg->getId(),prg->getUniformLocation(name),(int*)&v);
          vars.addBool (vName,v);break;
        }
      case GL_FLOAT_VEC2:
        {
          glm::vec2 v;
          ge::gl::glGetUniformfv(prg->getId(),prg->getUniformLocation(name),(float*)&v);
          vars.add<glm::vec2>(vName,v);break;
        }
      default:break;


    }


  }
}

void setProgramUniforms(vars::Vars&vars,std::string const&prgName){
  auto prg = vars.get<ge::gl::Program>(prgName);
  std::vector<std::string>uniformNames;
  vars.getDir(uniformNames,"method.uniforms");
  for(auto const&n:uniformNames){
    auto vName = "method.uniforms."+n;
    auto const&type = vars.getType(vName);
    if(type == typeid(bool    ))prg->set1i (n,vars.getBool  (vName));
    if(type == typeid(uint32_t))prg->set1ui(n,vars.getUint32(vName));
    if(type == typeid( int32_t))prg->set1i (n,vars.getInt32 (vName));
    if(type == typeid(float   ))prg->set1f (n,vars.getFloat (vName));
    //if(type == typeid(glm::vec2))prg->set2fv(n,(float*)&vars.getVec2(vName));
  }
}

void loadProgramFromFile(vars::Vars&vars){
  auto shaderFile = vars.getString("method.shaderFile");
  auto version    = vars.getString("method.glsl_version");

  auto src = loadTxtFile(shaderFile);
  auto vs = std::make_shared<ge::gl::Shader>(GL_VERTEX_SHADER,
      "#version "+version+"\n",
      "#define VERTEX_SHADER\n",
      src
      );
  auto fs = std::make_shared<ge::gl::Shader>(GL_FRAGMENT_SHADER,
      "#version "+version+"\n",
      "#define FRAGMENT_SHADER\n",
      src
      );
  auto prg = vars.reCreate<ge::gl::Program>("method.program",vs,fs);
  prg->setNonexistingUniformWarning(false);
}

void createProgram(vars::Vars&vars){
  loadProgramFromFile(vars);
  getProgramInterface(vars,"method.program");
}

void onResize(vars::Vars&vars){
  auto width  = vars.getUint32("event.resizeX");
  auto height = vars.getUint32("event.resizeY");
  perspectiveCamera.setAspect((float)width/(float)height);

  glViewport(0,0,width,height);
}

void onInit(vars::Vars&vars){
  glClearColor(0.1f,0.2f,0.2f,1.0f);

  vars.addOrGetUint32("method.triangles",10000);
  orbitCamera.addDistance(3.f);
  orbitCamera.addXAngle(glm::radians(30.f));
  orbitCamera.addYAngle(glm::radians(-30.f));

  perspectiveCamera.setNear(0.01f);
  perspectiveCamera.setFar(1000.f);
  perspectiveCamera.setFovy(glm::radians(90.f));
  perspectiveCamera.setAspect(1.f);
   
  vars.addFloat("method.sensitivity",0.01f);
  vars.addFloat("method.zoomSpeed",0.02f);

  vars.addString("method.glsl_version","460");
  vars.addString("method.shaderFile",std::string(CMAKE_ROOT_DIR)+"/resources/shaders/sphere_ray.glsl");

  createProgram(vars);

  glCreateVertexArrays(1,&vao);

  ImGui::GetStyle().ScaleAllSizes(4.f);
  ImGui::GetIO().FontGlobalScale = 4.f;

  timer = Timer<float>();

}

void onMouseMotion(vars::Vars&vars){
  auto xrel = vars.getInt32("event.mouse.xrel");
  auto yrel = vars.getInt32("event.mouse.yrel");

  auto sensitivity = vars.getFloat("method.sensitivity"    );
  auto zoomSpeed   = vars.getFloat("method.zoomSpeed"      );
  if(vars.getBool("event.mouse.left")){
    orbitCamera.addXAngle(sensitivity * yrel);
    orbitCamera.addYAngle(sensitivity * xrel);
  }
  if(vars.getBool("event.mouse.right")){
    orbitCamera.addDistance(yrel * zoomSpeed);
  }
  if(vars.getBool("event.mouse.middle")){
    auto width  = vars.getUint32("event.resizeX");
    auto height = vars.getUint32("event.resizeY");
    orbitCamera.addXPosition(+orbitCamera.getDistance() * xrel /
                              float(width) * 2.f);
    orbitCamera.addYPosition(-orbitCamera.getDistance() * yrel /
                              float(height) * 2.f);
  }
}

std::map<int,bool>keys;
void onKeyDown(vars::Vars&vars){
  auto key = vars.getInt32("event.key");
  keys[key] = true;

  if(key == SDLK_r){
    std::cerr << "shader reloaded" << std::endl;
    createProgram(vars);
  }

  if(key == SDLK_f){
    auto window = *vars.get<SDL_Window*>("window");
    auto flags = SDL_GetWindowFlags(window);
    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP)
      SDL_SetWindowFullscreen(window, 0);
    else
      SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
  }
}

void onKeyUp(vars::Vars&vars){
  auto key = vars.getInt32("event.key");
  keys[key] = false;
}

void onUpdate(vars::Vars&vars){
  auto dt = vars.getFloat("event.dt");

  auto keyVector = glm::vec3(
      keys[SDLK_a     ]-keys[SDLK_d    ],
      keys[SDLK_LSHIFT]-keys[SDLK_SPACE],
      keys[SDLK_w     ]-keys[SDLK_s    ]);

  //position += dt * keyVector*glm::mat3(viewRotation);
  //computeFreelookMatrix(vars);
}

void onDraw(vars::Vars&vars){
  glClearColor(0.5,0.5,1,1);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  auto viewMatrix = orbitCamera.getView();
  auto projMatrix = perspectiveCamera.getProjection();
  auto width      = vars.getUint32("event.resizeX"   );
  auto height     = vars.getUint32("event.resizeY"   );
  auto triangles  = vars.getUint32("method.triangles");

  glBindVertexArray(vao);

  auto prg=vars.get<ge::gl::Program>("method.program");
  prg->use();

  setProgramUniforms(vars,"method.program");
  prg->setMatrix4fv("view",(float*)&viewMatrix);
  prg->setMatrix4fv("proj",(float*)&projMatrix);
  prg->set1f       ("iTime",timer.elapsedFromStart());
  prg->set2f       ("iResolution",width,height);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
  glDrawArrays(GL_TRIANGLES,0,triangles*3);
}


void onQuit(vars::Vars&vars){
  ImGui::GetIO().FontGlobalScale = 1.f;
  ImGui::GetStyle().ScaleAllSizes(.25f);
  glDeleteVertexArrays(1,&vao);
  vars.erase("method");
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onInit        = onInit       ;
  clbs.onQuit        = onQuit       ;
  clbs.onDraw        = onDraw       ;
  clbs.onResize      = onResize     ;
  clbs.onKeyDown     = onKeyDown    ;
  clbs.onKeyUp       = onKeyUp      ;
  clbs.onMouseMotion = onMouseMotion;
  clbs.onUpdate      = onUpdate     ;
  MethodRegister::get().manager.registerMethod("lkg.lkgp",clbs);
};

}
