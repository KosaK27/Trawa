#pragma once
#include <GL/glew.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

inline GLuint compileShader(GLenum type, const std::string& path) {
    std::ifstream f(path);
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string src = ss.str();
    const char* c = src.c_str();

    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &c, nullptr);
    glCompileShader(id);

    GLint ok;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(id, 1024, nullptr, log);
        std::cerr << path << "\n" << log << "\n";
    }
    return id;
}

inline GLuint linkProgram(const std::string& vert, const std::string& frag) {
    GLuint v = compileShader(GL_VERTEX_SHADER, vert);
    GLuint f = compileShader(GL_FRAGMENT_SHADER, frag);
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);

    GLint ok;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(p, 1024, nullptr, log);
        std::cerr << vert << "+" << frag << "\n" << log << "\n";
    }
    glDeleteShader(v);
    glDeleteShader(f);
    return p;
}