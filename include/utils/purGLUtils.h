
#ifndef _PUR_GL_UTILS_H_
#define _PUR_GL_UTILS_H_

#include "purHeader.h"

#include "purGLInclude.h"
#include "inkGeometry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	GLfloat x, y;
} purGLVertex; // 8 - bytes

typedef struct
{
	GLfloat x, y;
	GLubyte r, g, b, a;
} purGLColorVertex; // 12 - bytes

typedef struct
{
	GLfloat x, y;
	GLfloat s, t;
} purGLTextureVertex; // 16 - bytes

typedef struct
{
	GLfloat x, y;
	GLubyte r, g, b, a;
	GLfloat s, t;
} purGLColoredTextureVertex; // 20 - bytes

typedef struct
{
	GLint xMin;
	GLint yMin;
	GLint xMax;
	GLint yMax;
} purGLAABB; // 16 - bytes

typedef struct
{
	GLfloat xMin;
	GLfloat yMin;
	GLfloat xMax;
	GLfloat yMax;
} purGLAABBf; // 16 - bytes

typedef struct
{
	purGLVertex *vertices;
	GLuint vertexCount;

	GLubyte r, g, b, a;
} purGLColorVertices; // 12 - bytes

typedef struct
{
	GLint x;
	GLint y;
	GLint width;
	GLint height;
} _purGLRect; // 16 - bytes

typedef purGLColorVertices* purGLColorVerticesRef;

#ifdef __cplusplus
}
#endif

purExtern purGLVertex purGLVertexMake(GLfloat x, GLfloat y);
purExtern purGLColorVertex purGLColorVertexMake(GLfloat x, GLfloat y, GLubyte r, GLubyte g, GLubyte b, GLubyte a);
purExtern purGLTextureVertex purGLTextureVertexMake(GLfloat x, GLfloat y, GLfloat s, GLfloat t);
purExtern purGLColoredTextureVertex purGLColoredTextureVertexMake(GLfloat x, GLfloat y, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat s, GLfloat t);
purExtern inkMatrix inkMatrixMake(GLfloat a, GLfloat b, GLfloat c, GLfloat d, GLfloat tx, GLfloat ty);
purExtern purGLAABB purGLAABBMake(GLint xMin, GLint yMin, GLint xMax, GLint yMax);
purExtern purGLAABBf purGLAABBfMakeWithInit();
purExtern purGLAABBf purGLAABBfMake(GLfloat xMin, GLfloat yMin, GLfloat xMax, GLfloat yMax);

purExtern purGLColorVerticesRef purGLColorVerticesRefMake(GLuint vertexCount, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
purExtern void purGLColorVerticesRefFree(purGLColorVertices* ref);
purExtern purGLColorVertices purGLColorVerticesMake(GLuint vertexCount, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
purExtern void purGLColorVerticesFree(purGLColorVertices *colorVertices);

purExtern const purGLAABB purGLAABBReset;
purExtern void purGLAABBUpdate(purGLAABB *toBeUpdated, purGLAABB *checkVals);
purExtern void purGLAABBExpand(purGLAABB *aabb, inkPoint point);
purExtern void purGLAABBExpandv(purGLAABB *aabb, GLint x, GLint y);
purExtern void purGLAABBInflate(purGLAABB *aabb, inkPoint point);
purExtern void purGLAABBInflatev(purGLAABB *aabb, GLint x, GLint y);
purExtern bool purGLAABBIsReset(purGLAABB *aabb);
purExtern bool purGLAABBContainsPoint(purGLAABB *aabb, inkPoint point);
purExtern bool purGLAABBContainsPointv(purGLAABB *aabb, GLint x, GLint y);
purExtern bool purGLAABBIsEqual(purGLAABB *aabb1, purGLAABB *aabb2);

purExtern const purGLAABBf purGLAABBfReset;
purExtern void purGLAABBfUpdate(purGLAABBf *toBeUpdated, purGLAABBf *checkVals);
purExtern void purGLAABBfExpand(purGLAABBf *aabb, inkPoint point);
purExtern void purGLAABBfExpandv(purGLAABBf *aabb, GLfloat x, GLfloat y);
purExtern void purGLAABBfInflate(purGLAABBf *aabb, inkPoint point);
purExtern void purGLAABBfInflatev(purGLAABBf *aabb, GLfloat x, GLfloat y);
purExtern bool purGLAABBfIsReset(purGLAABBf *aabb);
purExtern bool purGLAABBfContainsPoint(purGLAABBf *aabb, inkPoint point);
purExtern bool purGLAABBfContainsPointv(purGLAABBf *aabb, GLfloat x, GLfloat y);
purExtern bool purGLAABBfIsEqual(purGLAABBf *aabb1, purGLAABBf *aabb2);

purExtern inkPoint inkMatrixConvertPoint(inkMatrix matrix, inkPoint point);
purExtern void inkMatrixConvertPointv(inkMatrix matrix, GLfloat *x, GLfloat *y);
purExtern void inkMatrixConvertPoints(inkMatrix matrix, inkPoint *points, GLuint count);
purExtern void inkMatrixConvertPointsv(inkMatrix matrix, GLfloat *xs, GLfloat *ys, GLuint count);
purExtern void inkMatrixConvert4Points(inkMatrix matrix, inkPoint *point0, inkPoint *point1, inkPoint *point2, inkPoint *point3);
purExtern void inkMatrixConvert4Pointsv(inkMatrix matrix, GLfloat *x0, GLfloat *y0, GLfloat *x1, GLfloat *y1, GLfloat *x2, GLfloat *y2, GLfloat *x3, GLfloat *y3);
purExtern inkRect inkMatrixConvertRect(inkMatrix matrix, inkRect rect);
purExtern void inkMatrixConvertRectv(inkMatrix matrix, GLfloat *x, GLfloat *y, GLfloat *width, GLfloat *height);
purExtern purGLAABB inkMatrixConvertAABB(inkMatrix matrix, purGLAABB aabb);
purExtern void inkMatrixConvertAABBv(inkMatrix matrix, GLint *xMin, GLint *yMin, GLint *xMax, GLint *yMax);
purExtern purGLAABBf inkMatrixConvertAABBf(inkMatrix matrix, purGLAABBf aabb);
purExtern void inkMatrixConvertAABBfv(inkMatrix matrix, GLfloat *xMin, GLfloat *yMin, GLfloat *xMax, GLfloat *yMax);

purExtern bool _purGLRectContainsAABB(_purGLRect *rect, purGLAABB *aabb);

#endif
