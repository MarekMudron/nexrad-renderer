#pragma once

struct OpenGLStateImpl;
class OpenGLState{
  public:
    OpenGLState();
    ~OpenGLState();
    void setLastState();
    void pop();
    void push();
  private:
    OpenGLStateImpl*impl;
};
