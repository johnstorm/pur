#include "purGLUtils.h"

#include <limits.h>

#define purGLAABBResetMin INT_MAX
#define purGLAABBResetMax INT_MIN

#define purGLAABBfResetMin FLT_MAX
#define purGLAABBfResetMax (-FLT_MAX)

const purGLAABB purGLAABBReset = {purGLAABBResetMin, purGLAABBResetMin, purGLAABBResetMax, purGLAABBResetMax};
const purGLAABBf purGLAABBfReset = {purGLAABBfResetMin, purGLAABBfResetMin, purGLAABBfResetMax, purGLAABBfResetMax};

purGLVertex purGLVertexMake(GLfloat x, GLfloat y)
{
	purGLVertex retVal;

	retVal.x = x;
	retVal.y = y;

	return retVal;
}

purGLColorVertex purGLColorVertexMake(GLfloat x, GLfloat y, GLubyte r, GLubyte g, GLubyte b, GLubyte a)
{
	purGLColorVertex retVal;

	retVal.x = x;
	retVal.y = y;
	retVal.r = r;
	retVal.g = g;
	retVal.b = b;
	retVal.a = a;

	return retVal;
}

purGLTextureVertex purGLTextureVertexMake(GLfloat x, GLfloat y, GLfloat s, GLfloat t)
{
	purGLTextureVertex retVal;

	retVal.x = x;
	retVal.y = y;
	retVal.s = s;
	retVal.t = t;

	return retVal;
}
purGLColoredTextureVertex purGLColoredTextureVertexMake(GLfloat x, GLfloat y,
																  GLubyte r, GLubyte g, GLubyte b, GLubyte a,
																  GLfloat s, GLfloat t)
{
	purGLColoredTextureVertex retVal;

	retVal.x = x;
	retVal.y = y;
	retVal.r = r;
	retVal.g = g;
	retVal.b = b;
	retVal.a = a;
	retVal.s = s;
	retVal.t = t;

	return retVal;
}

purGLAABB purGLAABBMake(GLint xMin,
							 GLint yMin,
							 GLint xMax,
							 GLint yMax)
{
	purGLAABB retVal;

	retVal.xMin = xMin;
	retVal.yMin = yMin;
	retVal.xMax = xMax;
	retVal.yMax = yMax;

	return retVal;
}

purGLAABBf purGLAABBfMake(GLfloat xMin,
							   GLfloat yMin,
							   GLfloat xMax,
							   GLfloat yMax)
{
	purGLAABBf retVal;

	retVal.xMin = xMin;
	retVal.yMin = yMin;
	retVal.xMax = xMax;
	retVal.yMax = yMax;

	return retVal;
}

purGLColorVerticesRef purGLColorVerticesRefMake(unsigned vertexCount,
													 unsigned char red,
													 unsigned char green,
													 unsigned char blue,
													 unsigned char alpha)
{
	purGLColorVerticesRef ref = calloc(1, sizeof(purGLColorVertices));

	if (ref)
	{
		if (vertexCount > 0)
		{
			ref->vertices = calloc(vertexCount, sizeof(purGLVertex));
			if (ref->vertices)
				ref->vertexCount = vertexCount;
		}
		// Else is taken care of by calloc...

		ref->r = red;
		ref->g = green;
		ref->b = blue;
		ref->a = alpha;
	}

	return ref;
}
void purGLColorVerticesRefFree(purGLColorVertices * ref)
{
	if (ref)
	{
		if (ref->vertices)
		{
			free(ref->vertices);
			ref->vertices = 0;
			ref->vertexCount = 0;
		}

		free(ref);
	}
}

purGLColorVertices purGLColorVerticesMake(unsigned vertexCount,
											   unsigned char red,
											   unsigned char green,
											   unsigned char blue,
											   unsigned char alpha)
{
	purGLColorVertices ref;

	if (vertexCount > 0)
	{
		ref.vertices = calloc(vertexCount, sizeof(purGLVertex));
		if (ref.vertices)
			ref.vertexCount = vertexCount;
	}
	else
	{
		ref.vertices = 0;
		ref.vertexCount = 0;
	}

	ref.r = red;
	ref.g = green;
	ref.b = blue;
	ref.a = alpha;

	return ref;
}

void purGLColorVerticesFree(purGLColorVertices *colorVertices)
{
	if (colorVertices)
	{
		if (colorVertices->vertices)
		{
			free(colorVertices->vertices);
			colorVertices->vertices = 0;
		}

		colorVertices->vertexCount = 0;
	}
}

/*purGLAABB purGLAABBReset()
{
	return purGLAABBMake(purGLAABBResetMin, purGLAABBResetMin, purGLAABBResetMax, purGLAABBResetMax);
}*/
void purGLAABBUpdate(purGLAABB *toBeUpdated, purGLAABB *checkVals)
{
	*toBeUpdated = purGLAABBMake(fminf(toBeUpdated->xMin, checkVals->xMin),
								fminf(toBeUpdated->yMin, checkVals->yMin),
								fmaxf(toBeUpdated->xMax, checkVals->xMax),
								fmaxf(toBeUpdated->yMax, checkVals->yMax));
}

void purGLAABBExpand(purGLAABB *aabb, inkPoint point)
{
	return purGLAABBExpandv(aabb, roundf(point.x), roundf(point.y));
}

void purGLAABBExpandv(purGLAABB *aabb, GLint x, GLint y)
{
	*aabb = purGLAABBMake(fminf(aabb->xMin, x),
						 fminf(aabb->yMin, y),
						 fmaxf(aabb->xMax, x),
						 fmaxf(aabb->yMax, y));
}

void purGLAABBInflate(purGLAABB *aabb, inkPoint point)
{
	return purGLAABBInflatev(aabb, roundf(point.x), roundf(point.y));
}

void purGLAABBInflatev(purGLAABB *aabb, GLint x, GLint y)
{
	aabb->xMin -= x;
	aabb->yMin -= y;
	aabb->xMax += x;
	aabb->yMax += y;
}

bool purGLAABBIsReset(purGLAABB *aabb)
{
	return (aabb->xMin == purGLAABBResetMin ||
			aabb->yMin == purGLAABBResetMin ||
			aabb->xMax == purGLAABBResetMax ||
			aabb->yMax == purGLAABBResetMax);
}

bool purGLAABBContainsPoint(purGLAABB *aabb, inkPoint point)
{
	return purGLAABBContainsPointv(aabb, (GLint)(point.x), (GLint)(point.y));
}

bool purGLAABBContainsPointv(purGLAABB *aabb, GLint x, GLint y)
{
	return ((x >= aabb->xMin) &&
			(x <= aabb->xMax) &&
			(y >= aabb->yMin) &&
			(y <= aabb->yMax));
}

bool purGLAABBIsEqual(purGLAABB *aabb1, purGLAABB *aabb2)
{
	return (aabb1->xMin == aabb2->xMin &&
			aabb1->yMin == aabb2->yMin &&
			aabb1->xMax == aabb2->xMax &&
			aabb1->yMax == aabb2->yMax);
}

//purGLAABBf purGLAABBfReset()
//{
//	return purGLAABBfMake(purGLAABBfResetMin, purGLAABBfResetMin, purGLAABBfResetMax, purGLAABBfResetMax);
//}
void purGLAABBfUpdate(purGLAABBf *toBeUpdated, purGLAABBf *checkVals)
{
	*toBeUpdated = purGLAABBfMake(fminf(toBeUpdated->xMin, checkVals->xMin),
								 fminf(toBeUpdated->yMin, checkVals->yMin),
								 fmaxf(toBeUpdated->xMax, checkVals->xMax),
								 fmaxf(toBeUpdated->yMax, checkVals->yMax));
}

void purGLAABBfExpand(purGLAABBf *aabb, inkPoint point)
{
	return purGLAABBfExpandv(aabb, point.x, point.y);
}

void purGLAABBfExpandv(purGLAABBf *aabb, GLfloat x, GLfloat y)
{
	*aabb = purGLAABBfMake(fminf(aabb->xMin, x),
						  fminf(aabb->yMin, y),
						  fmaxf(aabb->xMax, x),
						  fmaxf(aabb->yMax, y));
}

void purGLAABBfInflate(purGLAABBf *aabb, inkPoint point)
{
	return purGLAABBfInflatev(aabb, point.x, point.y);
}

void purGLAABBfInflatev(purGLAABBf *aabb, GLfloat x, GLfloat y)
{
	aabb->xMin -= x;
	aabb->yMin -= y;
	aabb->xMax += x;
	aabb->yMax += y;
}

bool purGLAABBfIsReset(purGLAABBf *aabb)
{
	return (inkIsEqualf(aabb->xMin, purGLAABBfResetMin) ||
			inkIsEqualf(aabb->xMin, purGLAABBfResetMin) ||
			inkIsEqualf(aabb->xMax, purGLAABBfResetMax) ||
			inkIsEqualf(aabb->xMax, purGLAABBfResetMax));
}

bool purGLAABBfContainsPoint(purGLAABBf *aabb, inkPoint point)
{
	return purGLAABBfContainsPointv(aabb, point.x, point.y);
}

bool purGLAABBfContainsPointv(purGLAABBf *aabb, GLfloat x, GLfloat y)
{
	return ((x >= aabb->xMin) &&
			(x <= aabb->xMax) &&
			(y >= aabb->yMin) &&
			(y <= aabb->yMax));
}

bool purGLAABBfIsEqual(purGLAABBf *aabb1, purGLAABBf *aabb2)
{
	return (inkIsEqualf(aabb1->xMin, aabb2->xMin) &&
			inkIsEqualf(aabb1->xMin, aabb2->yMin) &&
			inkIsEqualf(aabb1->xMax, aabb2->xMax) &&
			inkIsEqualf(aabb1->xMax, aabb2->yMax));
}

void inkMatrixConvertPoints(inkMatrix matrix,
									inkPoint *points,
									unsigned count)
{
	inkPoint *point;
	unsigned index;

	for (index = 0, point = points; index < count; ++index, ++point)
	{
		*point = inkMatrixTransformPoint(matrix, *point);
	}
}

void inkMatrixConvertPointsv(inkMatrix matrix,
									 float *xs,
									 float *ys,
									 unsigned count)
{
	float *curX;
	float *curY;
	unsigned int index;

	for (index = 0, curX = xs, curY = ys; index < count; ++curX, ++curY)
	{
		inkMatrixConvertPointv(matrix, curX, curY);
	}
}

void inkMatrixConvert4Points(inkMatrix matrix,
									 inkPoint *point0,
									 inkPoint *point1,
									 inkPoint *point2,
									 inkPoint *point3)
{
	inkMatrixConvertPointv(matrix, &(point0->x), &(point0->y));
	inkMatrixConvertPointv(matrix, &(point1->x), &(point1->y));
	inkMatrixConvertPointv(matrix, &(point2->x), &(point2->y));
	inkMatrixConvertPointv(matrix, &(point3->x), &(point3->y));
}

void inkMatrixConvert4Pointsv(inkMatrix matrix,
									  float *x0, float *y0,
									  float *x1, float *y1,
									  float *x2, float *y2,
									  float *x3, float *y3)
{
	
	inkMatrixConvertPointv(matrix, x0, y0);
	inkMatrixConvertPointv(matrix, x1, y1);
	inkMatrixConvertPointv(matrix, x2, y2);
	inkMatrixConvertPointv(matrix, x3, y3);
}

inkRect inkMatrixConvertRect(inkMatrix matrix, inkRect rect)
{
	inkRect retVal = rect;

	inkMatrixConvertRectv(matrix,
						   &(rect.origin.x),   &(rect.origin.y),
						   &(rect.size.width), &(rect.size.height));

	return retVal;
}

void inkMatrixConvertRectv(inkMatrix matrix,
								   float *x, float *y,
								   float *width, float *height)
{
	purGLAABBf aabb = purGLAABBfMake(*x, *y, *x + *width, *y + *height);

	aabb = inkMatrixConvertAABBf(matrix, aabb);

	*x = aabb.xMin;
	*y = aabb.yMin;
	*width  = aabb.xMax - aabb.xMin;
	*height = aabb.yMax - aabb.yMin;
}

purGLAABB inkMatrixConvertAABB(inkMatrix matrix, purGLAABB aabb)
{
	purGLAABB retVal = aabb;

	inkMatrixConvertAABBv(matrix,
							(&(retVal.xMin)),
							(&(retVal.yMin)),
							(&(retVal.xMax)),
							(&(retVal.yMax)));

	return retVal;
}

void inkMatrixConvertAABBv(inkMatrix matrix,
								   GLint *xMin, GLint *yMin,
								   GLint *xMax, GLint *yMax)
{
	float xMinf = *xMin;
	float yMinf = *yMin;
	float xMaxf = *xMax;
	float yMaxf = *yMax;

	inkMatrixConvertAABBfv(matrix, &xMinf, &yMinf, &xMaxf, &yMaxf);

	*xMin = floorf(xMinf);
	*yMin = floorf(yMinf);
	*xMax = ceilf(xMaxf);
	*yMax = ceilf(yMaxf);
}

purGLAABBf inkMatrixConvertAABBf(inkMatrix matrix, purGLAABBf aabb)
{
	purGLAABBf retVal = aabb;

	inkMatrixConvertAABBfv(matrix,
							&(retVal.xMin), &(retVal.yMin),
							&(retVal.xMax), &(retVal.yMax));

	return retVal;
}

void inkMatrixConvertAABBfv(inkMatrix matrix,
									GLfloat *xMin, GLfloat *yMin,
									GLfloat *xMax, GLfloat *yMax)
{
	inkPoint p1 = inkPointMake(*xMin, *yMin);
	inkPoint p2 = inkPointMake(*xMax, *yMin);
	inkPoint p3 = inkPointMake(*xMin, *yMax);
	inkPoint p4 = inkPointMake(*xMax, *yMax);

	inkMatrixConvert4Points(matrix, &p1, &p2, &p3, &p4);

	*xMin = fminf(p1.x, fminf(p2.x, fminf(p3.x, p4.x)));
	*yMin = fminf(p1.y, fminf(p2.y, fminf(p3.y, p4.y)));
	*xMax = fmaxf(p1.x, fmaxf(p2.x, fmaxf(p3.x, p4.x)));
	*yMax = fmaxf(p1.y, fmaxf(p2.y, fmaxf(p3.y, p4.y)));
}

bool _purGLRectContainsAABB(_purGLRect *rect, purGLAABB *aabb)
{
	// If max is less then, or min is greater then, then it is out of bounds.
	if (aabb->xMax < rect->x ||
		aabb->yMax < rect->y ||
		aabb->xMin > rect->x + rect->width ||
		aabb->yMin > rect->y + rect->height)
	{
		return false;
	}

	return true;
}
