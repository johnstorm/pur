/*
 *  purGLRenderer.c
 *  pur
 *
 *  Created by John Lattin on 12/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "purGLRenderer.h"
#include "purGLPrivate.h"

#include "purPrivateUtils.h"
#include "purGLStatePrivate.h"

#define PUR_GL_RENDERER_MAX_VERTICES 0xFFFF
#define PUR_GL_RENDERER_MAX_VERTICES_MINUS_2 0xFFFD

#define PUR_GL_RENDERER_MIN_BUFFER_SIZE 16

#define PUR_GL_RENDERER_SEND_CORRECTED_SIZE

//#define PUR_RENDER_VBO

typedef struct
{
	unsigned int size;
	unsigned int maxSize;

	purGLColoredTextureVertex *array;
} _purGLVertexBuffer;

typedef struct
{
	unsigned int size;
	unsigned int maxSize;

	GLushort *array;
} _purGLIndexBuffer;

typedef struct
{
	unsigned int size;
	unsigned int maxSize;

	GLfloat *array;
} _purGLPointSizeBuffer;

typedef struct
{
	unsigned int size;
	unsigned int maxSize;
	
	purGLElementBucket *array;
} _purGLElementBucketBuffer;

purGLColoredTextureVertex *purGLVertexBufferCurrentObject = NULL;
GLushort *purGLIndexBufferCurrentObject = NULL;
GLfloat *purGLPointSizeBufferCurrentObject = NULL;
purGLElementBucket *purGLElementBucketBufferCurrentObject = NULL;

purGLState purGLDefaultState;
purGLState purGLCurState;
purGLState purGLStateInGL;

GLuint purGLBufferVertexColorState = PUR_GL_VERTEX_COLOR_RESET;

unsigned int purGLVertexBufferMaxSize = 0;
unsigned int purGLVertexOldBufferMaxSize = 0;

unsigned int purGLIndexBufferMaxSize = 0;
unsigned int purGLIndexOldBufferMaxSize = 0;

unsigned int purGLPointSizeBufferMaxSize = 0;
unsigned int purGLPointSizeOldBufferMaxSize = 0;

unsigned int purGLElementBucketBufferMaxSize = 0;
unsigned int purGLElementBucketOldBufferMaxSize = 0;

_purGLVertexBuffer purGLVertexBuffer;
_purGLIndexBuffer purGLIndexBuffer;
_purGLPointSizeBuffer purGLPointSizeBuffer;
_purGLElementBucketBuffer purGLElementBucketBuffer;

GLenum purGLDrawMode = 0;

const GLubyte purGLSizeOfIndex = sizeof(GLushort);
const GLubyte purGLSizeOfPointSize = sizeof(GLfloat);
GLubyte purGLIsColorArrayEnabled = false;
GLubyte purGLShouldDrawElements = false;

GLubyte purGLBufferLastVertexRed   = 0xFF;
GLubyte purGLBufferLastVertexGreen = 0xFF;
GLubyte purGLBufferLastVertexBlue  = 0xFF;
GLubyte purGLBufferLastVertexAlpha = 0xFF;

GLushort purGLHadDrawnElements = false;
GLushort purGLHadDrawnArrays = false;

#ifdef PUR_DEBUG_MODE
int purGLDrawCallCount = 0;
#endif

#ifdef PUR_RENDER_VBO
GLuint purGLBufferVertexID = 0;
unsigned int purGLLastMaxBufferVertexSize = 0;
GLuint purGLBufferIndexID  = 0;
#endif

/*
 * This method initializes the buffer arrays.
 */
void purGLRendererInit()
{
	//Set the size to 0, set the max size to the minimum allowed size, and
	//allocate some memory.

	purGLVertexBuffer.size = 0;
	purGLVertexBuffer.maxSize = PUR_GL_RENDERER_MIN_BUFFER_SIZE;
	purGLVertexBuffer.array = malloc(sizeof(purGLColoredTextureVertex) * purGLVertexBuffer.maxSize);

	purGLIndexBuffer.size = 0;
	purGLIndexBuffer.maxSize = PUR_GL_RENDERER_MIN_BUFFER_SIZE;
	purGLIndexBuffer.array = malloc(purGLSizeOfIndex * purGLIndexBuffer.maxSize);

	purGLPointSizeBuffer.size = 0;
	purGLPointSizeBuffer.maxSize = PUR_GL_RENDERER_MIN_BUFFER_SIZE;
	purGLPointSizeBuffer.array = malloc(purGLSizeOfPointSize * purGLPointSizeBuffer.maxSize);

	purGLElementBucketBuffer.size = 0;
	purGLElementBucketBuffer.maxSize = PUR_GL_RENDERER_MIN_BUFFER_SIZE;
	purGLElementBucketBuffer.array = malloc(sizeof(purGLElementBucket) * purGLElementBucketBuffer.maxSize);

	purGLVertexBufferCurrentObject = purGLVertexBuffer.array;
	purGLIndexBufferCurrentObject = purGLIndexBuffer.array;
	purGLPointSizeBufferCurrentObject = purGLPointSizeBuffer.array;

#ifdef PUR_RENDER_VBO
	glGenBuffers(1, &purGLBufferVertexID);
	glGenBuffers(1, &purGLBufferIndexID);
#endif
}

/*
 * This method frees the memory used by the buffers.
 */
void purGLRendererDealloc()
{
	//If the buffer arrays exist, we should free their memory.

	if (purGLVertexBuffer.array)
	{
		free(purGLVertexBuffer.array);
		purGLVertexBuffer.array = NULL;
	}

	if (purGLIndexBuffer.array)
	{
		free(purGLIndexBuffer.array);
		purGLIndexBuffer.array = NULL;
	}

	if (purGLPointSizeBuffer.array)
	{
		free(purGLPointSizeBuffer.array);
		purGLPointSizeBuffer.array = NULL;
	}

	if (purGLElementBucketBuffer.array)
	{
		free(purGLElementBucketBuffer.array);
		purGLElementBucketBuffer.array = NULL;
	}

#ifdef PUR_RENDER_VBO
	if (purGLBufferVertexID)
		glDeleteBuffers(1, &purGLBufferVertexID);
	if (purGLBufferIndexID)
		glDeleteBuffers(1, &purGLBufferIndexID);
#endif
}

/*
 * This method sets the draw mode, switching modes will cause the buffer to be
 * flushed.  Line loops and strips will also cause the buffer to be flushed, as
 * they can not be combined with anything else.
 *
 * @param GLenum mode - Specifies what kind of primitives to render. Symbolic
 * constants GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES,
 * GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, and GL_TRIANGLES are accepted.
 */
void purGLSetDrawMode(GLenum mode)
{
	// Check to see if our mode has changed, or if we are equal to line loop or
	// strip; if so, then we need to flush the buffer and change modes.
	if (mode != purGLDrawMode || mode == GL_LINE_LOOP || mode == GL_LINE_STRIP || mode == GL_TRIANGLE_FAN)
	{
		purGLFlushBuffer();
		purGLDrawMode = mode;
	}
}

/*
 * This method checks how frequently the color changes, which deterimines later
 * whether or not GLColor4ub or color GLColorPointer should be used.  One
 * should check if bufferVertexColorState is not equal to
 * PUR_GL_VERTEX_COLOR_MULTIPLE prior to calling this function.
 *
 * @param GLubyte red   - The red value for the color [0,255]
 * @param GLubyte green - The green value for the color [0,255]
 * @param GLubyte blue  - The blue value for the color [0,255]
 * @param GLubyte alpha - The alpha value for the color [0,255]
 */
void purGLSetBufferLastVertexColor(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	//It should always be within this range.
	assert((purGLBufferVertexColorState <= PUR_GL_VERTEX_COLOR_MULTIPLE));

	//Check to see if the color has changed, if so increment the value.
	if (red   != purGLBufferLastVertexRed   ||
		green != purGLBufferLastVertexGreen ||
		blue  != purGLBufferLastVertexBlue  ||
		alpha != purGLBufferLastVertexAlpha ||
		purGLBufferVertexColorState == PUR_GL_VERTEX_COLOR_RESET)
	{
		++purGLBufferVertexColorState;

		purGLBufferLastVertexRed   = red;
		purGLBufferLastVertexGreen = green;
		purGLBufferLastVertexBlue  = blue;
		purGLBufferLastVertexAlpha = alpha;
	}
}

/*
 * This method enables the color array if it is not enabeled already.
 */
void purGLEnableColorArray()
{
	//Check to see if the color array is enabled, if it is then we can just
	//return as our job is done.
	if (purGLIsColorArrayEnabled)
		return;

	//Actually set the state in GL
	glEnableClientState(GL_COLOR_ARRAY);
	purGLIsColorArrayEnabled = true;
}

/*
 * This method disables the color array if it is not disabled already.
 */
void purGLDisableColorArray()
{
	//Check to see if the color array is not enabled, if that is the case then
	//we can just return as our job is done.
	if (!purGLIsColorArrayEnabled)
		return;

	//Actually set the state in GL
	glDisableClientState(GL_COLOR_ARRAY);
	purGLIsColorArrayEnabled = false;
}

/*
 * This method simply returns the current size that the vertex buffer is at.
 *
 * @return - the current size that the vertex buffer is at.
 */
unsigned int purGLGetCurrentVertexIndex()
{
	return purGLVertexBuffer.size;
}

/*
 * This method sets the current vertex size (aka. its current indexed
 * position).
 *
 * @param unsigned int index - The current indexed position.
 */
void purGLSetCurrentVertexIndex(unsigned int index)
{
	// The index should never be greater or equal to the max size, the max size
	// denotes the actual size that is allocated by the buffer array.
	assert(index < purGLVertexBuffer.maxSize);

	purGLVertexBuffer.size = index;
	purGLVertexBufferCurrentObject = purGLVertexBuffer.array + purGLVertexBuffer.size;
}

/*
 * This method simply returns the current size that the index buffer is at.
 *
 * @return - the current size that the index buffer is at.
 */
unsigned int purGLGetCurrentIndex()
{
	return purGLIndexBuffer.size;
}

/*
 * This method sets the current index buffer size (aka. its current indexed
 * position).
 *
 * @param unsigned int index - The current indexed position.
 */
void purGLSetCurrentIndex(unsigned int index)
{
	//The index should never be greater or equal to the max size, the max size
	//denotes the actual size that is allocated by the buffer array.
	assert(index < purGLIndexBuffer.maxSize);

	purGLIndexBuffer.size = index;
	purGLIndexBufferCurrentObject = purGLIndexBuffer.array + purGLIndexBuffer.size;
}

/*
 * This method simply returns the current size that the point size buffer is
 * at.
 *
 * @return - the current size that the point size buffer is at.
 */
unsigned int purGLGetCurrentPointSizeIndex()
{
	return purGLPointSizeBuffer.size;
}

/*
 * This method sets the current point size buffer size (aka. its current
 * indexed position).
 *
 * @param unsigned int index - The current indexed position.
 */
void purGLSetCurrentPointSizeIndex(unsigned int index)
{
	//The index should never be greater or equal to the max size, the max size
	//denotes the actual size that is allocated by the buffer array.
	assert(index < purGLPointSizeBuffer.maxSize);

	purGLPointSizeBuffer.size = index;
	purGLPointSizeBufferCurrentObject = purGLPointSizeBuffer.array + purGLPointSizeBuffer.size;
}

/*
 * This method returns a pointer to the vertex at a given index.  If the index
 * is out of bounds then an assertion is thrown (in debug mode).
 *
 * @return - A pointer to the vertex at the given index.
 */
purGLColoredTextureVertex *purGLGetVertexAt(unsigned int index)
{
	assert(purGLVertexBuffer.size != 0 && index < purGLVertexBuffer.size);

	return purGLVertexBuffer.array + index;
}

/*
 * This method returns a pointer to the current vertex.  If the buffer is empty
 * this will assert so (in debug mode).
 *
 * @return - A pointer to the current vertex.
 */
purGLColoredTextureVertex *purGLCurrentVertex()
{
	assert(purGLVertexBuffer.size != 0);

	return purGLVertexBufferCurrentObject - 1;
}

purGLColoredTextureVertex *purGLAskForVertices(unsigned int count)
{
	while (purGLVertexBuffer.size + count >= purGLVertexBuffer.maxSize)
	{
		//Lets double the size of the array
		purGLVertexBuffer.maxSize <<= 1;
		purGLVertexBuffer.array = realloc(purGLVertexBuffer.array, sizeof(purGLColoredTextureVertex) * purGLVertexBuffer.maxSize);
		purGLVertexBufferCurrentObject = purGLVertexBuffer.array + purGLVertexBuffer.size;
	}

	// Lets return the next available vertex for use.
	return purGLVertexBufferCurrentObject;
}

void purGLUsedVertices(unsigned int count)
{
	purGLVertexBufferCurrentObject += count;
	purGLVertexBuffer.size += count;
}

/*
 * This method returns a pointer to the index at a given index.  If the index
 * is out of bounds then an assertion is thrown (in debug mode).
 *
 * @return - A pointer to the index at the given index.
 */
GLushort *purGLGetIndexAt(unsigned int index)
{
	assert(purGLIndexBuffer.size != 0 && index < purGLIndexBuffer.size);

	return purGLIndexBuffer.array + index;
}

/*
 * This method returns a pointer to the current index.  If the buffer is empty
 * this will assert so (in debug mode).
 *
 * @return - A pointer to the current index.
 */
GLushort *purGLCurrentIndex()
{
	assert(purGLIndexBuffer.size != 0);

	return purGLIndexBufferCurrentObject - 1;
}

GLushort *purGLAskForIndices(unsigned int count)
{
	while (purGLIndexBuffer.size + count >= purGLIndexBuffer.maxSize)
	{
		//Lets double the size of the array
		purGLIndexBuffer.maxSize <<= 1;
		purGLIndexBuffer.array = realloc(purGLIndexBuffer.array, purGLSizeOfIndex * purGLIndexBuffer.maxSize);
		purGLIndexBufferCurrentObject = purGLIndexBuffer.array + purGLIndexBuffer.size;
	}

	// Lets return the next available vertex for use.
	return purGLIndexBufferCurrentObject;
}
void purGLUsedIndices(unsigned int count)
{
	purGLIndexBufferCurrentObject += count;
	purGLIndexBuffer.size += count;
}

/*
 * This method returns a pointer to the point size at a given index.  If the
 * index is out of bounds then an assertion is thrown (in debug mode).
 *
 * @return - A pointer to the point size at the given index.
 */
GLfloat *purGLGetPointSizeAt(unsigned int index)
{
	assert(purGLPointSizeBuffer.size != 0 && index < purGLPointSizeBuffer.size);

	return purGLPointSizeBuffer.array + index;
}

/*
 * This method returns a pointer to the current point size.  If the buffer is
 * empty this will assert so (in debug mode).
 *
 * @return - A pointer to the current point size.
 */
GLfloat *purGLCurrentPointSize()
{
	assert(purGLPointSizeBuffer.size != 0);

	return purGLPointSizeBufferCurrentObject - 1;
}

GLfloat *purGLAskForPointSizes(unsigned int count)
{
	while (purGLPointSizeBuffer.size + count >= purGLPointSizeBuffer.maxSize)
	{
		//Lets double the size of the array
		purGLPointSizeBuffer.maxSize <<= 1;
		purGLPointSizeBuffer.array = realloc(purGLPointSizeBuffer.array, purGLSizeOfPointSize * purGLPointSizeBuffer.maxSize);
		purGLPointSizeBufferCurrentObject = purGLPointSizeBuffer.array + purGLPointSizeBuffer.size;
	}

	// Lets return the next available vertex for use.
	return purGLPointSizeBufferCurrentObject;
}

void purGLUsedPointSizes(unsigned int count)
{
	purGLPointSizeBufferCurrentObject += count;
	purGLPointSizeBuffer.size += count;
}

purGLElementBucket *purGLGetElementBuckets(unsigned int maxBucketVal)
{
	purGLElementBucketBuffer.size = maxBucketVal;

	while (purGLElementBucketBuffer.size >= purGLElementBucketBuffer.maxSize)
	{
		//Lets double the size of the array
		purGLElementBucketBuffer.maxSize <<= 1;
		purGLElementBucketBuffer.array = realloc(purGLElementBucketBuffer.array, sizeof(purGLElementBucket) * purGLElementBucketBuffer.maxSize);
	}

	if (purGLElementBucketBufferMaxSize < purGLElementBucketBuffer.size)
		purGLElementBucketBufferMaxSize = purGLElementBucketBuffer.size;

	memset(purGLElementBucketBuffer.array, 0, sizeof(purGLElementBucket) * purGLElementBucketBuffer.size);

	// Lets return the next available vertex for use.
	return purGLElementBucketBuffer.array;
}

/*
 * This method runs through all of the pre-render commands... which as of now
 * none exist.
 */
void purGLRendererPreRender()
{
}

/*
 * This method flushes the buffer, incase anything is left in it at the end of
 * a render cycle.
 */
void purGLRendererPostRender()
{
	assert(purGLVertexBuffer.array);

	//purGLSetupEnables();
	// If the array isn't empty, then we need to draw what is left inside it.
	purGLFlushBuffer();
}

/*
 * This method consolidates the buffers, meaning if any of the buffers are less
 * then 1/4 of their max size, then it will reduce the size of the buffer to
 * half of it's normal size.
 */
void purGLConsolidateBuffer()
{
	assert(purGLVertexBuffer.array);

	//Check to see if the max size is less then then a quarter the size of the
	//previous max.  If it is then make the new size equal to double that of the
	//current max.
	if (purGLHadDrawnArrays && purGLVertexBufferMaxSize < (purGLVertexOldBufferMaxSize >> 2))
	{
		int newMaxSize = purGLVertexBuffer.maxSize >> 1;
		if (newMaxSize > PUR_GL_RENDERER_MIN_BUFFER_SIZE)
		{
			purGLVertexBuffer.maxSize = newMaxSize;
			purGLVertexBuffer.array = realloc(purGLVertexBuffer.array, sizeof(purGLColoredTextureVertex) * purGLVertexBuffer.maxSize);
			purGLVertexBufferCurrentObject = purGLVertexBuffer.array + purGLVertexBuffer.size;
		}
	}

	//Lets check it for indices now.
	if (purGLHadDrawnElements && purGLIndexBufferMaxSize < (purGLIndexOldBufferMaxSize >> 2))
	{
		int newMaxSize = purGLIndexBuffer.maxSize >> 1;
		if (newMaxSize > PUR_GL_RENDERER_MIN_BUFFER_SIZE)
		{
			purGLIndexBuffer.maxSize = newMaxSize;
			purGLIndexBuffer.array = realloc(purGLIndexBuffer.array, purGLSizeOfIndex * purGLIndexBuffer.maxSize);
			purGLIndexBufferCurrentObject = purGLIndexBuffer.array + purGLIndexBuffer.size;
		}
	}

	//Lets check it for point sizes now.
	if (purGLPointSizeBufferMaxSize < (purGLPointSizeOldBufferMaxSize >> 2))
	{
		int newMaxSize = purGLPointSizeBuffer.maxSize >> 1;
		if (newMaxSize > PUR_GL_RENDERER_MIN_BUFFER_SIZE)
		{
			purGLPointSizeBuffer.maxSize = newMaxSize;
			purGLPointSizeBuffer.array = realloc(purGLPointSizeBuffer.array, purGLSizeOfPointSize * purGLPointSizeBuffer.maxSize);
			purGLPointSizeBufferCurrentObject = purGLPointSizeBuffer.array + purGLPointSizeBuffer.size;
		}
	}

	if (purGLHadDrawnElements && purGLElementBucketBufferMaxSize < (purGLElementBucketBufferMaxSize >> 2))
	{
		int newMaxSize = purGLElementBucketBuffer.maxSize >> 1;
		if (newMaxSize > PUR_GL_RENDERER_MIN_BUFFER_SIZE)
		{
			purGLElementBucketBuffer.maxSize = newMaxSize;
			purGLElementBucketBuffer.array = realloc(purGLElementBucketBuffer.array, sizeof(purGLElementBucket) * purGLElementBucketBuffer.maxSize);
		}
	}

	//Set the old max to the new max.
	purGLVertexOldBufferMaxSize = purGLVertexBufferMaxSize;
	purGLIndexOldBufferMaxSize = purGLIndexBufferMaxSize;
	purGLPointSizeOldBufferMaxSize = purGLPointSizeBufferMaxSize;
	purGLElementBucketOldBufferMaxSize = purGLElementBucketBufferMaxSize;

	//Reset the max.
	purGLVertexBufferMaxSize = 0;
	purGLIndexBufferMaxSize = 0;
	purGLPointSizeBufferMaxSize = 0;
	purGLElementBucketBufferMaxSize = 0;

	//Set the variables to have yet been drawn.
	purGLHadDrawnArrays = false;
	purGLHadDrawnElements = false;
}

purInline void purGLDraw()
{
	// If the array is larger then max vertices, we should flush it in chunks.
	// This is best done by flushing up until MAX_VERTICES - 2, then again from
	// MAX_VERTICES - 2 until whatever happens next.  This could be
	// 2 * (MAX_VERTICES - 2) or size.  This needs to continue until it is empty

	// HAVE TO BE SIGNED
	int start;
	int amountToDraw = purGLShouldDrawElements ? purGLIndexBuffer.size : purGLVertexBuffer.size;

	if (purGLShouldDrawElements)
	{
		for (start = 0; amountToDraw > 0; start += PUR_GL_RENDERER_MAX_VERTICES_MINUS_2, amountToDraw -= PUR_GL_RENDERER_MAX_VERTICES_MINUS_2)
			glDrawElements(purGLDrawMode, ((amountToDraw < PUR_GL_RENDERER_MAX_VERTICES) ? amountToDraw : PUR_GL_RENDERER_MAX_VERTICES), GL_UNSIGNED_SHORT, purGLIndexBuffer.array + start);
	}
	else
	{
		for (start = 0; amountToDraw > 0; start += PUR_GL_RENDERER_MAX_VERTICES_MINUS_2, amountToDraw -= PUR_GL_RENDERER_MAX_VERTICES_MINUS_2)
			glDrawArrays(purGLDrawMode, start, ((amountToDraw < PUR_GL_RENDERER_MAX_VERTICES) ? amountToDraw : PUR_GL_RENDERER_MAX_VERTICES));
	}
}

/*
 * This method flushes the buffer to GL, meaning that it takes whatever the
 * buffer status is right now, and calls the appropriate methods in gl to
 * display them.
 */
void purGLFlushBufferToGL()
{
	// Check to see if the color array is enabled, or if we are going to use a
	// color array anyway, if so... lets turn it on!
	if (purGLBufferVertexColorState == PUR_GL_VERTEX_COLOR_MULTIPLE ||
		PUR_IS_BIT_ENABLED(purGLStateInGL.clientState, PUR_GL_COLOR_ARRAY))
		//PUR_IS_BIT_ENABLED(purGLClientStateInGL, PUR_GL_COLOR_ARRAY))
	{
		purGLEnableColorArray();
	}
	else
	{
		purGLDisableColorArray();
		glColor4ub(purGLBufferLastVertexRed,
				   purGLBufferLastVertexGreen,
				   purGLBufferLastVertexBlue,
				   purGLBufferLastVertexAlpha);
	}

	// If the point size array is enabled, then lets set the pointer for it.
	if (PUR_IS_BIT_ENABLED(purGLStateInGL.clientState, PUR_GL_POINT_SIZE_ARRAY))
	//if (PUR_IS_BIT_ENABLED(purGLClientStateInGL, PUR_GL_POINT_SIZE_ARRAY))
		glPointSizePointerOES(GL_FLOAT, 0, purGLPointSizeBuffer.array);

	// shorts even though it is actually a boolean, for alignment
	int isTextured = PUR_IS_BIT_ENABLED(purGLStateInGL.clientState, PUR_GL_TEXTURE_COORD_ARRAY);
	//int isTextured = PUR_IS_BIT_ENABLED(purGLClientStateInGL, PUR_GL_TEXTURE_COORD_ARRAY);

#ifdef PUR_RENDER_VBO
	if (purGLBufferVertexID)
	{
		glBindBuffer(GL_ARRAY_BUFFER, purGLBufferVertexID);

	//	if (purGLLastMaxBufferVertexSize != purGLVertexBuffer.maxSize)
	//	{
	//		purGLLastMaxBufferVertexSize = purGLVertexBuffer.maxSize;
	//		glBufferData(GL_ARRAY_BUFFER, sizeof(purGLColoredTextureVertex) * purGLVertexBuffer.maxSize, purGLVertexBuffer.array, GL_DYNAMIC_DRAW);
	//	}
	//	else
	//	{
	//		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(purGLColoredTextureVertex) * purGLVertexBuffer.size, purGLVertexBuffer.array);
	//	}
		glBufferData(GL_ARRAY_BUFFER, sizeof(purGLColoredTextureVertex) * purGLVertexBuffer.size, purGLVertexBuffer.array, GL_DYNAMIC_DRAW);

		glVertexPointer(2, GL_FLOAT, sizeof(purGLColoredTextureVertex), (GLvoid *)(0));
		if (purGLIsColorArrayEnabled)
			glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(purGLColoredTextureVertex), (GLvoid *)(sizeof(purGLVertex)));
		if (isTextured)
			glTexCoordPointer(2, GL_FLOAT, sizeof(purGLColoredTextureVertex), (GLvoid *)(sizeof(purGLColorVertex)));

		if (purGLShouldDrawElements)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, purGLBufferIndexID);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(purGLSizeOfIndex) * purGLIndexBuffer.size, purGLIndexBuffer.array, GL_DYNAMIC_DRAW);
			glDrawElements(purGLDrawMode, purGLIndexBuffer.size, GL_UNSIGNED_SHORT, (GLvoid *)(0));
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		}
		else
		{
			glDrawArrays(purGLDrawMode, 0, purGLVertexBuffer.size);
		}

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
#else
	// TODO Later:	Stop this dumb copying. Instead just use the correct chunks
	//				of memory. The reason this is not done now is due to the
	//				color array delima
#ifndef PUR_GL_RENDERER_SEND_CORRECTED_SIZE
	glVertexPointer(2, GL_FLOAT, sizeof(purGLColoredTextureVertex), &(purGLVertexBuffer.array->x));
	if (isTextured)
		glTexCoordPointer(2, GL_FLOAT, sizeof(purGLColoredTextureVertex), &(purGLVertexBuffer.array->s));
	if (purGLIsColorArrayEnabled)
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(purGLColoredTextureVertex), &(purGLVertexBuffer.array->r));
	
	// Have to call draw here because we are using stack memory
	purGLDraw();
#else
	// If we are textured,, and color array is turned on, then we don't need to
	// manipulate the array.. we can just pass it to gl.
	if (isTextured && purGLIsColorArrayEnabled)
	{
		glVertexPointer(2, GL_FLOAT, sizeof(purGLColoredTextureVertex), &(purGLVertexBuffer.array->x));
		glTexCoordPointer(2, GL_FLOAT, sizeof(purGLColoredTextureVertex), &(purGLVertexBuffer.array->s));
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(purGLColoredTextureVertex), &(purGLVertexBuffer.array->r));

		// Have to call draw here because we are using stack memory
		purGLDraw();
	}
	// If we are textured, but not using a color array, then we are going to
	// store the data into a textured vertex struct.  This means we need to copy
	// the previous data before giving it to gl.
	else if (isTextured)
	{
		// Lets make an array of the same size, except of just textured vertex
		// type.
		purGLTextureVertex vertices[purGLVertexBuffer.size];

		purGLTextureVertex *vertex = vertices;
		purGLColoredTextureVertex *oldVertex = purGLVertexBuffer.array;
		// Lets go through the old array, and copy the values.
		for (unsigned int index = 0; index < purGLVertexBuffer.size; ++index)
		{
			vertex->x = oldVertex->x;
			vertex->y = oldVertex->y;
			vertex->s = oldVertex->s;
			vertex->t = oldVertex->t;
			++vertex;
			++oldVertex;
		}

//		glEnable(GL_TEXTURE_2D);
//		glEnableClientState(GL_VERTEX_ARRAY);
//		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		// Lets pass the info onto gl
		glVertexPointer(2, GL_FLOAT, sizeof(purGLTextureVertex), &(vertices->x));
		glTexCoordPointer(2, GL_FLOAT, sizeof(purGLTextureVertex), &(vertices->s));

		// Have to call draw here because we are using stack memory
		purGLDraw();
	}
	// If the color array is turned on, but it isn't textured, then lets copy
	// the values into a colored vertex array prior to sending it to gl.
	else if (purGLIsColorArrayEnabled)
	{
		purGLColorVertex vertices[purGLVertexBuffer.size];
		purGLColorVertex *vertex = vertices;
		purGLColoredTextureVertex *oldVertex = purGLVertexBuffer.array;
		// Iterate through the array copying over the values to the new one.
		for (unsigned int index = 0; index < purGLVertexBuffer.size; ++index)
		{
			vertex->x = oldVertex->x;
			vertex->y = oldVertex->y;
			vertex->r = oldVertex->r;
			vertex->g = oldVertex->g;
			vertex->b = oldVertex->b;
			vertex->a = oldVertex->a;

			++vertex;
			++oldVertex;
		}

		glVertexPointer(2, GL_FLOAT, sizeof(purGLColorVertex), &(vertices->x));
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(purGLColorVertex), &(vertices->r));

		//Have to call draw here because we are using stack memory
		purGLDraw();
	}
	//If the vertices aren't colored, and aren't textured, then lets just draw
	//the vertices.  Before sending the array to GL, lets copy it over.
	else
	{
		//Make an array of just vertices, this needs to be equal size of the
		//previous array.
		purGLVertex vertices[purGLVertexBuffer.size];
		purGLVertex *vertex = vertices;
		purGLColoredTextureVertex *oldVertex = purGLVertexBuffer.array;

		for (unsigned int index = 0; index < purGLVertexBuffer.size; ++index)
		{
			vertex->x = oldVertex->x;
			vertex->y = oldVertex->y;
			++vertex;
			++oldVertex;
		}

		glVertexPointer(2, GL_FLOAT, sizeof(purGLVertex), &(vertices->x));

		//Have to call draw here because we are using stack memory
		purGLDraw();
	}
#endif // PUR_GL_RENDERER_SEND_CORRECTED_SIZE

#endif // PUR_RENDER_VBO

#ifdef PUR_DEBUG_MODE
	++purGLDrawCallCount;
#endif
}

/*
 * This method flushes the buffer, if the buffer is empty then nothing occurs.
 */
void purGLFlushBuffer()
{
	//If the buffer is empty, lets just return.
	if (purGLVertexBuffer.size == 0)
		return;

	//Lets check to see if we are going to draw elements, if so then we should
	//check to see if we have any indices, if not then we can simply return as
	//there is nothing to draw.
	purGLShouldDrawElements = PUR_IS_BIT_ENABLED(purGLStateInGL.state, PUR_GL_DRAW_ELEMENTS);
	//purGLDrawElements = PUR_IS_BIT_ENABLED(purGLStateInGL, PUR_GL_DRAW_ELEMENTS);
	if (purGLShouldDrawElements && purGLIndexBuffer.size == 0)
		return;

	//Flush the buffer to gl
	purGLFlushBufferToGL();

	//If the max size is less then the current size, lets set the max size to
	//the current size... then reset the size to 0.
	if (purGLVertexBufferMaxSize < purGLVertexBuffer.size)
		purGLVertexBufferMaxSize = purGLVertexBuffer.size;

	purGLVertexBuffer.size = 0;
	purGLVertexBufferCurrentObject = purGLVertexBuffer.array;

	//If the max size is less then the current size, lets set the max size to
	//the current size... then reset the size to 0.
	if (purGLIndexBufferMaxSize < purGLIndexBuffer.size)
		purGLIndexBufferMaxSize = purGLIndexBuffer.size;

	purGLIndexBuffer.size = 0;
	purGLIndexBufferCurrentObject = purGLIndexBuffer.array;

	//If the max size is less then the current size, lets set the max size to
	//the current size... then reset the size to 0.
	if (purGLPointSizeBufferMaxSize < purGLPointSizeBuffer.size)
		purGLPointSizeBufferMaxSize = purGLPointSizeBuffer.size;

	purGLPointSizeBuffer.size = 0;
	purGLPointSizeBufferCurrentObject = purGLPointSizeBuffer.array;

	//Lets reset the colors to their max... aka white and visible.
	purGLBufferLastVertexRed   = 0xFF;
	purGLBufferLastVertexGreen = 0xFF;
	purGLBufferLastVertexBlue  = 0xFF;
	purGLBufferLastVertexAlpha = 0xFF;

	purGLBufferVertexColorState = PUR_GL_VERTEX_COLOR_RESET;

	if (purGLShouldDrawElements)
		purGLHadDrawnElements = true;
	else
		purGLHadDrawnArrays = true;
}

int purGLGetDrawCountThenResetIt()
{
#ifdef PUR_DEBUG_MODE
	int drawCount = purGLDrawCallCount;
	purGLDrawCallCount = 0;
	return drawCount;
#else
	return 0;
#endif
}
