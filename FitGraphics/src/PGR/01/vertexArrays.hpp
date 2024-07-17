#pragma once

#include<geGL/geGL.h>

namespace vertexArrays{
  void setVertexAttrib(
      GLuint   vao   ,
      GLuint   attrib,
      GLint    size  ,
      GLenum   type  ,
      GLuint   buffer,
      GLintptr offset,
      GLsizei  stride);
}
