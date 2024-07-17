#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<framework/methodRegister.hpp>
#include<PGR/01/emptyWindow.hpp>
#include<PGR/01/compileShaders.hpp>
#include<PGR/01/vertexArrays.hpp>

using namespace ge::gl;
using namespace emptyWindow;
using namespace compileShaders;

namespace vertexArrays{

GLuint vbo;
GLuint ebo;
GLuint vao;
GLuint prg;

std::string source = R".(
#ifdef VERTEX_SHADER

layout(location=0)in vec2 position;
layout(location=1)in vec3 color   ;

out vec3 vColor;

void main(){
  vColor = color;
  gl_Position = vec4(position,0.f,1.f);
}
#endif

#ifdef FRAGMENT_SHADER
in vec3 vColor;
layout(location=0)out vec4 fColor;
void main(){
  fColor = vec4(vColor,1);
}
#endif
).";

template<typename T>
GLuint createBuffer(std::vector<T>const&data){
  GLuint id;
  glCreateBuffers(1,&id);
  glNamedBufferData(
      id,
      sizeof(T)*data.size(),
      data.data(),
      GL_DYNAMIC_DRAW);
  return id;
}

void setVertexAttrib(
    GLuint   vao   ,
    GLuint   attrib,
    GLint    size  ,
    GLenum   type  ,
    GLuint   buffer,
    GLintptr offset,
    GLsizei  stride){
  glVertexArrayAttribBinding(
      vao     ,
      attrib  , //attrib index
      attrib  );//binding index

  glEnableVertexArrayAttrib(
      vao     ,
      attrib  ); // attrib index

  glVertexArrayAttribFormat(
      vao     ,
      attrib  , //attrib index
      size    ,
      type    ,
      GL_FALSE, //normalization
      0       );//relative offset

  glVertexArrayVertexBuffer(
      vao     ,
      attrib  , //binding index
      buffer  , 
      offset  , 
      stride  );
}

void onInit(vars::Vars&){
  glClearColor(0.1,0,0,1);
  prg = createProgram({
      createShader(GL_VERTEX_SHADER  ,"#version 460\n#define VERTEX_SHADER\n"  +source),
      createShader(GL_FRAGMENT_SHADER,"#version 460\n#define FRAGMENT_SHADER\n"+source)});

  struct Vertex{
    float position[2];
    float color   [3];
  };


  //czech flag vertices
  std::vector<Vertex>vertices = {
    //blue
    {0.25f,0.25f,0,0,1},
    {0.50f,0.50f,0,0,1},
    {0.25f,0.75f,0,0,1},

    //red
    {0.25f,0.25f,1,0,0},
    {0.75f,0.25f,1,0,0},
    {0.75f,0.50f,1,0,0},
    {0.50f,0.50f,1,0,0},

    //white
    {0.50f,0.50f,1,1,1},
    {0.75f,0.50f,1,1,1},
    {0.75f,0.75f,1,1,1},
    {0.25f,0.75f,1,1,1},
  };

  std::vector<uint32_t>indices = {
    0,1, 2,//blue triangle
    3,4, 5,//first red triangle
    3,5, 6,//second red triangle
    7,8, 9,//first white triangle
    7,9,10,//second white triangle
  };

  vbo = createBuffer(vertices);
  ebo = createBuffer(indices );

  glCreateVertexArrays(1,&vao);

  setVertexAttrib(vao,0,2,GL_FLOAT,vbo,0                       ,sizeof(Vertex));
  setVertexAttrib(vao,1,3,GL_FLOAT,vbo,sizeof(Vertex::position),sizeof(Vertex));

  glVertexArrayElementBuffer(vao,ebo);

}

void onQuit(vars::Vars&){
  glDeleteBuffers     (1,&vbo);
  glDeleteBuffers     (1,&ebo);
  glDeleteVertexArrays(1,&vao);
  glDeleteProgram     (   prg);
  vbo = 0;
  ebo = 0;
  vao = 0;
  prg = 0;
  glUseProgram     (prg);
  glBindVertexArray(vao);
}

void onDraw(vars::Vars&){
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glUseProgram(prg);
  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES,5*3,GL_UNSIGNED_INT,0);
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onInit   = onInit;
  clbs.onQuit   = onQuit;
  clbs.onDraw   = onDraw;
  clbs.onResize = emptyWindow::onResize;
  MethodRegister::get().manager.registerMethod("pgr01.vertexArrays",clbs);
};

}
