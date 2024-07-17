#include<framework/defineGLSLVersion.hpp>
#include<sstream>
#include<geGL/StaticCalls.h>
#include<cstring>

using namespace ge::gl;

std::string defineGLSLVersion(){
  std::stringstream ss;
  GLint major;
  GLint minor;
  auto ver = std::string((char const*)glGetString(GL_SHADING_LANGUAGE_VERSION));
  if(ver.length()>2){
    major = (GLint)(ver[0]-'0');
    minor = (GLint)(ver[2]-'0');
  }else{
    glGetIntegerv(GL_MAJOR_VERSION,&major);
    glGetIntegerv(GL_MAJOR_VERSION,&minor);
  }

  ss << "#version ";
  ss << major;
  ss << minor;
  ss << "0";
  ss << std::endl;
  return ss.str();
}
