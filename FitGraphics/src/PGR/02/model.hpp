#pragma once

#include<Vars/Fwd.h>

namespace model{
  void onMouseMotion(vars::Vars&vars);
  void computeProjectionMatrix(vars::Vars&vars);
  void setUpCamera(vars::Vars&vars);
}
