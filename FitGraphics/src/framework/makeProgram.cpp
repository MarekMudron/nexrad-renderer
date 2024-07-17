#include<framework/makeProgram.hpp>
#include<framework/defineGLSLVersion.hpp>

using namespace std;
using namespace ge::gl;

shared_ptr<Shader>makeShader(GLuint type,std::string const&src){
  return make_shared<Shader>(type,src);
}

bool hasSubstring(std::string const&txt,std::string w){
  return txt.find(w) != std::string::npos;
}

#define APPLY_ON_GL_SHADERS(f)                            \
  f(         GL_VERTEX_SHADER,    "VERTEX_SHADER");\
  f(   GL_TESS_CONTROL_SHADER,   "CONTROL_SHADER");\
  f(GL_TESS_EVALUATION_SHADER,"EVALUATION_SHADER");\
  f(       GL_GEOMETRY_SHADER,  "GEOMETRY_SHADER");\
  f(       GL_FRAGMENT_SHADER,  "FRAGMENT_SHADER");\
  f(        GL_COMPUTE_SHADER,   "COMPUTE_SHADER");\




bool hasShader(std::string const&txt,GLenum type){
#define HAS(a,b)if(type == a)return hasSubstring(txt,b)
  APPLY_ON_GL_SHADERS(HAS);
#undef HAS
  return false;
}

shared_ptr<Program>makeProgram(std::string const&src){
  std::vector<shared_ptr<Shader>>shaders;
#define MAKE_SHADER(a,b)\
  if(hasShader(src,a))shaders.push_back(makeShader(a,defineGLSLVersion()+"\n#define     " b " \n"+src));
  APPLY_ON_GL_SHADERS(MAKE_SHADER);
#undef MAKE_SHADER
  auto prg = make_shared<Program>(shaders);
  prg->setNonexistingUniformWarning(false);
  return prg;
};


