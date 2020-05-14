//
// class to represent a 3D volume (fbo and associated textures)
//
// Author: Simon Green
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <GL/glew.h>
#include <Cg/cgGL.h>

#include "VolumeBuffer.h"

VolumeBuffer::VolumeBuffer(GLint format, int width, int height, int depth, int banks)
    : m_width(width),
      m_height(height),
      m_depth(depth),
      m_banks(banks),
      m_blendMode(BLEND_NONE)
{
    // create fbo
    m_fbo = new FramebufferObject();

    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS_EXT, &m_max_banks);
    if (m_banks > m_max_banks) m_banks = m_max_banks;

    // create textures
    m_tex = new GLuint [m_banks];
    for(int i=0; i<m_banks; i++) {
        m_tex[i] = create3dTexture(format, m_width, m_height, m_depth);
    }

    // attach slice 0 of first texture to fbo for starters
    m_fbo->Bind();
    m_fbo->AttachTexture(GL_TEXTURE_3D, m_tex[0], GL_COLOR_ATTACHMENT0_EXT, 0, 0);
    m_fbo->IsValid();
    m_fbo->Disable();
}

VolumeBuffer::~VolumeBuffer()
{
    delete m_fbo;
    for(int i=0; i<m_banks; i++) {
        glDeleteTextures(1, &m_tex[i]);
    }
    delete [] m_tex;
}

GLuint
VolumeBuffer::create3dTexture(GLint internalformat, int w, int h, int d)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    GLint mode = GL_CLAMP_TO_BORDER;
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, mode);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, mode);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, mode);
    glTexImage3D(GL_TEXTURE_3D, 0, internalformat, w, h, d, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    return tex;
}

void
VolumeBuffer::setWrapMode(GLint mode, int bank)
{
    glBindTexture(GL_TEXTURE_3D, m_tex[bank]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, mode);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, mode);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, mode);
}

void
VolumeBuffer::setFiltering(GLint mode, int bank)
{
    glBindTexture(GL_TEXTURE_3D, m_tex[bank]);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, mode);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, mode);
}

void
VolumeBuffer::setData(unsigned char *data, int bank)
{
    glBindTexture(GL_TEXTURE_3D, m_tex[bank]);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, m_width, m_height, m_depth, GL_RGBA, GL_UNSIGNED_BYTE, data);
}


// draw a slice of the volume
void
VolumeBuffer::drawSlice(float x, unsigned int p, float h)
{
	if( p == 0 ) // x
	{
		glBegin(GL_QUADS);
		glTexCoord3f(x + h, 0.0f, 0.0f); glVertex3f(x - 0.5f, -0.5f, -0.5f);
		glTexCoord3f(x + h, 0.0f, 1.0f); glVertex3f(x - 0.5f, 0.5f, -0.5f);
		glTexCoord3f(x + h, 1.0f, 1.0f); glVertex3f(x - 0.5f, 0.5f, 0.5f);
		glTexCoord3f(x + h, 1.0f, 0.0f); glVertex3f(x - 0.5f, -0.5f, 0.5f);
		glEnd();
	}
	else if( p == 1 ) // z
	{
		glBegin(GL_QUADS);
		glTexCoord3f(0.0f, 0.0f, x + h); glVertex3f(-0.5f, x - 0.5f, -0.5f);
		glTexCoord3f(1.0f, 0.0f, x + h); glVertex3f(0.5f, x - 0.5f, -0.5f);
		glTexCoord3f(1.0f, 1.0f, x + h); glVertex3f(0.5f, x - 0.5f, 0.5f);
		glTexCoord3f(0.0f, 1.0f, x + h); glVertex3f(-0.5f, x - 0.5f, 0.5f);
		glEnd();
	}
	else if( p == 2 ) // y
	{
		glBegin(GL_QUADS);
		glTexCoord3f(0.0f, x + h, 0.0f); glVertex3f(-0.5f, -0.5f, x - 0.5f);
		glTexCoord3f(1.0f, x + h, 0.0f); glVertex3f(0.5f, -0.5f, x - 0.5f);
		glTexCoord3f(1.0f, x + h, 1.0f); glVertex3f(0.5f, 0.5f, x - 0.5f);
		glTexCoord3f(0.0f, x + h, 1.0f); glVertex3f(-0.5f, 0.5f, x - 0.5f);
		glEnd();
	}
}