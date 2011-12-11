
#ifndef _PUR_GL_PRIVATE_H_
#define _PUR_GL_PRIVATE_H_

#include "purGLInclude.h"

#include "purGLUtils.h"
#include "purGLState.h"

inkExtern void purGLInit(unsigned int width, unsigned int height, float scaleFactor);
inkExtern void purGLDealloc();

inkExtern void purGLFlush();

inkExtern void purGLSyncPurToGL();
inkExtern void purGLSyncGLToPur();

inkExtern void purGLSyncTransforms();
inkExtern void purGLUnSyncTransforms();

inkExtern void purGLPreRender();
inkExtern void purGLPostRender(bool resetCounter);
inkExtern void purGLConsolidateBuffers();

inkExtern void purGLResetStates(purGLState desiredState);

inkExtern void purGLClipRect(GLint x, GLint y, GLint width, GLint height);
inkExtern purGLAABB *purGLGetCurrentAABB();
inkExtern void purGLResetAABB(bool setToClipRect);
inkExtern bool purGLIsAABBVisible(purGLAABB *aabb);

inkExtern void purGLAABBMult(purGLAABB *aabb);

inkExtern void purGLSetViewSize(unsigned int width, unsigned int height, float scaleFactor, bool orientationEnabled);

inkExtern GLuint purGLFrameBuffer;
inkExtern GLuint purGLRenderBuffer;

#endif
