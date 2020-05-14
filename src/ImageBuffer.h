//
// class to represent a 3D volume (fbo and associated textures)
//
// Author: Simon Green
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#ifndef _IMAGE_BUFFER_H
#define _IMAGE_BUFFER_H

#include <GL/glew.h>
#include "framebufferObject.h"
//#include "Program.h"

// class to represent a 3D volume (fbo and associated textures)
class ImageBuffer {
public:
    ImageBuffer(GLint format, int width, int height, int banks);
    ~ImageBuffer();

    enum BlendMode { BLEND_NONE = 0, BLEND_ADDITIVE };

    void bind() { m_fbo->Bind(); }
    void unbind() { m_fbo->Disable(); }

    void setData(unsigned char *data, int bank=0);

    void setWrapMode(GLint mode, int bank=0);
    void setFiltering(GLint mode, int bank=0);
    void setBlendMode(BlendMode mode) { m_blendMode = mode; }

    //void runProgram(Program *fprog, int bank=0);

    FramebufferObject *getFBO() { return m_fbo; }
    GLuint getTexture(int bank = 0) { return m_tex[bank]; }

    int getWidth() { return m_width; }
    int getHeight() { return m_height; }

private:
    GLuint create2dTexture(GLint internalformat, int w, int h);
    void draw();

    int m_width, m_height;
    int m_max_banks;
    int m_banks;
    BlendMode m_blendMode;

    FramebufferObject *m_fbo;
    GLuint *m_tex;
};

#endif
