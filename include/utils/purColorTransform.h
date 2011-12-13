#ifndef _PUR_COLOR_TRANSFORM_H_
#define _PUR_COLOR_TRANSFORM_H_

#include "purHeader.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	float redMultiplier;
	float greenMultiplier;
	float blueMultiplier;
	float alphaMultiplier;
} purColorTransform;

#define _purColorTransformIdentity {1.0f, 1.0f, 1.0f, 1.0f}
purExtern purColorTransform purColorTransformIdentity;

purInline purColorTransform purColorTransformMake(float red, float green, float blue, float alpha);

purInline purColorTransform purColorTransformMake(float red, float green, float blue, float alpha)
{
	purColorTransform transform;

	transform.redMultiplier = red;
	transform.greenMultiplier = green;
	transform.blueMultiplier = blue;
	transform.alphaMultiplier = alpha;

	return transform;
}

#ifdef __cplusplus
}
#endif

#endif
