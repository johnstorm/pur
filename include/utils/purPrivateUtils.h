
#ifndef _PUR_PRIVATE_UTILS_H_
#define _PUR_PRIVATE_UTILS_H_

#include "inkHeader.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef float			purGLfloat;
typedef unsigned char   purGLubyte;

#pragma mark -
#pragma mark Macros
#pragma mark -

#define PUR_IS_BIT_ENABLED(_var_, _flag_)	(((_var_) & (_flag_)) == (_flag_))
#define PUR_IS_BIT_ENABLED_IN_BOTH(_var1_, _var2_, _flag_)	(PUR_IS_BIT_ENABLED(_var1_, _flag_) && PUR_IS_BIT_ENABLED(_var2_, _flag_))
#define PUR_ENABLE_BIT(_var_, _flag_)		((_var_) |= (_flag_))
#define PUR_DISABLE_BIT(_var_, _flag_)		((_var_) &= (~_flag_))
#define PUR_COLOR_ONE_DIV_255 (0.00392156863f)
#define PUR_COLOR_BYTE_TO_FLOAT(_color_) ((purGLfloat)(_color_)  * PUR_COLOR_ONE_DIV_255)
#define PUR_COLOR_FLOAT_TO_BYTE(_color_) ((purGLubyte)((_color_) * 0xFF))
#define PUR_BOOL_TO_STRING(_var_) ((_var_) ? @"YES" : @"NO")

#pragma mark -
#pragma mark Declerations
#pragma mark -

inkInline void purSwap(int *val0, int *val1);
inkInline void purSwapf(float *val0, float *val1);
inkInline void purSwapv(void *val0, void *val1, size_t size);

inkInline void *purStridedMemcpy(void *dest, void *src, size_t size, size_t len, size_t destStride, size_t srcStride);

#pragma mark -
#pragma mark Implementations
#pragma mark -

inkInline void purSwap(int *val0, int *val1)
{
	*val0 ^= *val1;
	*val1 ^= *val0;
	*val0 ^= *val1;
}

inkInline void purSwapf(float *val0, float *val1)
{
	purSwap((int *)(val0), (int *)(val1));
}

inkInline void purSwapv(void *val0, void *val1, size_t size)
{
//	void *temp = alloca(size);
//
//	temp = memcpy(temp, val0, size);
//	val0 = memcpy(val0, val1, size);
//	val1 = memcpy(val1, temp, size);

	size_t index;
	unsigned char *byte0;
	unsigned char *byte1;
	for (index = 0, byte0 = (unsigned char *)val0, byte1 = (unsigned char *)val1; index < size; ++index, ++byte0, ++byte1)
	{
		*byte0 ^= *byte1;
		*byte1 ^= *byte0;
		*byte0 ^= *byte1;
	}
}

void *purStridedMemcpy(void *dest, void *src, size_t size, size_t len, size_t destStride, size_t srcStride)
{
	if (destStride == srcStride == 0)
	{
		size_t totalBytes = size * len;
		return memcpy(dest, src, totalBytes);
	}

	size_t index;
	unsigned char *destData;
	unsigned char *srcData;

	for (index = 0, destData = (unsigned char *)dest, srcData = (unsigned char *)src; index < len; ++index, destData += destStride, srcData += srcStride)
	{
		memcpy(destData, srcData, size);
	}

	return dest;
}

#ifdef __cplusplus
}
#endif

#endif
