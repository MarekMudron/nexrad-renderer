#include<Vars/Vars.h>
#include<framework/methodRegister.hpp>
#include<SDL.h>

namespace oglFunc{

using GLCLEAR = void(*)(int32_t);
#define GL_COLOR_BUFFER_BIT 0x00004000

using GLCLEARCOLOR = void(*)(float,float,float,float);

GLCLEAR glClear = nullptr;
GLCLEARCOLOR glClearColor = nullptr;

void onInit(vars::Vars&){
  glClear      = (GLCLEAR     )SDL_GL_GetProcAddress("glClear"     );
  glClearColor = (GLCLEARCOLOR)SDL_GL_GetProcAddress("glClearColor");

  glClearColor(0,0,1,1);
}

void onDraw(vars::Vars&){
  glClear(GL_COLOR_BUFFER_BIT);
}

EntryPoint main = [](){
  methodManager::Callbacks clbs;
  clbs.onDraw   = onDraw  ;
  clbs.onInit   = onInit  ;
  MethodRegister::get().manager.registerMethod("pgr01.oglFunc",clbs);
};

}

