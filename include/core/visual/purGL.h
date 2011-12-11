#ifndef _PUR_GL_H_
#define _PUR_GL_H_

#include "inkHeader.h"
#include "inkGeometry.h"

#include "purGLInclude.h"
#include "purGLUtils.h"
#include "purColorTransform.h"
#include "purGLState.h"

inkExtern GLfloat purGLGetContentScaleFactor();
inkExtern GLfloat purGLGetOneOverContentScaleFactor();
inkExtern GLuint purGLDBGGetRenderCallCount();

inkExtern void purGLBindFramebuffer(GLenum target, GLuint framebuffer);
inkExtern GLuint purGLBoundTexture();

inkExtern void purGLBindTexture(GLenum target, GLuint texture);
inkExtern void purGLColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
inkExtern void purGLColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
inkExtern void purGLColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
inkExtern void purGLDisable(GLenum cap);
inkExtern void purGLDisableClientState(GLenum array);
inkExtern void purGLDrawArrays(GLenum mode, GLint first, GLsizei count);
inkExtern void purGLDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
inkExtern void purGLEnable(GLenum cap);
inkExtern void purGLEnableClientState(GLenum array);
inkExtern void purGLLineWidth(GLfloat width);
inkExtern void purGLPointSize(GLfloat size);
inkExtern void purGLPointSizePointer(GLenum type, GLsizei stride, const GLvoid *pointer);
inkExtern void purGLPopMatrix();
inkExtern void purGLPushMatrix();
inkExtern void purGLTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
inkExtern void purGLTexParameteri(GLenum target, GLenum pname, GLint param);
inkExtern void purGLVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);

inkExtern void purGLGetBooleanv(GLenum pname, GLboolean *params);
inkExtern void purGLGetFloatv(GLenum pname, GLfloat *params);
inkExtern void purGLGetIntegerv(GLenum pname, GLint *params);
inkExtern void purGLGetTexParameteriv(GLenum target, GLenum pname, GLint *params);

inkExtern void purGLShadeModel(GLenum mode);

inkExtern void purGLTexEnvf(GLenum target, GLenum pname, GLfloat param);
inkExtern void purGLTexEnvi(GLenum target, GLenum pname, GLint param);
inkExtern void purGLTexEnvx(GLenum target, GLenum pname, GLfixed param);
inkExtern void purGLTexEnvfv(GLenum target, GLenum pname, const GLfloat *params);
inkExtern void purGLTexEnviv(GLenum target, GLenum pname, const GLint *params);
inkExtern void purGLTexEnvxv(GLenum target, GLenum pname, const GLfixed *params);

inkExtern void purGLBlendFunc(GLenum sfactor, GLenum dfactor);

inkExtern void purGLPopMatrix();
inkExtern void purGLPushMatrix();
inkExtern void purGLLoadIdentity();
inkExtern void purGLTranslate(GLfloat x, GLfloat y);
inkExtern void purGLScale(GLfloat x, GLfloat y);
inkExtern void purGLRotate(GLfloat angle);
inkExtern void purGLMultMatrix(inkMatrix mat);
inkExtern void purGLLoadMatrixToGL();
inkExtern void purGLResetMatrixStack();
inkExtern inkMatrix purGLCurrentMatrix();

inkExtern void purGLPopColorTransform();
inkExtern void purGLPushColorTransform();
inkExtern void purGLLoadColorTransformIdentity();
inkExtern void purGLResetColorTransformStack();
inkExtern void purGLSetColorTransform(purColorTransform transform);

inkExtern purGLState _purGLDefaultState();
inkExtern void _purGLStateEnable(purGLState *state, GLenum cap);
inkExtern void _purGLStateDisable(purGLState *state, GLenum cap);
inkExtern void _purGLStateEnableClientState(purGLState *state, GLenum array);
inkExtern void _purGLStateDisableClientState(purGLState *state, GLenum array);
inkExtern void _purGLStateBlendFunc(purGLState *state, GLenum sfactor, GLenum dfactor);
inkExtern bool _purGLStateIsEnabled(purGLState *state, GLenum cap);
inkExtern void _purGLStateGetIntegerv(purGLState *state, GLenum pname, GLint *params);

#endif
