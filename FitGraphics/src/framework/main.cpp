#include<tuple>
#include<fstream>

#include<Simple3DApp/Application.h>
#include<Vars/Vars.h>

#include<imguiVars/imguiVars.h>
#include<imguiVars/addVarsLimits.h>
#include<imguiDormon/imgui.h>

#include<geGL/geGL.h>
#include<geGL/StaticCalls.h>

#include<framework/DVars.hpp>
#include<framework/Timer.hpp>

#include<framework/methodManager.hpp>
#include<framework/methodRegister.hpp>
#include<framework/OpenGLState.hpp>

class OGL: public simple3DApp::Application{
 public:
  OGL(int argc, char* argv[]) : Application(argc, argv,460) {
    vars.addInt32("OpenGLVersion",460);
  }
  virtual ~OGL(){}
  virtual void draw() override;
  DVars vars;
  Timer<float> timer;

  virtual void init() override;
  virtual void deinit() override;
  virtual void mouseMove(SDL_Event const& event) override;
  virtual void key(SDL_Event const& e, bool down) override;
  virtual void resize(uint32_t x,uint32_t y) override;
  virtual void mouseWheel(SDL_Event const& event){}
 private:
  //OpenGLState oglState;
  std::map<std::string,std::vector<std::string>>groups;
  std::map<std::string,std::vector<std::string>>getMethodsInGroups();
  void createMenuElements();
  void createMainMenu();
};

std::string loadMethod(){
  std::string name = "storedMethod.txt";

  std::ifstream file (name);
  if(!file.is_open()){
    std::cerr << "cannot open camera file: " << name << std::endl;
    return "";
  }
  std::string res;
  file >> res;
  file.close();
  return res;
}

void saveMethod(std::string const&method){
  std::string name = "storedMethod.txt";
  std::ofstream file (name);
  if(!file.is_open()){
    std::cerr << "cannot open camera file: " << name << std::endl;
    return;
  }
  file << method;
  file.close();
}

void OGL::init(){
  uint32_t size[]={1024,768};
  SDL_SetWindowSize(window->getWindow(),size[0],size[1]);
  SDL_SetWindowPosition(window->getWindow(),SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);
  ge::gl::glViewport(0,0,size[0],size[1]);
  vars.addOrGet<SDL_Window*>("window",window->getWindow());
  vars.addOrGetUint32("event.resizeX") = size[0];
  vars.addOrGetUint32("event.resizeY") = size[1];
  vars.addOrGetInt32 ("event.mouse.x"     ) = 0; 
  vars.addOrGetInt32 ("event.mouse.y"     ) = 0; 
  vars.addOrGetInt32 ("event.mouse.xrel"  ) = 0; 
  vars.addOrGetInt32 ("event.mouse.yrel"  ) = 0; 
  vars.addOrGetBool  ("event.mouse.left"  ) = 0; 
  vars.addOrGetBool  ("event.mouse.right" ) = 0; 
  vars.addOrGetBool  ("event.mouse.middle") = 0;
  auto ogl = vars.add<OpenGLState>("oglState");
  ogl->push();
  MethodRegister::get().manager.onInit(vars);
  MethodRegister::get().manager.setCurrentMethod(vars,loadMethod());
  groups = getMethodsInGroups();
  timer.reset();
}

void OGL::resize(uint32_t x,uint32_t y){
  vars.addOrGetUint32("event.resizeX") = x;
  vars.addOrGetUint32("event.resizeY") = y;
  vars.updateTicks("event.resizeX");
  vars.updateTicks("event.resizeY");
  MethodRegister::get().manager.onResize(vars);
}

std::tuple<std::string,std::string> split(std::string const&str,std::string splitter){
  auto w = str.find(splitter);
  if(w==std::string::npos)return std::make_tuple("",str);
  return std::make_tuple(
    str.substr(0                  ,w),
    str.substr(w+splitter.length()  )
    );
}
std::map<std::string,std::vector<std::string>>OGL::getMethodsInGroups(){
  auto      &manager = MethodRegister::get().manager;
  auto const&methods = manager.getMethods();

  std::map<std::string,std::vector<std::string>>groups;
  for(auto const&m:methods){
    auto ss = split(m,".");
    auto group = std::get<0>(ss);
    auto name  = std::get<1>(ss);
    auto it = groups.find(group);
    if(it == std::end(groups))
      groups[group];
    groups[group].push_back(name);
  }
  return groups;
}

void OGL::createMenuElements(){
  auto&manager = MethodRegister::get().manager;
  for(auto const&g:groups){
    auto groupName = g.first;
    auto finalGroupName = groupName==""?"uncategorized":groupName;
    if(ImGui::BeginMenu(finalGroupName.c_str())){
      for(auto const&m:g.second){

        if(ImGui::MenuItem(m.c_str())){
          auto fullName = (groupName==""?"":groupName+".")+m;
          manager.setCurrentMethod(vars,fullName);
          SDL_SetWindowTitle(window->getWindow(),fullName.c_str());
        }
      }
      ImGui::EndMenu();
    }
  }
  if(ImGui::BeginMenu("A")){
    if(ImGui::BeginMenu("B")){
      if(ImGui::MenuItem("C")){
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenu();
  }
}

void OGL::createMainMenu(){
  if(!ImGui::BeginMainMenuBar())return;
  createMenuElements();
  ImGui::EndMainMenuBar();
}

void OGL::draw(){
  auto&manager = MethodRegister::get().manager;
  vars.addOrGetFloat("event.dt") = timer.elapsedFromLast();
  vars.updateTicks("event.dt");

  createMainMenu();

  manager.onUpdate(vars);
  manager.onDraw(vars);
  drawImguiVars(vars);

  swap();
}

void OGL::key(SDL_Event const&e,bool down){
  vars.addOrGetInt32("event.key") = e.key.keysym.sym;
  vars.updateTicks("event.key");
  if(down)
    MethodRegister::get().manager.onKeyDown(vars);
  else
    MethodRegister::get().manager.onKeyUp(vars);
}

void OGL::mouseMove(SDL_Event const& event){
  vars.addOrGetInt32("event.mouse.x"     ) = event.motion.x                       ;
  vars.addOrGetInt32("event.mouse.y"     ) = event.motion.y                       ;
  vars.addOrGetInt32("event.mouse.xrel"  ) = event.motion.xrel                    ;
  vars.addOrGetInt32("event.mouse.yrel"  ) = event.motion.yrel                    ;
  vars.addOrGetBool ("event.mouse.left"  ) = event.motion.state & SDL_BUTTON_LMASK;
  vars.addOrGetBool ("event.mouse.right" ) = event.motion.state & SDL_BUTTON_RMASK;
  vars.addOrGetBool ("event.mouse.middle") = event.motion.state & SDL_BUTTON_MMASK;
  vars.updateTicks("event.mouse.x"     );
  vars.updateTicks("event.mouse.y"     );
  vars.updateTicks("event.mouse.xrel"  );
  vars.updateTicks("event.mouse.yrel"  );
  vars.updateTicks("event.mouse.left"  );
  vars.updateTicks("event.mouse.right" );
  vars.updateTicks("event.mouse.middle");
  MethodRegister::get().manager.onMouseMotion(vars);
}

void OGL::deinit(){
  saveMethod(MethodRegister::get().manager.getCurrentMethod());
  MethodRegister::get().manager.onQuit(vars);
}

int main(int argc,char*argv[]){
  OGL app{argc, argv};
  app.start();
  return EXIT_SUCCESS;
}
