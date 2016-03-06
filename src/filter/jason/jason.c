#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "frei0r.h"

typedef unsigned int uint;

static const uint SIZE = 5;

typedef struct inverter_instance
{
    uint width;
    uint height;
    uint sectionWidth;
    uint sectionHeight;
    uint numSectionsX;
    uint numSectionsY;
} plugin_instance_t;

int f0r_init()
{
    return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* inverterInfo)
{
    inverterInfo->name = "Jason";
    inverterInfo->author = "Jason Stich";
    inverterInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    inverterInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
    inverterInfo->frei0r_version = FREI0R_MAJOR_VERSION;
    inverterInfo->major_version = 0; 
    inverterInfo->minor_version = 9; 
    inverterInfo->num_params =  0; 
    inverterInfo->explanation = "Does whatever the fuck I want it to";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
    /* no params */
}

f0r_instance_t f0r_construct(uint width, uint height)
{
    plugin_instance_t* inst = (plugin_instance_t*)calloc(1, sizeof(*inst));
    inst->width = width; inst->height = height;
    inst->sectionWidth = SIZE; inst->sectionHeight = SIZE;
    inst->numSectionsX = inst->width / inst->sectionWidth;
    inst->numSectionsY = inst->height / inst->sectionHeight;
    return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
    free(instance);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{ /* no params */ }

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{ /* no params */ }

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
    assert(instance);
    plugin_instance_t* inst = (plugin_instance_t*)instance;
    uint w = inst->width;
    uint h = inst->height;
    uint sw = inst->sectionWidth;
    uint sh = inst->sectionHeight;
    uint nsx = inst->numSectionsX;
    uint nsy = inst->numSectionsY;
    uint sx, sy, i, j;
  
    uint32_t* dst = outframe;
    const uint32_t* src = inframe;

    for (sx=0; sx<nsx; sx++)
    {
        for (sy=0; sy<nsy; sy++)
        {
            // ARGB color model
            #define R(v) ((v >> 16) & 0xFF)
            #define G(v) ((v >> 8)  & 0xFF)
            #define B(v) (v & 0xFF)
            #define RGB(r, g, b) (r << 16) | (g << 8) | b
            #define PIXEL_AT(b, x, y) &b[(y) * inst->width + (x)]
            uint32_t x0 = sx*sw;
            uint32_t y0 = sy*sh;
            uint32_t avg = *PIXEL_AT(inframe, x0, y0);
            uint32_t avgR = R(avg);
            uint32_t avgG = G(avg);
            uint32_t avgB = B(avg);
            uint32_t n = 1;
            for (i=0; i<sw; i++)
            {
                for (j=0; j<sh; j++)
                {
                    uint32_t px = *PIXEL_AT(inframe, x0+i, y0+j);
                    avgR = (avgR * n + R(px)) / (n + 1);
                    avgG = (avgG * n + G(px)) / (n + 1);
                    avgB = (avgB * n + B(px)) / (n + 1);
                    n++;
                }
            }
            for (i=0; i<sw; i++)
            {
                for (j=0; j<sh; j++)
                {
                    // Iterate each row in the section and memset to average
                    *PIXEL_AT(outframe, x0+i, y0+j) = RGB(avgR, avgG, avgB);
                }   
            }
        }
    }
}

