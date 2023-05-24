#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h> // include glad to get all the required OpenGL headers
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
  

class Shader
{
public:
    // the program ID
    unsigned int ID;
  
    // constructor reads and builds the shader
    Shader(const char* vertexPath, const char* fragmentPath);
    // use/activate the shader
    void use();
    // utility uniform functions
    void setBool(const std::string &name, bool value) const;  
    void setInt(const std::string &name, int value) const;   
    void setFloat(const std::string &name, float value) const;
};

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{

  //////////////////////////////
  // Read shaders into memory
  //////////////////////////////

  std::string vertexCode;
  std::string fragmentCode;
  std::ifstream vShaderFile; //ifstream -> input file stream
  std::ifstream fShaderFile;
  //ensure ifstream objects can throw exceptions (not sure what this is)
  vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
  fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
  try 
  {
    vShaderFile.open(vertexPath);
    fShaderFile.open(fragmentPath);
    std::stringstream vShaderStream, fShaderStream;
    //read file buffers into streams
    vShaderStream << vShaderFile.rdbuf();
    fShaderStream << fShaderFile.rdbuf();
    //close file handlers
    vShaderFile.close();
    fShaderFile.close();
    //convert streams to strings
    vertexCode = vShaderStream.str();
    fragmentCode = fShaderStream.str();
  }
  catch (std::ifstream::failure e)
  {
    std::cout << "ERR::SHADER::FILE_NOT_SUCCESSFULLY_READ" << std::endl;
  }
  const char* vShaderCode =  vertexCode.c_str(); //convert c++ strings to c strings
  const char* fShaderCode = fragmentCode.c_str();

  unsigned int vertex, fragment;
  int success;
  char infoLog[512];
  
  //////////////////////////////
  // Compile Shaders
  //////////////////////////////

  vertex = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex, 1, &vShaderCode, NULL);
  glCompileShader(vertex);
  //errors?
  glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(vertex, 512, NULL, infoLog);
    std::cout << "ERR::SHADER::VERTEX::COMPILATION_FAILED\n" << std::endl;
    std::cout << infoLog << std::endl;
  }

  //compile vertex shader

  fragment = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment, 1, &fShaderCode, NULL);
  glCompileShader(fragment);
  //errors?
  glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
  if (!success)
  {
    glGetShaderInfoLog(fragment, 512, NULL, infoLog);
    std::cout << "ERR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << std::endl;
    std::cout << infoLog << std::endl;
  }

  //////////////////////////////
  // Build Shader Program
  //////////////////////////////
  
  ID = glCreateProgram();
  glAttachShader(ID, vertex);
  glAttachShader(ID, fragment);
  glLinkProgram(ID);
  //errors?
  glGetProgramiv(ID, GL_LINK_STATUS, &success);
  if (!success)
  {
    glGetProgramInfoLog(ID, 512, NULL, infoLog);
    std::cout << "ERR::SHADER::PROGRAM::LINKING_FAILED\n" << std::endl;
  }

  //delete shaders
  glDeleteShader(vertex);
  glDeleteShader(fragment);

}

void Shader::use() {
  glUseProgram(ID);
}

void Shader::setBool(const std::string &name, bool value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}
void Shader::setInt(const std::string &name, int value) const
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}
void Shader::setFloat(const std::string &name, float value) const
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

#endif
