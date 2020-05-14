//
// class to render a 3D volume
//
// Author: Simon Green
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#include <GL/glew.h>
#include <GL/glut.h>
#include <Cg/cgGL.h>

#include <nvSDKPath.h>

static nv::SDKPath sdkPath;

#include "VolumeRender.h"

VolumeRender::VolumeRender(CGcontext cg_context, VolumeBuffer *volume, ImageBuffer* image)
    : m_cg_context(cg_context),
      m_volume(volume),
	  m_image(image),
      m_density(0.05),
      m_brightness(2.0)
{
    loadPrograms();
}

VolumeRender::~VolumeRender()
{
    cgDestroyProgram(m_raymarch_vprog);
    cgDestroyProgram(m_raymarch_fprog);
}

void
VolumeRender::loadPrograms()
{
    m_cg_vprofile = cgGLGetLatestProfile(CG_GL_VERTEX);
    m_cg_fprofile = cgGLGetLatestProfile(CG_GL_FRAGMENT);

    std::string resolved_path;

    if (sdkPath.getFilePath( "shaders/raymarch.cg", resolved_path)) 
	{
        m_raymarch_vprog = cgCreateProgramFromFile( m_cg_context, CG_SOURCE, resolved_path.c_str(), m_cg_vprofile , "RayMarchVP", 0);
        cgGLLoadProgram(m_raymarch_vprog);

        m_raymarch_fprog = cgCreateProgramFromFile( m_cg_context, CG_SOURCE, resolved_path.c_str(), m_cg_fprofile , "RayMarchFP", 0);
        cgGLLoadProgram(m_raymarch_fprog);

        m_density_param = cgGetNamedParameter(m_raymarch_fprog, "density");
        m_brightness_param = cgGetNamedParameter(m_raymarch_fprog, "brightness");
    }
    else {
        fprintf( stderr, "Failed to find shader file '%s'\n", "shaders/raymarch.cg");
    }
}


// render using ray marching
void
VolumeRender::render()
{
    cgGLBindProgram(m_raymarch_vprog);
    cgGLEnableProfile(m_cg_vprofile);
    cgGLEnableProfile(m_cg_fprofile);

    cgGLBindProgram(m_raymarch_fprog);
    cgGLSetParameter1f(m_density_param, m_density);
    cgGLSetParameter1f(m_brightness_param, m_brightness);

    glActiveTextureARB(GL_TEXTURE0_ARB);
    glBindTexture(GL_TEXTURE_3D, m_volume->getTexture());

	//glActiveTextureARB(GL_TEXTURE0_ARB);
 //   glBindTexture(GL_TEXTURE_2D, m_image->getTexture());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//float step = 1.0f / 4.0f;
	//for(float x = 0; x <= 1.0f; x+=step)
	//	for(unsigned int i = 0; i < 3; i++)
	//	    m_volume -> drawSlice(x, i, step / 2.0f);

    glutSolidCube(1.0);

    cgGLDisableProfile(m_cg_vprofile);
    cgGLDisableProfile(m_cg_fprofile);
}
