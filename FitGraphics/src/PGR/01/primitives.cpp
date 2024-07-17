#include<Vars/Vars.h>
#include<geGL/StaticCalls.h>
#include<imguiDormon/imgui.h>
#include<imguiVars/addVarsLimits.h>

#include<framework/methodRegister.hpp>
#include<framework/defineGLSLVersion.hpp>
#include<PGR/01/emptyWindow.hpp>
#include<PGR/01/compileShaders.hpp>
#include<PGR/01/primitives.hpp>

using namespace emptyWindow;
using namespace compileShaders;
using namespace ge::gl;

namespace primitives{

GLuint prg;
GLuint vao;
GLenum selectPolygon(vars::Vars&vars);
GLenum selectMode   (vars::Vars&vars);

void onInit(vars::Vars&vars){
  auto vsSrc = defineGLSLVersion()+R".(
  flat out vec3 vColor;
  uniform uint maxVertices = 10;
  void main(){
    float angle = radians(360.f* float(gl_VertexID) / float(maxVertices));
    float r = 0.7f;
    vColor = vec3(cos(angle),sin(angle),cos(angle*2))*.5f+.5f;
    gl_Position = vec4(r*cos(angle),r*sin(angle),0,1);
  }
  ).";
  auto fsSrc = defineGLSLVersion()+R".(

  layout(location=0)out vec4 fColor;
  flat in vec3 vColor;
  void main(){
    fColor = vec4(vColor,1);
  }
  ).";

  prg = createProgram({
      createShader(GL_VERTEX_SHADER  ,vsSrc),
      createShader(GL_FRAGMENT_SHADER,fsSrc)
      });

  glCreateVertexArrays(1,&vao);

  glClearColor(0.3,0.2,0.1,1);

  vars.addUint32("method.pointSize",3);
  vars.addUint32("method.lineWidth",3);
  vars.addUint32("method.maxVertices",10);
  addVarsLimitsU(vars,"method.pointSize",1,100);
  addVarsLimitsU(vars,"method.lineWidth",1,100);
}

void onDraw(vars::Vars&vars){
  auto polygon     = selectPolygon(vars);
  auto mode        = selectMode   (vars);
  auto pointSize   = vars.getUint32("method.pointSize");
  auto lineWidth   = vars.getUint32("method.lineWidth");
  auto maxVertices = vars.getUint32("method.maxVertices");

  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  glPolygonMode(GL_FRONT_AND_BACK,mode);
  glPointSize  (pointSize);
  glLineWidth  (lineWidth);

  glBindVertexArray(vao);
  glProgramUniform1ui(prg,glGetUniformLocation(prg,"maxVertices"),maxVertices);
  glUseProgram(prg);
  
  glDrawArrays(polygon,0,maxVertices);
}

void onQuit(vars::Vars&vars){
  glDeleteProgram(prg);
  glDeleteVertexArrays(1,&vao);
  prg = 0;
  vao = 0;
  glUseProgram(prg);
  glBindVertexArray(vao);
  vars.erase("method");
}

GLenum selectMode(vars::Vars&vars){
  char const* modeNames[]={
    "GL_POINT",
    "GL_LINE ",
    "GL_FILL ",
  };
  GLenum modes[]={
    GL_POINT,
    GL_LINE ,
    GL_FILL ,
  };
  uint32_t n = (int32_t)sizeof(modeNames)/sizeof(char const*);
  auto&selected = vars.addOrGetInt32("method.selectedModeId",2);
  ImGui::Begin("vars");
  ImGui::ListBox("polygonMode",&selected,modeNames,n);
  ImGui::End();
  hide(vars,"method.selectedModeId");
  return modes[selected];
}

GLenum selectPolygon(vars::Vars&vars){
  char const* polygonNames[]={
    "GL_POINTS                  ",
    "GL_LINES                   ",
    "GL_LINE_LOOP               ",
    "GL_LINE_STRIP              ",
    "GL_LINES_ADJACENCY         ",
    "GL_LINE_STRIP_ADJACENCY    ",
    "GL_TRIANGLES               ",
    "GL_TRIANGLE_STRIP          ",
    "GL_TRIANGLE_FAN            ",
    "GL_TRIANGLE_STRIP_ADJACENCY",
  };
  GLenum polygons[]={
    GL_POINTS                  ,
    GL_LINES                   ,
    GL_LINE_LOOP               ,
    GL_LINE_STRIP              ,
    GL_LINES_ADJACENCY         ,
    GL_LINE_STRIP_ADJACENCY    ,
    GL_TRIANGLES               ,
    GL_TRIANGLE_STRIP          ,
    GL_TRIANGLE_FAN            ,
    GL_TRIANGLE_STRIP_ADJACENCY,
  };
  auto n = (int32_t)sizeof(polygonNames)/sizeof(char const*);
  auto&selected = vars.addOrGetInt32("method.selectedPolygonId",0);
  ImGui::Begin("vars");
  ImGui::ListBox("primitive",&selected,polygonNames,n);
  ImGui::End();
  hide(vars,"method.selectedPolygonId");
  return polygons[selected];
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw   =              onDraw  ;
  clbs.onInit   =              onInit  ;
  clbs.onQuit   =              onQuit  ;
  clbs.onResize = emptyWindow::onResize;
  MethodRegister::get().manager.registerMethod("pgr01.primitives",clbs);
};


}

