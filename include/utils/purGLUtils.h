
#ifndef _PUR_GL_UTILS_H_
#define _PUR_GL_UTILS_H_

#include "inkHeader.h"

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

inkExtern purGLVertex purGLVertexMake(GLfloat x, GLfloat y);
inkExtern purGLColorVertex purGLColorVertexMake(GLfloat x, GLfloat y, GLubyte r, GLubyte g, GLubyte b, GLubyte a);
inkExtern purGLTextureVertex purGLTextureVertexMake(GLfloat x, GLfloat y, GLfloat s, GLfloat t);
inkExtern purGLColoredTextureVertex purGLColoredTextureVertexMake(GLfloat x, GLfloat y, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat s, GLfloat t);
inkExtern inkMatrix inkMatrixMake(GLfloat a, GLfloat b, GLfloat c, GLfloat d, GLfloat tx, GLfloat ty);
inkExtern purGLAABB purGLAABBMake(GLint xMin, GLint yMin, GLint xMax, GLint yMax);
inkExtern purGLAABBf purGLAABBfMakeWithInit();
inkExtern purGLAABBf purGLAABBfMake(GLfloat xMin, GLfloat yMin, GLfloat xMax, GLfloat yMax);

inkExtern purGLColorVerticesRef purGLColorVerticesRefMake(GLuint vertexCount, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
inkExtern void purGLColorVerticesRefFree(purGLColorVertices* ref);
inkExtern purGLColorVertices purGLColorVerticesMake(GLuint vertexCount, GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
inkExtern void purGLColorVerticesFree(purGLColorVertices *colorVertices);

inkExtern const purGLAABB purGLAABBReset;
inkExtern void purGLAABBUpdate(purGLAABB *toBeUpdated, purGLAABB *checkVals);
inkExtern void purGLAABBExpand(purGLAABB *aabb, inkPoint point);
inkExtern void purGLAABBExpandv(purGLAABB *aabb, GLint x, GLint y);
inkExtern void purGLAABBInflate(purGLAABB *aabb, inkPoint point);
inkExtern void purGLAABBInflatev(purGLAABB *aabb, GLint x, GLint y);
inkExtern bool purGLAABBIsReset(purGLAABB *aabb);
inkExtern bool purGLAABBContainsPoint(purGLAABB *aabb, inkPoint point);
inkExtern bool purGLAABBContainsPointv(purGLAABB *aabb, GLint x, GLint y);
inkExtern bool purGLAABBIsEqual(purGLAABB *aabb1, purGLAABB *aabb2);

inkExtern const purGLAABBf purGLAABBfReset;
inkExtern void purGLAABBfUpdate(purGLAABBf *toBeUpdated, purGLAABBf *checkVals);
inkExtern void purGLAABBfExpand(purGLAABBf *aabb, inkPoint point);
inkExtern void purGLAABBfExpandv(purGLAABBf *aabb, GLfloat x, GLfloat y);
inkExtern void purGLAABBfInflate(purGLAABBf *aabb, inkPoint point);
inkExtern void purGLAABBfInflatev(purGLAABBf *aabb, GLfloat x, GLfloat y);
inkExtern bool purGLAABBfIsReset(purGLAABBf *aabb);
inkExtern bool purGLAABBfContainsPoint(purGLAABBf *aabb, inkPoint point);
inkExtern bool purGLAABBfContainsPointv(purGLAABBf *aabb, GLfloat x, GLfloat y);
inkExtern bool purGLAABBfIsEqual(purGLAABBf *aabb1, purGLAABBf *aabb2);

inkExtern inkPoint inkMatrixConvertPoint(inkMatrix matrix, inkPoint point);
inkExtern void inkMatrixConvertPointv(inkMatrix matrix, GLfloat *x, GLfloat *y);
inkExtern void inkMatrixConvertPoints(inkMatrix matrix, inkPoint *points, GLuint count);
inkExtern void inkMatrixConvertPointsv(inkMatrix matrix, GLfloat *xs, GLfloat *ys, GLuint count);
inkExtern void inkMatrixConvert4Points(inkMatrix matrix, inkPoint *point0, inkPoint *point1, inkPoint *point2, inkPoint *point3);
inkExtern void inkMatrixConvert4Pointsv(inkMatrix matrix, GLfloat *x0, GLfloat *y0, GLfloat *x1, GLfloat *y1, GLfloat *x2, GLfloat *y2, GLfloat *x3, GLfloat *y3);
inkExtern inkRect inkMatrixConvertRect(inkMatrix matrix, inkRect rect);
inkExtern void inkMatrixConvertRectv(inkMatrix matrix, GLfloat *x, GLfloat *y, GLfloat *width, GLfloat *height);
inkExtern purGLAABB inkMatrixConvertAABB(inkMatrix matrix, purGLAABB aabb);
inkExtern void inkMatrixConvertAABBv(inkMatrix matrix, GLint *xMin, GLint *yMin, GLint *xMax, GLint *yMax);
inkExtern purGLAABBf inkMatrixConvertAABBf(inkMatrix matrix, purGLAABBf aabb);
inkExtern void inkMatrixConvertAABBfv(inkMatrix matrix, GLfloat *xMin, GLfloat *yMin, GLfloat *xMax, GLfloat *yMax);

inkExtern bool _purGLRectContainsAABB(_purGLRect *rect, purGLAABB *aabb);

#endif
