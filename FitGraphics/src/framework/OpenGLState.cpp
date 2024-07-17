#include<vector>
#include<framework/OpenGLState.hpp>
#include<geGL/StaticCalls.h>
using namespace ge::gl;


#define OPENGL_ENABLE_DISABLE_LIST(f)\
  f(GL_BLEND                        );\
  f(GL_CLIP_DISTANCE0               );\
  f(GL_COLOR_LOGIC_OP               );\
  f(GL_CULL_FACE                    );\
  f(GL_DEBUG_OUTPUT                 );\
  f(GL_DEBUG_OUTPUT_SYNCHRONOUS     );\
  f(GL_DEPTH_CLAMP                  );\
  f(GL_DEPTH_TEST                   );\
  f(GL_DITHER                       );\
  f(GL_FRAMEBUFFER_SRGB             );\
  f(GL_LINE_SMOOTH                  );\
  f(GL_MULTISAMPLE                  );\
  f(GL_POLYGON_OFFSET_FILL          );\
  f(GL_POLYGON_OFFSET_LINE          );\
  f(GL_POLYGON_OFFSET_POINT         );\
  f(GL_POLYGON_SMOOTH               );\
  f(GL_PRIMITIVE_RESTART            );\
  f(GL_PRIMITIVE_RESTART_FIXED_INDEX);\
  f(GL_RASTERIZER_DISCARD           );\
  f(GL_SAMPLE_ALPHA_TO_COVERAGE     );\
  f(GL_SAMPLE_ALPHA_TO_ONE          );\
  f(GL_SAMPLE_COVERAGE              );\
  f(GL_SAMPLE_SHADING               );\
  f(GL_SAMPLE_MASK                  );\
  f(GL_SCISSOR_TEST                 );\
  f(GL_STENCIL_TEST                 );\
  f(GL_TEXTURE_CUBE_MAP_SEAMLESS    );\
  f(GL_PROGRAM_POINT_SIZE           );\


#define OPENGL_INT_LIST(f)\
  f(GL_POLYGON_MODE,GL_FILL);\

#define OPENGL_FLOAT_LIST(f)\
  f(GL_LINE_WIDTH,1.0f);\

struct Backup{
#define DECLARE(x)GLboolean v##x = GL_FALSE
  OPENGL_ENABLE_DISABLE_LIST(DECLARE);
#undef DECLARE
#define DECLARE(x,y)GLint v##x = y
  OPENGL_INT_LIST(DECLARE);
#undef DECLARE
#define DECLARE(x,y)GLfloat v##x = y
  OPENGL_FLOAT_LIST(DECLARE);
#undef DECLARE
};


struct OpenGLStateImpl{
  void setLastState();
  void pop();
  void push();
  std::vector<Backup>backups;
};

void OpenGLStateImpl::setLastState(){
  if(backups.empty())return;

  auto const&b=backups.at(backups.size()-1);
#define ENABLE_DISABLE(x)if(b.v##x)ge::gl::glEnable(x);else ge::gl::glDisable(x)
  OPENGL_ENABLE_DISABLE_LIST(ENABLE_DISABLE);
#undef ENABLE_DISABLE


  glPolygonMode(GL_FRONT_AND_BACK,b.vGL_POLYGON_MODE);
  glLineWidth(b.vGL_LINE_WIDTH);
//#define SET_INT(x)GLint v##x = 0
//  OPENGL_INT_LIST(SET_INT);
//#undef DECLARE
}

void OpenGLStateImpl::pop(){
  backups.pop_back();
}
void OpenGLStateImpl::push(){
  Backup b;
#define GET_BOOLEAN(x)ge::gl::glGetBooleanv(x,&b.v##x)
  OPENGL_ENABLE_DISABLE_LIST(GET_BOOLEAN);
#undef  GET_BOOLEAN
  glGetIntegerv(GL_POLYGON_MODE,&b.vGL_POLYGON_MODE);
  glGetFloatv(GL_LINE_WIDTH,&b.vGL_LINE_WIDTH);
 
  backups.push_back(b);
}

OpenGLState::OpenGLState(){
  impl = new OpenGLStateImpl;
}

OpenGLState::~OpenGLState(){
  delete impl;
}

void OpenGLState::setLastState(){
  impl->setLastState();
}

void OpenGLState::pop(){
  impl->pop();
}

void OpenGLState::push(){
  impl->push();
}

