/*
 *  purGLRenderer.h
 *  pur
 *
 *  Created by John Lattin on 12/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef PUR_GL_RENDERER_H
#define PUR_GL_RENDERER_H

#include "purGL.h"
#include "purHeader.h"

#include "purGLState.h"

#define PUR_GL_VERTEX_COLOR_RESET 0
#define PUR_GL_VERTEX_COLOR_ONE 1
#define PUR_GL_VERTEX_COLOR_MULTIPLE 2

extern purGLState purGLDefaultState;
extern purGLState purGLCurState;
extern purGLState purGLStateInGL;

extern GLuint purGLBufferVertexColorState;

typedef struct
{
	purGLColoredTextureVertex *vertex;
	GLfloat *pointSize;
	GLuint vertexIndex;
} purGLElementBucket;

void purGLRendererInit();
void purGLRendererDealloc();

void purGLSetDrawMode(GLenum mode);
void purGLSetBufferLastVertexColor(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void purGLEnableColorArray();
void purGLDisableColorArray();

// TODO Later: Change these to use purArrayBuffer.
unsigned int purGLGetCurrentVertexIndex();
void purGLSetCurrentVertexIndex(unsigned int index);
unsigned int purGLGetCurrentIndex();
void purGLSetCurrentIndex(unsigned int index);
unsigned int purGLGetCurrentPointSizeIndex();
void purGLSetCurrentPointSizeIndex(unsigned int index);

//purGLColoredTextureVertex *purGLNextVertex();
purGLColoredTextureVertex *purGLGetVertexAt(unsigned int index);
purGLColoredTextureVertex *purGLCurrentVertex();
purGLColoredTextureVertex *purGLAskForVertices(unsigned int count);
void purGLUsedVertices(unsigned int count);

GLushort *purGLGetIndexAt(unsigned int index);
GLushort *purGLCurrentIndex();
GLushort *purGLAskForIndices(unsigned int count);
void purGLUsedIndices(unsigned int count);

//GLfloat *purGLNextPointSize();
GLfloat *purGLGetPointSizeAt(unsigned int index);
GLfloat *purGLCurrentPointSize();
GLfloat *purGLAskForPointSizes(unsigned int count);
void purGLUsedPointSizes(unsigned int count);

purGLElementBucket *purGLGetElementBuckets(unsigned int maxBucketVal);

void purGLRendererPreRender();
void purGLRendererPostRender();
void purGLConsolidateBuffer();

void purGLFlushBuffer();

void purGLSetupEnables();
int purGLGetDrawCountThenResetIt();

#endif
