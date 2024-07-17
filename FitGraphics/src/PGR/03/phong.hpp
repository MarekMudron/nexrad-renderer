#pragma once

#include<Vars/Fwd.h>
#include<geGL/geGL.h>

namespace phong{
  void drawBunny(vars::Vars&);
  void drawScene(vars::Vars&);
  void setScene(vars::Vars&vars    ,
      std::string const&vao        ,
      std::string const&prg        ,
      GLenum            primitive  ,
      uint32_t          nofVertices,
      bool              hasIndices );

  std::string const extern phongLightingShader;
}
