#include<sstream>

#include<glm/glm.hpp>


#include<Vars/Vars.h>
#include<framework/Barrier.h>

#include<geGL/geGL.h>
#include<geGL/StaticCalls.h>
#include<imguiDormon/imgui.h>
#include<imguiVars/addVarsLimits.h>

#include<framework/methodRegister.hpp>
#include<framework/defineGLSLVersion.hpp>
#include<PGR/01/emptyWindow.hpp>

using namespace ge::gl;
using namespace std;

namespace tessellationLevels{

enum Spacing{
  EQUAL_SPACING,
  FRACTIONAL_EVEN_SPACING,
  FRACTIONAL_ODD_SPACING,
};

enum DrawMode{
  FILL,
  LINE,
  POINT,
};

enum Primitive{
  ISOLINE,
  TRIANGLE,
  QUAD,
};

void createProgram(vars::Vars&vars){
  if(notChanged(vars,"method.all",__FUNCTION__,
        {"method.primitive","method.spacing","method.drawMode"}))return;

  auto vertices = (int32_t)vars.getEnum<Primitive>("method.primitive")+2;
  auto spacing  = (int32_t)vars.getEnum<Spacing  >("method.spacing");
  auto drawMode = (int32_t)vars.getEnum<DrawMode >("method.drawMode");
  stringstream vsSrc;
  vsSrc << "#version                     " << 450                << endl;
  vsSrc << "#line                        " << __LINE__           << endl;
  vsSrc << "#define PATCH_VERTICES       " << vertices           << endl;
  vsSrc << R".(

  void main(){
    if(gl_VertexID == 0)gl_Position = vec4(-.5,-.5,0,1);
    if(gl_VertexID == 1)gl_Position = vec4(+.5,-.5,0,1);

    #if PATCH_VERTICES == 3
      if(gl_VertexID == 2)gl_Position = vec4(0,+.5,0,1);
    #else
      if(gl_VertexID == 2)gl_Position = vec4(+.5,+.5,0,1);
    #endif

    if(gl_VertexID == 3)gl_Position = vec4(-.5,+.5,0,1);
  }

  ).";
  auto vert = make_shared<Shader>(GL_VERTEX_SHADER,vsSrc.str());

  stringstream csSrc;
  csSrc << "#version                     " << 450                << endl;
  csSrc << "#line                        " << __LINE__           << endl;
  csSrc << "#define PATCH_VERTICES       " << vertices           << endl;
  csSrc << R".(

  // Number of vertices in output primitive
  // Number of invocation of control shader
  layout(vertices=PATCH_VERTICES)out;
  
  uniform vec2 inner = vec2(1,1);//tessellation inner level
  uniform vec4 outer = vec4(1,1,1,1);//tessellation outer level
  
  void main(){
  	gl_out[gl_InvocationID].gl_Position=gl_in[gl_InvocationID].gl_Position;
  	if(gl_InvocationID==0){
  		gl_TessLevelOuter[0]=outer[0];
  		gl_TessLevelOuter[1]=outer[1];
  		gl_TessLevelOuter[2]=outer[2];
  		gl_TessLevelOuter[3]=outer[3];
  		gl_TessLevelInner[0]=inner[0];
  		gl_TessLevelInner[1]=inner[1];
  	}
  }

  ).";
  auto cont = make_shared<Shader>(GL_TESS_CONTROL_SHADER,csSrc.str());

  std::string const spacingNames[] = {
      "equal_spacing", "fractional_even_spacing", "fractional_odd_spacing"
  };
  std::string const primitiveNames[] = {
    "isolines","triangles","quads"
  };
  stringstream evSrc;
  evSrc << "#version                     " << 450                        << endl;
  evSrc << "#line                        " << __LINE__                   << endl;
  evSrc << "#define PATCH_VERTICES       " << vertices                   << endl;
  evSrc << "#define SPACING              " << spacingNames[spacing]      << endl;
  evSrc << "#define PRIMITIVE            " << primitiveNames[vertices-2] << endl;
  evSrc << R".(

  layout(PRIMITIVE,SPACING)in;
  
  void main(){

  #if PATCH_VERTICES == 2
	  gl_Position=mix(gl_in[0].gl_Position,gl_in[1].gl_Position,gl_TessCoord.x)+vec4(0,gl_TessCoord.y,0,0);
  #elif PATCH_VERTICES == 3
	  gl_Position=
		  gl_in[0].gl_Position*gl_TessCoord.x+
		  gl_in[1].gl_Position*gl_TessCoord.y+
		  gl_in[2].gl_Position*gl_TessCoord.z;
  #else
  	vec4 A=mix(gl_in[0].gl_Position,gl_in[1].gl_Position,gl_TessCoord.x);
  	vec4 B=mix(gl_in[3].gl_Position,gl_in[2].gl_Position,gl_TessCoord.x);
  	gl_Position=mix(A,B,gl_TessCoord.y);
  #endif
  }

  ).";
  auto eval = make_shared<Shader>(GL_TESS_EVALUATION_SHADER,evSrc.str());

  stringstream gsSrc;
  gsSrc << "#version                     " << 450                << endl;
  gsSrc << "#line                        " << __LINE__           << endl;
  gsSrc << "#define PATCH_VERTICES       " << vertices           << endl;
  gsSrc << "#define DRAW_MODE            " << drawMode           << endl;
  gsSrc << R".(

  #if PATCH_VERTICES < 3
  layout(lines)in;
  #else
  layout(triangles)in;
  #endif

  #if DRAW_MODE == 0
  layout(triangle_strip,max_vertices=3)out;
  #endif
  #if DRAW_MODE == 1
  layout(line_strip,max_vertices=4)out;
  #endif
  #if DRAW_MODE == 2
  layout(points,max_vertices=3)out;
  #endif

  void main(){
    #if PATCH_VERTICES > 2
    #if DRAW_MODE == 1
    gl_Position = gl_in[0].gl_Position;EmitVertex();
    gl_Position = gl_in[1].gl_Position;EmitVertex();
    gl_Position = gl_in[2].gl_Position;EmitVertex();
    gl_Position = gl_in[0].gl_Position;EmitVertex();
    #else
    gl_Position = gl_in[0].gl_Position;EmitVertex();
    gl_Position = gl_in[1].gl_Position;EmitVertex();
    gl_Position = gl_in[2].gl_Position;EmitVertex();
    #endif
    #else
    gl_Position = gl_in[0].gl_Position;EmitVertex();
    gl_Position = gl_in[1].gl_Position;EmitVertex();
    #endif
    EndPrimitive();
  }

  ).";
  auto geom = make_shared<Shader>(GL_GEOMETRY_SHADER,gsSrc.str());

  stringstream fsSrc;
  fsSrc << "#version                     " << 450                << endl;
  fsSrc << "#line                        " << __LINE__           << endl;
  fsSrc << R".(
  
  out vec4 fColor;

  void main(){
    fColor = vec4(1);
  }

  ).";
  auto frag = make_shared<Shader>(GL_FRAGMENT_SHADER,fsSrc.str());
  vars.reCreate<Program>("method.program",vert,cont,eval,geom,frag);
}


void onInit(vars::Vars&vars){
  vars.add<ge::gl::VertexArray>("method.emptyVao");
  vars.add<glm::vec2>("method.innerLevel",glm::vec2(1.f));
  vars.add<glm::vec4>("method.outerLevel",glm::vec4(1.f));
  vars.addEnum<Spacing>("method.spacing",EQUAL_SPACING);
  vars.addEnum<DrawMode>("method.drawMode",FILL);
  vars.addEnum<Primitive>("method.primitive",QUAD);

  addVarsLimits2F(vars,"method.innerLevel",1,64,0.1f);
  addVarsLimits4F(vars,"method.outerLevel",1,64,0.1f);

  addEnumValues<Primitive>(vars,{ISOLINE,TRIANGLE,QUAD},{"isoline","triangle","quad"});
  addEnumValues<Spacing>  (vars,{EQUAL_SPACING,FRACTIONAL_EVEN_SPACING,FRACTIONAL_ODD_SPACING},{"equal","fractional_even","fractional_odd"});
  addEnumValues<DrawMode> (vars,{FILL,LINE,POINT},{"fill","line","point"});

}

void onDraw(vars::Vars&vars){
  createProgram(vars);

  auto primitive = (int32_t)vars.getEnum<Primitive  >("method.primitive" );
  auto vao       =          vars.get    <VertexArray>("method.emptyVao"  );
  auto program   =          vars.get    <Program    >("method.program"   );
  auto inner     =  (float*)vars.get    <glm::vec2  >("method.innerLevel");
  auto outer     =  (float*)vars.get    <glm::vec4  >("method.outerLevel");

  glClearColor(0.f,0.1f,0.f,1.f);
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

  vao->bind();
  auto const vertices = primitive+2;
  program
    ->set2fv("inner",inner)
    ->set4fv("outer",outer)
    ->use();
  glPatchParameteri(GL_PATCH_VERTICES,vertices);
  glPointSize(4);
  glDrawArrays(GL_PATCHES,0,vertices);
  vao->unbind();

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
  MethodRegister::get().manager.registerMethod("misc.tessellationLevels",clbs);
};
}

