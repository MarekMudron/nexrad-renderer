#include<glm/glm.hpp>
#include<glm/gtc/type_ptr.hpp>
#include<SDL.h>
#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<geGL/geGL.h>
#include<imguiDormon/imgui.h>
#include<imguiVars/addVarsLimits.h>
#include<framework/FunctionPrologue.h>
#include<framework/methodRegister.hpp>
#include<PGR/01/emptyWindow.hpp>
#include<PGR/01/compileShaders.hpp>

#include<libs/stb_image/stb_image.h>

using namespace ge::gl;
using namespace compileShaders;

namespace texture{

class TextureData{
  public:
    std::vector<uint8_t>data;
    uint32_t width    = 0;
    uint32_t height   = 0;
    uint32_t channels = 0;
    TextureData(){}
    TextureData(uint32_t w,uint32_t h,uint32_t c):width(w),height(h),channels(c){
      data.resize((size_t)w*h*c,0);
    }
};

TextureData loadTexture(std::string const&fileName){
  TextureData res;

  int32_t w,h,channels;
  uint8_t* data = stbi_load(fileName.c_str(),&w,&h,&channels,0);
  if(!data){
    std::cerr << "Cannot open image file: "<<fileName<<std::endl;
    return res;
  }
  //std::cerr << "w: " << w << " h: " << h << " c: " << channels << std::endl;
  res.data.resize(w*h*channels);

  for(int32_t y=0;y<h;++y)
    for(int32_t x=0;x<w;++x)
      for(int32_t c=0;c<channels;++c){
        res.data[(y*w+x)*channels+c] = data[((h-y-1)*w+x)*channels+c];
      }

  res.channels = channels;
  res.height = h;
  res.width = w;
  stbi_image_free(data);
  return res;
}

std::string const source = R".(
#line 20

#ifdef VERTEX_SHADER
out vec2 vCoord;
void main(){
  vCoord = vec2(gl_VertexID&1,gl_VertexID>>1);
  gl_Position = vec4(vCoord*2.f-1.f,0.f,1.f);
}
#endif

#ifdef FRAGMENT_SHADER
in vec2 vCoord;
layout(binding=0)uniform sampler2D tex;
layout(location=0)out vec4 fColor;
void main(){
  fColor = texture(tex,vCoord);
}
#endif
).";

GLuint tex;

#ifndef CMAKE_ROOT_DIR
#define CMAKE_ROOT_DIR "."
#endif

GLuint createTexture(std::string const&file){
  auto texData = loadTexture(file);

  GLuint res;
  glCreateTextures(GL_TEXTURE_2D,1,&res);
  GLenum internalFormat = GL_RGB;
  GLenum format         = GL_RGB;
  if(texData.channels==4){
    internalFormat = GL_RGBA;
    format         = GL_RGBA;
  }
  if(texData.channels==3){
    internalFormat = GL_RGB ;
    format         = GL_RGB ;
  }
  if(texData.channels==2){
    internalFormat = GL_RG  ;
    format         = GL_RG;
  }
  if(texData.channels==1){
    internalFormat = GL_R   ;
    format         = GL_R   ;
  }

  glTextureImage2DEXT(
      res                 ,//texture
      GL_TEXTURE_2D       ,//target
      0                   ,//mipmap level
      internalFormat      ,//gpu format
      texData.width       ,
      texData.height      ,
      0                   ,//border
      format              ,//cpu format
      GL_UNSIGNED_BYTE    ,//cpu type
      texData.data.data());//pointer to data

  glGenerateTextureMipmap(res);
  glTextureParameteri(res,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
  glTextureParameteri(res,GL_TEXTURE_MAG_FILTER,GL_LINEAR              );
  return res;
}

void onInit(vars::Vars&vars){

  vars.reCreate<ge::gl::Program>("method.prg",
      std::make_shared<ge::gl::Shader>(GL_VERTEX_SHADER    ,"#version 460\n#define   VERTEX_SHADER\n"  +source),
      std::make_shared<ge::gl::Shader>(GL_FRAGMENT_SHADER  ,"#version 460\n#define FRAGMENT_SHADER\n"  +source)
      );

  vars.reCreate<ge::gl::VertexArray>("method.vao");

  tex = createTexture(std::string(CMAKE_ROOT_DIR)+"/resources/images/example.png");

  glClearColor(0.4,0.0,0.3,1);
  glEnable(GL_DEPTH_TEST);
}

void onDraw(vars::Vars&vars){
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  auto prg = vars.get<ge::gl::Program    >("method.prg");
  auto vao = vars.get<ge::gl::VertexArray>("method.vao");

  prg->use();
  vao->bind();

  glBindTextureUnit(0,tex);
  glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}

void onQuit(vars::Vars&vars){
  glDeleteTextures(1,&tex);
  vars.erase("method");
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw        =              onDraw       ;
  clbs.onInit        =              onInit       ;
  clbs.onQuit        =              onQuit       ;
  clbs.onResize      = emptyWindow::onResize     ;
  MethodRegister::get().manager.registerMethod("pgr02.texture",clbs);
};

}
