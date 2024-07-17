#pragma once

#include<memory>
#include<cstring>
#include<geGL/Program.h>

std::shared_ptr<ge::gl::Program>makeProgram(std::string const&src);
