#pragma once

#include <glad/gl.h>
#include <iostream>
#include <string>

// OpenGL error checking macro
#ifdef NDEBUG
    #define GL_CHECK_ERROR()
#else
    #define GL_CHECK_ERROR() GLDebug::checkError(__FILE__, __LINE__)
#endif

namespace GLDebug {
    inline void checkError(const char* file, int line) {
        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            std::string errorString;
            switch (error) {
                case GL_INVALID_ENUM:      errorString = "GL_INVALID_ENUM"; break;
                case GL_INVALID_VALUE:     errorString = "GL_INVALID_VALUE"; break;
                case GL_INVALID_OPERATION: errorString = "GL_INVALID_OPERATION"; break;
                case GL_OUT_OF_MEMORY:     errorString = "GL_OUT_OF_MEMORY"; break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: errorString = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
                default:                   errorString = "UNKNOWN_ERROR"; break;
            }
            std::cerr << "OpenGL Error: " << errorString << " (" << error << ") at " 
                      << file << ":" << line << std::endl;
        }
    }
}
