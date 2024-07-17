#include <framework/methodRegister.hpp>

#include<iostream>

MethodRegister&MethodRegister::get(){
  if(!reg)reg = new MethodRegister();
  return *reg;
}

MethodRegister::MethodRegister(){}
MethodRegister::~MethodRegister(){
  delete reg;
}

MethodRegister*MethodRegister::reg = nullptr;
