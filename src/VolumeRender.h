//
// class to render a 3D volume
//
// Author: Simon Green
// Email: sdkfeedback@nvidia.com
//
// Copyright (c) NVIDIA Corporation. All rights reserved.
////////////////////////////////////////////////////////////////////////////////

#include <Cg/cgGL.h>
#include "VolumeBuffer.h"
#include "ImageBuffer.h"

// class to render a 3D volume
class VolumeRender  {
public:
    VolumeRender(CGcontext cg_context, VolumeBuffer *volume, ImageBuffer* image);
    ~VolumeRender();

    void render();

    void setVolume(VolumeBuffer *volume) { m_volume = volume; }

    void setDensity(float x) { m_density = x; }
    void setBrightness(float x) { m_brightness = x; }

private:
    void loadPrograms();

    VolumeBuffer *m_volume;
	ImageBuffer *m_image;

    CGcontext m_cg_context;
    CGprofile m_cg_vprofile, m_cg_fprofile;

    CGprogram m_raymarch_vprog, m_raymarch_fprog;
    CGparameter m_density_param, m_brightness_param;

    float m_density, m_brightness;
};