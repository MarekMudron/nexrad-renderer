#pragma once

#include<map>
#include<iostream>
#include<functional>
#include<Vars/Vars.h>


namespace methodManager{

using Callback = std::function<void(vars::Vars&)>;

struct Callbacks{
  Callback onInit        = nullptr;
  Callback onDraw        = nullptr;
  Callback onQuit        = nullptr;
  Callback onUpdate      = nullptr;
  Callback onResize      = nullptr;
  Callback onKeyDown     = nullptr;
  Callback onKeyUp       = nullptr;
  Callback onMouseMotion = nullptr;
};

class Manager{
  public:
    void registerMethod(
        std::string const&name     ,
        Callbacks   const&callbacks);
    bool empty()const;
    void onInit(vars::Vars&vars);
    void onDraw(vars::Vars&vars);
    void onQuit(vars::Vars&vars);
    void onUpdate(vars::Vars&vars);
    void onResize(vars::Vars&vars);
    void onKeyDown(vars::Vars&vars);
    void onKeyUp(vars::Vars&vars);
    void onMouseMotion(vars::Vars&vars);
    Callbacks const& getCurrentCallbacks()const;
    std::vector<std::string>const&getMethods()const;

    bool methodExists(std::string const&name);
    void setCurrentMethod(vars::Vars&vars,std::string const&name);
    std::string getCurrentMethod()const;

  private:
    std::string                       currentMethod = "";
    std::vector<std::string          >methodNames       ;
    std::map   <std::string,Callbacks>methods           ;
};

}
