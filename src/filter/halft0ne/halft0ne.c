#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "frei0r.h"

// ARGB color model
#define R(v) (((v) >> 16) & 0xFF)
#define G(v) (((v) >> 8)  & 0xFF)
#define B(v) ((v) & 0xFF)
#define RGB(r, g, b) ((r) << 16) | ((g) << 8) | (b)
#define PIXEL_AT(b, x, y, w) &b[(y) * (w) + (x)]
#define X(ptr, start, w) (int32_t)((ptr - start) % w)
#define Y(ptr, start, w) (int32_t)((ptr - start) / w)

typedef unsigned int BOOL;
static const BOOL BTRUE     = 1;
static const BOOL BFALSE    = 0;

typedef unsigned int uint;

typedef struct
{
    uint x;
    uint y;
    uint radius;
} dot_info;

static const uint SIZE              = 30;
static const float NUM_COLORS       = 255;
static const uint NUM_BITS_RED      = 3;
static const uint NUM_BITS_GREEN    = 3;
static const uint NUM_BITS_BLUE     = 2;
static const uint MAX_DOT_RADIUS    = 15;

static const uint32_t STATUS_NONE       = 0;
static const uint32_t STATUS_ON_STACK   = 1;
static const uint32_t STATUS_LONELY     = 2;
static const uint32_t STATUS_SOURCE     = 3;
static const uint32_t STATUS_GROUPED    = 4;

typedef struct group_node
{
    struct group_node* pNext;
    struct group_node* pPrev;
    uint32_t status;
} group_node;

typedef struct
{
    uint64_t* pValues;
    uint64_t* pLastValue;
    uint32_t capacity;
} stack_t;

typedef struct
{
    uint width;
    uint height;
    uint sectionWidth;
    uint sectionHeight;
    uint numSectionsX;
    uint numSectionsY;
    stack_t groupStack;
    group_node* pGroups;
} plugin_instance_t;

void clear(stack_t* pStack)
{
    pStack->pLastValue = NULL;
}

void init(stack_t* pStack, uint32_t capacity)
{
    pStack->pValues = calloc(capacity, sizeof(uint64_t));
    pStack->capacity = capacity;
    clear(pStack);
}

int isEmpty(stack_t* pStack)
{
    return pStack->pLastValue == NULL;
}

void push(stack_t* pStack, uint64_t value)
{
    if (NULL == pStack->pLastValue)
    {
        pStack->pValues[0] = value;
        pStack->pLastValue = pStack->pValues;
    }
    else if ((pStack->pLastValue + 1) < (pStack->pValues + pStack->capacity))
    {
        pStack->pLastValue++;
        *pStack->pLastValue = value;
    }
    //printf( "Pushed %p, size=%d\n",
    //        (void*)value, pStack->pLastValue-pStack->pValues+1 );
}

uint64_t pop(stack_t* pStack)
{
    uint64_t retval = *pStack->pLastValue--;
    if (pStack->pLastValue < pStack->pValues)
    {
        pStack->pLastValue = NULL;
    }
    //printf("Popped %p, %d items remaining on stack\n", (void*)retval,
    //       NULL != pStack->pLastValue ? pStack->pLastValue - pStack->pValues : 0);
    return retval;
}

int f0r_init()
{
    printf("Init\n");
    return 1;
}

void f0r_deinit()
{ /* no initialization required */ }

void f0r_get_plugin_info(f0r_plugin_info_t* inverterInfo)
{
    inverterInfo->name = "halft0ne";
    inverterInfo->author = "Jason Stich";
    inverterInfo->plugin_type = F0R_PLUGIN_TYPE_FILTER;
    inverterInfo->color_model = F0R_COLOR_MODEL_RGBA8888;
    inverterInfo->major_version = 0; 
    inverterInfo->minor_version = 9; 
    inverterInfo->num_params =  0; 
    inverterInfo->explanation =
        "Creates a halftone effect for your viewing pleasure.";
}

void f0r_get_param_info(f0r_param_info_t* info, int param_index)
{
    /* no params */
}

f0r_instance_t f0r_construct(uint width, uint height)
{
    plugin_instance_t* inst = (plugin_instance_t*)calloc(1, sizeof(*inst));
    inst->width = width; inst->height = height;
    printf( "%dx%d\n", width, height );
    inst->sectionWidth = SIZE; inst->sectionHeight = SIZE;
    inst->numSectionsX = inst->width / inst->sectionWidth;
    inst->numSectionsY = inst->height / inst->sectionHeight;
    init(&inst->groupStack, width*height);
    inst->pGroups = (group_node*)calloc(width*height, sizeof(group_node));
    return (f0r_instance_t)inst;
}

void f0r_destruct(f0r_instance_t instance)
{
    plugin_instance_t* pInst = (plugin_instance_t*)instance;
    free(pInst->groupStack.pValues);
    free(pInst->pGroups);
    free(pInst);
}

void f0r_set_param_value(f0r_instance_t instance, 
			 f0r_param_t param, int param_index)
{ /* no params */ }

void f0r_get_param_value(f0r_instance_t instance,
			 f0r_param_t param, int param_index)
{ /* no params */ }

void rgb_to_cmyk(uint32_t* rgb, float* cmyk)
{
    static const uint NUM_COLORS_RGB = 3;
    static const uint R = 0;
    static const uint G = 1;
    static const uint B = 2;
    static const uint C = 0;
    static const uint M = 1;
    static const uint Y = 2;
    static const uint K = 3;
    
    float rgbPrime[NUM_COLORS_RGB];
    uint indexMax = 0, i;
    for (i=0; i<NUM_COLORS_RGB; i++)
    {
        rgbPrime[i] = (float)rgb[i] / 255.0;
        if (rgbPrime[i] > rgbPrime[indexMax])
        {
            indexMax = i;
        }
    }
    cmyk[K] = 1.0 - rgbPrime[indexMax];
    cmyk[C] = (1.0-rgbPrime[R]-cmyk[K]) / (1.0-cmyk[K]);
    cmyk[M] = (1.0-rgbPrime[G]-cmyk[K]) / (1.0-cmyk[K]);
    cmyk[Y] = (1.0-rgbPrime[B]-cmyk[K]) / (1.0-cmyk[K]);
}

/// Shifts color of pixel so that there are at most N colors in entire image.
uint32_t rebase(uint32_t value, uint32_t oldBase, uint32_t newBase)
{
    uint32_t sectorWidth = oldBase / newBase;
    uint32_t remainder = value % sectorWidth;
    uint32_t retval = value - remainder;
    if (remainder > sectorWidth / 2)
    {
        retval += remainder + sectorWidth;
    }
    return retval;
}
 
/// Shifts color of each pixel so that there are at most N colors in entire image.
void flatten(plugin_instance_t* pInst, const uint32_t* pInBuffer, uint32_t* pOutBuffer)
{
    uint i, j;

    for (i=0; i<pInst->width; i++)
    {
        for (j=0; j<pInst->height; j++)
        {
            static const uint NUM_COLORS = 7;
            const uint32_t* px = PIXEL_AT(pInBuffer, i, j, pInst->width);
            uint32_t* pxOut = PIXEL_AT(pOutBuffer, i, j, pInst->width);
            uint32_t oldBase = 0xFF;
            uint32_t r = rebase(R(*px), oldBase, NUM_COLORS);
            uint32_t g = rebase(G(*px), oldBase, NUM_COLORS);
            uint32_t b = rebase(B(*px), oldBase, NUM_COLORS);
            *pxOut = RGB(r, g, b);
        }
    }
}

static const int RECURSION_LIMIT = 1000;

BOOL searchPx(
    plugin_instance_t* pInst, uint32_t* pFrame,
    int32_t x, int32_t y, uint32_t color,
    group_node** ppLast, int stackDepth);

BOOL searchAround(
    plugin_instance_t* pInst, uint32_t* pFrame,
    int32_t x, int32_t y, uint32_t color,
    group_node** ppLast, int stackDepth)
{
    group_node* pCurr = PIXEL_AT(pInst->pGroups, x, y, pInst->width);
    //printf("Searching around x=%d, y=%d\n, pPrev=%p, pNext=%p\n",
    //       x, y, pCurr->pPrev, pCurr->pNext);

    BOOL retval = BFALSE;
    // Search each pixel above, below, left, and right of it.
#if 0
    if (x - 1 >= 0)
    {
        retval = searchPx(
            pInst, pFrame, x-1, y, color, ppLast, stackDepth+1);
    }
    if (y - 1 >= 0)
    {
        retval |= searchPx(
            pInst, pFrame, x, y-1, color, ppLast, stackDepth+1);
    }
#endif
    if (x + 1 < pInst->width)
    {
        retval |= searchPx(
            pInst, pFrame, x+1, y, color, ppLast, stackDepth+1);
    }
    if (y + 1 < pInst->height)
    {
        retval |= searchPx(
            pInst, pFrame, x, y+1, color, ppLast, stackDepth+1);
    }
    return retval;
}

/// @returns BTRUE if stack depth limit hit
BOOL searchPx(
    plugin_instance_t* pInst, uint32_t* pFrame,
    int32_t x, int32_t y, uint32_t color,
    group_node** ppLast, int stackDepth)
{
    BOOL retval = BFALSE;

    uint32_t* pCurrPx = PIXEL_AT(pFrame, x, y, pInst->width);
    group_node* pCurr = pInst->pGroups + (pCurrPx-pFrame);
    //printf( "x=%d y=%d prev=%p next=%p px=0x%x color=0x%x\n",
    //        x, y, pCurr->pPrev, pCurr->pNext, *pCurrPx,
    //        color );
    if (stackDepth < RECURSION_LIMIT && color == *pCurrPx)
    {
        // If the pixel hasn't already been grouped...
        if ((pCurr->status == STATUS_NONE) ||
            (pCurr->status == STATUS_ON_STACK))
        {
            // Join it to the group of the previous pixel.
            pCurr->status = STATUS_GROUPED;
            (*ppLast)->status = STATUS_GROUPED;
            (*ppLast)->pNext = pCurr;
            pCurr->pPrev = *ppLast;
            *ppLast = pCurr;
            retval = searchAround(
                pInst, pFrame, x, y, color, ppLast, stackDepth);
        }
    }
    else if ( pCurr->status == STATUS_NONE )
    {
        // Push pixel to stack if it isn't a member of a group, isn't already
        // on the stack, and doesn't match the current group.
        push(&pInst->groupStack, (uint64_t)pCurr);
        // Set flag to indicate that group is on stack.
        pCurr->status = STATUS_ON_STACK;
    }

    return (retval == BFALSE && stackDepth < RECURSION_LIMIT) ? BFALSE : BTRUE;
}

void f0r_update(f0r_instance_t instance, double time,
		const uint32_t* inframe, uint32_t* outframe)
{
    assert(instance);
    plugin_instance_t* pInst = (plugin_instance_t*)instance;
    memset(pInst->pGroups, 0, sizeof(group_node)*pInst->width*pInst->height);
    clear(&(pInst->groupStack));

    // Reduce number of colors in the image.
    flatten(pInst, inframe, outframe);

    // Push one pixel to the stack to get the search started.  Pixels will be
    // pushed and popped from the stack until all pixels are processed.
    push(&pInst->groupStack, (uint64_t)pInst->pGroups);
    // Flag the pixel for being on the stack.
    pInst->pGroups->status = STATUS_ON_STACK;

    while (!isEmpty(&pInst->groupStack))
    {
        // Pop next pixel off stack.  This pointer on the stack will be used
        // to point to the last group_node in the linked list as the group is
        // formed.
        group_node* pLast = (group_node*)pop(&pInst->groupStack);
        // Only process this pixel if it hasn't already been grouped by a
        // previous search path.
        if ( pLast->status == STATUS_ON_STACK )
        {
            // Clear "on stack" flag now that it has been popped.
            pLast->status = STATUS_SOURCE;
            group_node* pSavedLast = pLast;
            int32_t x = X(pLast, pInst->pGroups, pInst->width); 
            int32_t y = Y(pLast, pInst->pGroups, pInst->width);
            printf("Popped x=%d, y=%d\n", x, y);
            uint32_t color = *(outframe + (pLast-pInst->pGroups));
            searchAround(pInst, outframe, x, y, color, &pLast, 0);
            if ( pSavedLast->status == STATUS_SOURCE )
            {
                pSavedLast->status = STATUS_LONELY;
            }
        }
    }

    group_node** heads = (group_node**)calloc(
        pInst->height * pInst->width, sizeof(group_node*));
    int len = pInst->width*pInst->height;
    int i,j;
    int numHeads = 0;
    for (i=0; i<len; i++)
    {
        group_node* pHead = pInst->pGroups + i;
        // Find the head of this group
        while (pHead->pPrev != NULL)
        {
            pHead = pHead->pPrev;
        }
        // If the head is already in the set, carry on.
        // Otherwise add it to the set and increment the number of heads.
        for (j=0; j<pInst->width*pInst->height; j++)
        {
            if (heads[j] == pHead)
            {
                // already in set
                break;
            }
            else if (heads[j] == NULL)
            {
                heads[j] = pHead;
                numHeads++;
            }
        }
    }
    free(heads);
    printf("Frame has %d groups!", numHeads);

}

