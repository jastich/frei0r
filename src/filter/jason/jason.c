#include <stdlib.h>
#include <assert.h>

#include "frei0r.h"

static const unsigned int SIZE = 20;

typedef struct inverter_instance
{
    unsigned int width;
    unsigned int height;
    unsigned int sectionWidth;
    unsigned int sectionHeight;
    unsigned int** map;
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

f0r_instance_t f0r_construct(unsigned int width, unsigned int height)
{
    plugin_instance_t* inst = (plugin_instance_t*)calloc(1, sizeof(*inst));
    inst->width = width; inst->height = height;
    inst->sectionWidth = SIZE; inst->sectionHeight = SIZE;
    unsigned int sectionsX = 
    inst->map = new unsigned int
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
    unsigned int w = inst->width;
    unsigned int h = inst->height;
    unsigned int x,y;
  
    uint32_t* dst = outframe;
    const uint32_t* src = inframe;

    for (x=0; x<w; x+=sectionWidth)
    {
        for (y=0; y<h; y+=sectionHeight)
        {
        }
    }
    

  for(y=0;y<h;++y)
      for(x=0;x<w;++x,++src)
	  *dst++ = 0x00ffffff^(*src); 
}

