#ifndef _PUR_COLOR_TRANSFORM_H_
#define _PUR_COLOR_TRANSFORM_H_

#include "inkHeader.h"

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
inkExtern purColorTransform purColorTransformIdentity;

inkInline purColorTransform purColorTransformMake(float red, float green, float blue, float alpha);

inkInline purColorTransform purColorTransformMake(float red, float green, float blue, float alpha)
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
