#include<framework/methodManager.hpp>
#include<framework/OpenGLState.hpp>

namespace methodManager{

void Manager::registerMethod(
    std::string const&name              ,
    Callbacks   const&callbacks         ){
  methods[name] = callbacks;
  methodNames.emplace_back(name);
  if(methodNames.size() == 1)currentMethod = name;
  auto&c = methods[name];
  
  if(!c.onInit         )c.onInit        = [](vars::Vars&){};
  if(!c.onDraw         )c.onDraw        = [](vars::Vars&){};
  if(!c.onQuit         )c.onQuit        = [](vars::Vars&){};
  if(!c.onUpdate       )c.onUpdate      = [](vars::Vars&){};
  if(!c.onResize       )c.onResize      = [](vars::Vars&){};
  if(!c.onKeyDown      )c.onKeyDown     = [](vars::Vars&){};
  if(!c.onKeyUp        )c.onKeyUp       = [](vars::Vars&){};
  if(!c.onMouseMotion  )c.onMouseMotion = [](vars::Vars&){};
}

bool Manager::empty()const{
  return methodNames.empty();
}

void Manager::onInit(vars::Vars&vars){
  if(empty())return;
  getCurrentCallbacks().onInit(vars);
}

void Manager::onDraw(vars::Vars&vars){
  if(empty())return;
  getCurrentCallbacks().onDraw(vars);
}

void Manager::onQuit(vars::Vars&vars){
  if(empty())return;
  getCurrentCallbacks().onQuit(vars);
}
void Manager::onUpdate(vars::Vars&vars){
  if(empty())return;
  getCurrentCallbacks().onUpdate(vars);
}
void Manager::onResize(vars::Vars&vars){
  if(empty())return;
  getCurrentCallbacks().onResize(vars);
}
void Manager::onKeyDown(vars::Vars&vars){
  if(empty())return;
  getCurrentCallbacks().onKeyDown(vars);
}
void Manager::onKeyUp(vars::Vars&vars){
  if(empty())return;
  getCurrentCallbacks().onKeyUp(vars);
}
void Manager::onMouseMotion(vars::Vars&vars){
  if(empty())return;
  getCurrentCallbacks().onMouseMotion(vars);
}
Callbacks const& Manager::getCurrentCallbacks()const{
  return methods.at(currentMethod);
}
std::vector<std::string>const&Manager::getMethods()const{
  return methodNames;
}

bool Manager::methodExists(std::string const&name){
  for(auto const&x:methodNames)
    if(x==name)
      return true;
  return false;
}

void Manager::setCurrentMethod(vars::Vars&vars,std::string const&name){
  if(currentMethod == name)return;
  if(!methodExists(name))return;
  vars.get<OpenGLState>("oglState")->setLastState();

  onQuit(vars);
  currentMethod = name;
  onInit(vars);
}
std::string Manager::getCurrentMethod()const{
  return currentMethod;
}

}
