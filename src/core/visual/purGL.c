#include "purGL.h"
#include "purGLPrivate.h"
#include "purGLRenderer.h"

#include "purPrivateUtils.h"
//#include "purDebugUtils.h"

#include "purGLUtils.h"
#include "purGLStatePrivate.h"

#include "inkGeometry.h"
#include "purColorTransform.h"

#include <limits.h>

GLuint purGLFrameBuffer = 0;
GLuint purGLRenderBuffer = 0;

#define _purGLMatrixStackSize 16
#define _purGLColorStackSize 16

inkInline GLuint purGLGLStateTopurState(GLenum cap);
inkInline GLenum purGLpurStateToGLState(GLuint cap);
inkInline GLuint purGLGLClientStateTopurClientState(GLenum array);
inkInline GLuint purGLpurClientStateToGLClientState(GLenum array);

inkMatrix purGLMatrices[_purGLMatrixStackSize];
purColorTransform purGLColors[_purGLColorStackSize];

inkMatrix *purGLCurMatrix = purGLMatrices;
purColorTransform *purGLCurrentColor = purGLColors;

unsigned short purGLCurrentMatrixIndex = 0;
unsigned short purGLCurrentColorIndex = 0;

#ifdef PUR_DEBUG_MODE
GLuint purGLRenderCallCount;
#endif

float purGLScaleFactor = 1.0f;
float purGLOne_ScaleFactor = 1.0f;

unsigned int purGLWidthInPoints  = 0;
unsigned int purGLHeightInPoints = 0;

typedef struct
{
	GLint size;
	GLenum type;
	GLsizei stride;
	const GLvoid *pointer;  //Weakly referenced
} _purGLArrayPointer;

_purGLRect purGLRectClip;
purGLAABB purGLCurAABB;

GLfloat purGLCurPointSize = 0.0f;
GLfloat purGLHalfPointSize = 0.0f;
GLfloat purGLCurLineWidth = 0.0f;

GLuint purGLTexture = 0;
GLuint purGLFramebuffer = 0;

_purGLArrayPointer purGLPointSizePointerVal;
_purGLArrayPointer purGLVertexPointerVal;
_purGLArrayPointer purGLColorPointerVal;
_purGLArrayPointer purGLTexCoordPointerVal;

GLfloat purUploadMatrix[16] =
{
	1.0f, 0.0f, 0.0f, 0.0f,
	0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f,
	0.0f, 0.0f, 0.0f, 1.0f
};

GLubyte purGLRed   = 0xFF;
GLubyte purGLGreen = 0xFF;
GLubyte purGLBlue  = 0xFF;
GLubyte purGLAlpha = 0xFF;

/*
 * This method initializes GL with the width and height given
 *
 * @param width The width of the screen.
 * @param height The height of the screen.
 */
void purGLInit(unsigned int width, unsigned int height, float scaleFactor)
{
	glGenFramebuffersOES(1, &purGLFrameBuffer);
	glGenRenderbuffersOES(1, &purGLRenderBuffer);

	glBindFramebufferOES(GL_FRAMEBUFFER_OES, purGLFrameBuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, purGLRenderBuffer);
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, purGLRenderBuffer);

	glBindFramebufferOES(GL_FRAMEBUFFER_OES, purGLFrameBuffer);

	purGLClipRect(0, 0, width, height);

	purGLPointSizePointerVal.pointer = NULL;
	purGLVertexPointerVal.pointer = NULL;
	purGLColorPointerVal.pointer = NULL;
	purGLTexCoordPointerVal.pointer = NULL;

	purGLSetViewSize(width, height, scaleFactor, true);

	purGLDefaultState.blendSource = GL_SRC_ALPHA;
	purGLDefaultState.blendDestination = GL_ONE_MINUS_SRC_ALPHA;
	
	glBlendFunc(purGLDefaultState.blendSource, purGLDefaultState.blendDestination);
	// TODO:	This function should be used to make rendering to texture work
	//			better. It doesn't really make a difference when just rendering
	//			to the screen. So, glBlendFunc should be replaced by
	//			glBlendFuncSeparateOES everywhere. The problem is that
	//			glBlendFuncSeparateOES is only available in iOS 3.1 and above.
	//			So we need to check if (glBlendFuncSeparateOES != NULL) before
	//			calling it. Otherwise, glBlendFunc could just be used.

	// NOTE:	Could use glGetString(GL_EXTENSIONS) to figure out which
	//			extensions are available.
	//glBlendFuncSeparateOES(purGLDefaultState.blendSource, purGLDefaultState.blendDestination, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	// Set defaults
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_POINT_SMOOTH);

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_POINT_SIZE_ARRAY_OES);

	// Always enabled
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnable(GL_BLEND);

	PUR_ENABLE_BIT(purGLDefaultState.clientState, PUR_GL_VERTEX_ARRAY);

	purGLCurState = purGLDefaultState;
	purGLStateInGL = purGLDefaultState;

	// Lets load the matrix identity, and color transform identity
	purGLLoadIdentity();
	purGLLoadColorTransformIdentity();

	// Lets intialize the renderer
	purGLRendererInit();

	// and sync up with gl
	purGLSyncPurToGL();

	// Lets initialize the color to white
	purGLRed   = 0xFF;
	purGLGreen = 0xFF;
	purGLBlue  = 0xFF;
	purGLAlpha = 0xFF;

	purGLCurrentColor->redMultiplier   = 1.0f;
	purGLCurrentColor->greenMultiplier = 1.0f;
	purGLCurrentColor->blueMultiplier  = 1.0f;
	purGLCurrentColor->alphaMultiplier = 1.0f;

	// then reset the aabb
	purGLResetAABB(false);
}

/*
 * This method frees any of the memory we were using, and releases the render
 * to texture.. texture.
 */
void purGLDealloc()
{
	purGLRendererDealloc();

	glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
	glBindRenderbufferOES(GL_RENDERBUFFER_BINDING_OES, 0);

	glDeleteRenderbuffersOES(1, &purGLRenderBuffer);
	purGLRenderBuffer = 0;

	glDeleteFramebuffersOES(1, &purGLFrameBuffer);
	purGLFrameBuffer = 0;
}

/*
 * This method flushes the buffer, it is a method in purgl so that the engine
 * can use it also rather then just purgl.
 */
void purGLFlush()
{
	purGLFlushBuffer();
}

/*
 * This method returns the texture id for the render to texture buffer.
 *
 * @return - The texture id for the render to texture buffer.
 */
/*
GLuint purGLGetTextureBuffer()
{
	return purGLRTTFBO;
}
*/

void purGLSyncPurToGL()
{
	GLushort changed = false;
	GLshort bVal = 0;

	GLint nVal;
	GLfloat fVals[4];

	//Check texture
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &nVal);
	if (purGLTexture != nVal)
	{
		changed = true;
		purGLTexture = nVal;
	}

	//Check color
	glGetFloatv(GL_CURRENT_COLOR, fVals);
	bVal = PUR_COLOR_BYTE_TO_FLOAT(fVals[0]);
	if (purGLRed != bVal)
	{
		changed = true; purGLRed = bVal;
	}

	bVal = PUR_COLOR_BYTE_TO_FLOAT(fVals[1]);
	if (purGLGreen != bVal)
	{
		changed = true; purGLGreen = bVal;
	}

	bVal = PUR_COLOR_BYTE_TO_FLOAT(fVals[2]);
	if (purGLBlue != bVal)
	{
		changed = true; purGLBlue = bVal;
	}

	bVal = PUR_COLOR_BYTE_TO_FLOAT(fVals[3]);
	if (purGLAlpha != bVal)
	{
		changed = true; purGLAlpha = bVal;
	}

	// Check line width
	glGetFloatv(GL_LINE_WIDTH, fVals);
	if (purGLCurLineWidth != fVals[0])
	{
		changed = true;
		purGLCurLineWidth = fVals[0];
	}

	// Check point size
	glGetFloatv(GL_POINT_SIZE, fVals);
	if (purGLCurPointSize != fVals[0])
	{
		changed = true;
		purGLCurPointSize = fVals[0];
		purGLHalfPointSize = purGLCurPointSize * 0.5f;
	}

	// Check color type, reason for doing this is that we don't want anyone to
	// start batching with the wrong type.
	if (!changed)
	{
		// Keep this in the if statement so we might not have to do it.
		glGetIntegerv(GL_COLOR_ARRAY_TYPE, &nVal);
		if (purGLColorPointerVal.type != nVal)
			changed = true;
	}

	// Check color type, reason for doing this is that we don't want anyone to
	// start batching with the wrong type.
	if (!changed)
	{
		//Keep this in the if statement so we might not have to do it.
		glGetIntegerv(GL_VERTEX_ARRAY_TYPE, &nVal);
		if (purGLVertexPointerVal.type != nVal)
			changed = true;
	}

	// Check color type, reason for doing this is that we don't want anyone to
	// start batching with the wrong type.
	if (!changed)
	{
		// Keep this in the if statement so we might not have to do it.
		glGetIntegerv(GL_TEXTURE_COORD_ARRAY_TYPE, &nVal);
		if (purGLTexCoordPointerVal.type != nVal)
			changed = true;
	}

	// Check color type, reason for doing this is that we don't want anyone to
	// start batching with the wrong type.
	if (!changed)
	{
		// Keep this in the if statement so we might not have to do it.
		glGetIntegerv(GL_POINT_SIZE_ARRAY_TYPE_OES, &nVal);
		if (purGLPointSizePointerVal.type != nVal)
			changed = true;
	}

	// We need to check the texture parameters now...

	glGetIntegerv(GL_FRAMEBUFFER_BINDING_OES, &nVal);
	if (purGLFramebuffer != nVal)
	{
		purGLFramebuffer = nVal;
		changed = true;
	}

	// If any of our values have changed, then we should flush the buffer
	if (changed)
		purGLFlushBuffer();
}

/*
 * This method syncs up a server side state; this will set the gl state to
 * whatever state we are currently using.
 *
 * @param GLenum cap - The gl server state you wish to sync with.
 */
void purGLSyncState(GLenum cap)
{
	GLuint state = purGLGLStateTopurState(cap);

	if (PUR_IS_BIT_ENABLED(purGLStateInGL.state, state))
		glEnable(cap);
	else
		glDisable(cap);
}

/*
 * This method syncs up a client side state; this will set the gl state to
 * whatever state we are currently using.
 *
 * @param GLenum cap - The gl client state you wish to sync with.
 */
void purGLSyncClientState(GLenum array)
{
	GLuint state = purGLGLClientStateTopurClientState(array);

	if (PUR_IS_BIT_ENABLED(purGLStateInGL.clientState, state))
		glEnableClientState(array);
	else
		glDisableClientState(array);
}

/*
 * This method synchronizes each of the gl states with our states; meaning if
 * we are using GL_TEXTURE_2D, then we will turn that on in GL.  This does both
 * client and server states.
 */
void purGLSyncGLToPur()
{
	// Lets make sure vertex array is on... we do wish to draw stuff after all.
	glEnableClientState(GL_VERTEX_ARRAY);

	// Lets synchronize the rest of our states,
	purGLSyncState(GL_POINT_SPRITE_OES);
	purGLSyncState(GL_TEXTURE_2D);
	purGLSyncState(GL_POINT_SMOOTH);
	purGLSyncState(GL_LINE_SMOOTH);
	purGLSyncClientState(GL_TEXTURE_COORD_ARRAY);
	purGLSyncClientState(GL_POINT_SIZE_ARRAY_OES);

	// Bind the texture, color, line width and point size we are currently using,
	glBindTexture(GL_TEXTURE_2D, purGLTexture);
	glColor4ub(purGLRed, purGLGreen, purGLBlue, purGLAlpha);
	glLineWidth(purGLCurLineWidth);
	glPointSize(purGLCurPointSize);

	glBindFramebufferOES(GL_FRAMEBUFFER_OES, purGLFramebuffer);

	// and enable the color array.
	purGLEnableColorArray();
	glEnableClientState(GL_COLOR_ARRAY);

	if (PUR_IS_BIT_ENABLED(purGLStateInGL.state, PUR_GL_SHADE_MODEL_FLAT))
		glShadeModel(GL_FLAT);
	else
		glShadeModel(GL_SMOOTH);
}

void purGLSyncTransforms()
{
	glPushMatrix();
	purGLLoadMatrixToGL();

	glColor4ub(purGLRed, purGLGreen, purGLBlue, purGLAlpha);
}
void purGLUnSyncTransforms()
{
	// We don't do anything with the color, as the engine takes care of that
	// upon rendering.

	// Pops the matrix which was pushed in sync transforms above.
	glPopMatrix();
}

/*
 * This method prepairs both purGL and GL for rendering.
 */
void purGLPreRender()
{
	// Lets reset the color transform, and matrix stacks... so they are reaady
	// to be used by another render cycle.
	purGLResetColorTransformStack();
	purGLResetMatrixStack();

	glPushMatrix();
	glLoadIdentity();
	//glTranslatef(100.0f, -0.0f, 0.0f);
	purGLRendererPreRender();
}

/*
 * This method finishes both purGL and GL rendering cycle (flushing the buffer,
 * etc.).
 */
void purGLPostRender(bool resetCounter)
{
	purGLRendererPostRender();
	glPopMatrix();

#ifdef PUR_DEBUG_MODE
	if (resetCounter == true)
	{
		int count = purGLGetDrawCountThenResetIt();

		if (purDebugIsEnabled(purDebugSetting_CountGLCalls))
		{
			purGLRenderCallCount = count;
		}
	}
#endif
}

/*
 * This method consolidates the buffers; meaning it reduces the buffers down to
 * reasonable sizes if they are overly large and the data they are containing
 * is small.
 */
void purGLConsolidateBuffers()
{
	purGLConsolidateBuffer();
}

GLfloat purGLGetContentScaleFactor()
{
	return purGLScaleFactor;
}
GLfloat purGLGetOneOverContentScaleFactor()
{
	return purGLOne_ScaleFactor;
}
GLuint purGLDBGGetRenderCallCount()
{
#ifdef PUR_DEBUG_MODE
	if (purDebugIsEnabled(purDebugSetting_CountGLCalls))
	{
		return purGLRenderCallCount;
	}
#endif

	return 0;
}

/*
 * purGLBindFramebuffer lets you create or use a named framebuffer object.
 * Calling purGLBindFramebuffer with target set to GL_FRAMEBUFFER and
 * framebuffer set to the name of the new framebuffer object binds the
 * framebuffer object name.  When a framebuffer object is bound, the previous
 * binding is automatically broken.
 *
 * purGLBindFramebuffer does a check to see if the buffer you are binding has
 * the same name as the one that is currently bound, if it is then it does not
 * change the buffer; this is done to help stop redundant gl state changes.
 *
 * @param GLenum target - Specifies the target to which the framebuffer object
 * is bound.  The symbolic constraint must be GL_FRAMEBUFFER.
 * @param GLuint framebuffer - Specifies the name of a framebuffer object.
 */
void purGLBindFramebuffer(GLenum target, GLuint framebuffer)
{
	if (target != GL_FRAMEBUFFER_OES || purGLFramebuffer == framebuffer)
		return;

	purGLFlushBuffer();

	purGLFramebuffer = framebuffer;
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, purGLFramebuffer);
}

/*
 * purGLClipRect sets a bounding area where objects can be drawn.  If an object
 * has any point within this area, it will be drawn, however if all of it's
 * points are outside, then it will not be.  This does not mean that the object
 * itself will be clipped, therefore if an object has only 1 point inside, all
 * of the other points (even outside of the clipped area) will still be drawn.
 *
 * @param GLint x - The x starting position of the clipping area.
 * @param GLint y - The y starting position of the clipping area.
 * @param GLint width - The width of the clipping area.
 * @param GLint height - The height of the clipping area.
 */
void purGLClipRect(GLint x, GLint y, GLint width, GLint height)
{
	purGLRectClip.x = x;
	purGLRectClip.y = y;
	purGLRectClip.width = width;
	purGLRectClip.height = height;
}

/*
 * purGLGetCurrentAABB returns a pointer to a axis-aligned bounding box that
 * defines an object the object that was most recently drawn.
 *
 * @return purGLAABB * - A pointer to the axis-aligned bounding box that
 * represents the object that was most recently drawn.
 */
purGLAABB *purGLGetCurrentAABB()
{
	return &purGLCurAABB;
}

/*
 * purGLResetAABB resets the current axis-aligned bounding box to the max and
 * min values, thus ready to be modified.
 */
void purGLResetAABB(bool setToClipRect)
{
	if (setToClipRect)
	{
		purGLCurAABB.xMin = purGLRectClip.x;
		purGLCurAABB.yMin = purGLRectClip.y;
		purGLCurAABB.xMax = purGLRectClip.x + purGLRectClip.width;
		purGLCurAABB.yMax = purGLRectClip.y + purGLRectClip.height;
	}
	else
	{
		purGLCurAABB.xMin = INT_MAX;
		purGLCurAABB.yMin = INT_MAX;
		purGLCurAABB.xMax = INT_MIN;
		purGLCurAABB.yMax = INT_MIN;
	}
}

/*
 * purGLIsAABBVisible returns true when the axis-aligned bounding box given is
 * within the clip rect defined by purGLClipRect.
 *
 * @param purGLAABB * aabb - The axis-aligned bounding box to be checked.
 *
 * @return bool - true if any portion of the axis-aligned bounding box is
 * within the clipping rectangle.
 */
bool purGLIsAABBVisible(purGLAABB *aabb)
{
	if (aabb->xMin > (purGLRectClip.x + purGLRectClip.width))
		return false;

	if (aabb->yMin > (purGLRectClip.y + purGLRectClip.height))
		return false;

	if (aabb->xMax < purGLRectClip.x)
		return false;

	if (aabb->yMax < purGLRectClip.y)
		return false;

	return true;
}

/*
 * purGLBoundTexture returns the currently bound texture to gl.
 *
 * @return GLuint - The currently bound texture to gl.
 */
GLuint purGLBoundTexture()
{
	return purGLTexture;
}

/*
 * purGLBindTexture lets you create or use a named texture. Calling
 * purGLBindTexture with target set to GL_TEXTURE_2D, and texture set to the
 * name of the new texture binds the texture name to the target. When a texture
 * is bound to a target, the previous binding for that target is automatically
 * broken.
 *
 * If the texture you are binding is already bound or the target is not
 * GL_TEXTURE_2D, then this method just returns.
 *
 * @param GLenum target - Specifies the target to which the texture is bound.
 * Must be GL_TEXTURE_2D.
 * @param GLuint texture - Specifies the name of a texture.
 */
void purGLBindTexture(GLenum target, GLuint texture)
{
	if (target != GL_TEXTURE_2D || purGLTexture == texture)
		return;

	purGLFlushBuffer();
	purGLTexture = texture;
	glBindTexture(target, texture);
}

/*
 * purGLColor4f sets a new four-valued current RGBA color.  Current color values
 * are stored in bytes, thus it is more efficent to call purGLColor4ub.
 *
 * purGLColor4f does a check to see if the color you are setting has is the same
 * as the one that is currently set, if it is then it does not set the color;
 * this is done to help stop redundant gl state changes.
 *
 * @param GLfloat red   - The red value for the current color.
 * @param GLfloat green - The green value for the current color.
 * @param GLfloat blue  - The blue value for the current color.
 * @param GLfloat alpha - The alpha value for the current color.
 */
void purGLColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	purGLColor4ub(PUR_COLOR_FLOAT_TO_BYTE(red),
	             PUR_COLOR_FLOAT_TO_BYTE(green),
	             PUR_COLOR_FLOAT_TO_BYTE(blue),
	             PUR_COLOR_FLOAT_TO_BYTE(alpha));
}

/*
 * purGLColor4ub sets a new four-valued current RGBA color.  Current color values
 * are stored in bytes.
 *
 * purGLColor4ub does a check to see if the color you are setting has is the same
 * as the one that is currently set, if it is then it does not set the color;
 * this is done to help stop redundant gl state changes.
 *
 * @param GLfloat red   - The red value for the current color.
 * @param GLfloat green - The green value for the current color.
 * @param GLfloat blue  - The blue value for the current color.
 * @param GLfloat alpha - The alpha value for the current color.
 */
void purGLColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
	// Lets convert the color to inherit properties from the parents.
	red   *= purGLCurrentColor->redMultiplier;
	green *= purGLCurrentColor->greenMultiplier;
	blue  *= purGLCurrentColor->blueMultiplier;
	alpha *= purGLCurrentColor->alphaMultiplier;

	// If we are already using this color, then lets just return as no change to
	// gl needs to occur.
	if (red == purGLRed && green == purGLGreen && blue == purGLBlue && alpha == purGLAlpha)
		return;

	// If we are not usiong our color method, then lets break the buffer as we
	// actually need to change gl.

	purGLRed   = red;
	purGLGreen = green;
	purGLBlue  = blue;
	purGLAlpha = alpha;
}

/*
 * This method sets the gl state, if that state is not already on.  This is a
 * pre check before sending the data to the hardware so that unnecessary
 * synchronization doesn't occur as we handle all of the states locally.
 *
 * purGLEnable does a check to see if the GL capability you are enabeling is
 * currently enabled, if it is then it does not enable the state again; this is
 * done to help stop redundant gl state changes.
 *
 * @param GLenum cap - Specifies a symbolic constant indicating a GL capability
 */
void purGLEnable(GLenum cap)
{
	GLuint state = purGLGLStateTopurState(cap);

	PUR_ENABLE_BIT(purGLCurState.state, state);
}

/*
 * This method sets the gl client state, if that state is not already on.  This
 * is a pre check before sending the data to the hardware so that unnecessary
 * synchronization doesn't occur as we handle all of the states locally.
 *
 * purGLEnableClientState does a check to see if the GL capability you are
 * enabeling is currently enabled, if it is then it does not enable the state
 * again; this is done to help stop redundant gl state changes.
 *
 * @param GLenum array - Specifies the capability to enable or disable.
 * Symbolic constants GL_COLOR_ARRAY, GL_POINT_SIZE_ARRAY_OES,
 * GL_TEXTURE_COORD_ARRAY are accepted.
 */
void purGLEnableClientState(GLenum array)
{
	// If we are using our color array method, then we don't actually want to
	// enable the color array here, we just wish to know that the user wants to;
	// thus we are also not breaking the batch.

	// Lets convert the client state from gl to ours, so we can check it
	// properly
	GLuint state = purGLGLClientStateTopurClientState(array);

	PUR_ENABLE_BIT(purGLCurState.clientState, state);
}

/*
 * This method disables the gl state, if that state is not already disabled.
 * This is a pre check before sending the data to the hardware so that
 * unnecessary synchronization doesn't occur as we handle all of the states
 * locally.
 *
 * purGLDisable does a check to see if the GL capability you are dosabling is
 * currently disabled, if it is then it does not disable the state again;
 * this is done to help stop redundant gl state changes.
 *
 * @param GLenum cap - Specifies a symbolic constant indicating a GL capability
 */
void purGLDisable(GLenum cap)
{
	// Lets convert the client state from gl to ours, so we can check it
	// properly
	GLuint state = purGLGLStateTopurState(cap);

	PUR_DISABLE_BIT(purGLCurState.state, state);
}

/*
 * This method disables the gl client state, if that state is not already
 * disabled.  This is a pre check before sending the data to the hardware so
 * that unnecessary synchronization doesn't occur as we handle all of the
 * states locally.
 *
 * purGLDisableClientState does a check to see if the GL capability you are
 * disabling is currently disabled, if it is then it does not disable the state
 * again; this is done to help stop redundant gl state changes.
 *
 * @param GLenum cap - Specifies a symbolic constant indicating a GL capability
 */
void purGLDisableClientState(GLenum array)
{
	// If we are using our color array method, then we don't actually want to
	// disable the color array here, we just wish to know that the user wants
	// to; thus we are also not breaking the batch.

	// Lets convert the client state from gl to ours, so we can check it
	// properly
	GLuint state = purGLGLClientStateTopurClientState(array);

	PUR_DISABLE_BIT(purGLCurState.clientState, state);
}

/*
 * Texture mapping is a technique that applies an image onto an object's
 * surface as if the image were a decal or cellophane shrink-wrap.  The image
 * is created in texture space, with an (s, t) coordinate system. A texture is
 * a one- or two-dimensional image and a set of parameters that determine how
 * samples are derived from the image.
 *
 * purGLTexParameter assigns the value in param or params to the texture
 * parameter specified as pname.  target defines the target texture, which must
 * be GL_TEXTURE_2D.
 *
 * @param GLenum target Specifies the target texture, which must be
 * GL_TEXTURE_2D.
 * @param GLenum pname Specifies the symbolic name of a single-valued texture
 * parameter. Which can be one of the following: GL_TEXTURE_MIN_FILTER,
 * GL_TEXTURE_MAG_FILTER, GL_TEXTURE_WRAP_S, or GL_TEXTURE_WRAP_T.
 * @param GLint param Specifies the value of pname.
 */
void purGLTexParameteri(GLenum target, GLenum pname, GLint param)
{
	// If the value has changed, we need to flush the buffer before changing it.
	purGLFlushBuffer();

	// then update gl.
	glTexParameteri(target, pname, param);
}

/*
 * purGLLineWidth specifies the rasterized width of both aliased and antialiased
 * lines. Using a line width other than 1 has different effects, depending on
 * whether line antialiasing is enabled. To enable and disable line
 * antialiasing, call purGLEnable and purGLDisable with argument GL_LINE_SMOOTH.
 * Line antialiasing is initially disabled.
 *
 * purGLLineWidth does a check to see if the width you are setting is currently
 * set, if it is then it does not set the width again; this is done to help
 * stop redundant gl calls.
 *
 * @param GLfloat width - Specifies the width of rasterized lines. The initial
 * value is 1.  If 0 is specified, the width is set to one pixel (independent
 * of scale factor).
 */
void purGLLineWidth(GLfloat width)
{
	if (width == 0.0f)
		width = 1.0f / purGLScaleFactor;
	else
		width *= purGLScaleFactor;

	// If the line width is already equal to the width given, then we do not
	// need to set it in gl.
	if (purGLCurLineWidth == width)
		return;

	// Lets flush the buffer, as we do not know what is yet to come, and need to
	// have the buffer use the current gl state rather then the chagned one.
	purGLFlushBuffer();
	purGLCurLineWidth = width;

	// Lets actually change the gl state.
	glLineWidth(width);
}

/*
 * purGLPointSize specifies the rasterized diameter of both aliased and
 * antialiased points. Using a point size other than 1 has different effects,
 * depending on whether point antialiasing is enabled. To enable and disable
 * point antialiasing, call purGLEnable and purGLDisable with argument
 * GL_POINT_SMOOTH. Point antialiasing is initially disabled.
 *
 * purGLPointSize does a check to see if the size you are setting is currently
 * set, if it is then it does not set the size again; this is done to help stop
 * redundant gl calls.
 *
 * @param GLfloat size - Specifies the diameter of rasterized points. The
 * initial value is 1.
 */
void purGLPointSize(GLfloat size)
{
	size *= purGLScaleFactor;

	if (purGLCurPointSize == size)
		return;

	// Lets flush the buffer, as we do not know what is yet to come, and need to
	// have the buffer use the current gl state rather then the chagned one.
	purGLFlushBuffer();
	purGLCurPointSize = size;
	purGLHalfPointSize = purGLCurPointSize * 0.5f;

	// Lets actually change the gl state.
	glPointSize(size);
}

/*
 * purGLColorPointer specifies the location and data of an array of color
 * components to use when rendering.  size specifies the number of components
 * per color, and must be 4.  type specifies the data type of each color
 * component, and stride specifies the byte stride from one color to the next
 * allowing vertices and attributes to be packed into a single array or stored
 * in separate arrays. (Single-array storage may be more efficient on some
 * implementations).
 *
 * @param GLint size - Specifies the number of coordinates per array element.
 * Must be 4.
 * @param GLenum type - Specifies the data type of each texture coordinate.
 * Must be GL_UNSIGNED_BYTE.
 * @param GLsizei stride - Specifies the byte offset between consecutive array
 * elements. If stride is 0, the array elements are understood to be tightly
 * packed. The initial value is 0.
 * @param GLvoid * pointer - Specifies a pointer to the first coordinate of the
 * first element in the array. The initial value is 0.
 */
void purGLColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	assert(type == GL_UNSIGNED_BYTE);
	// We actually store this because we need to manipulate the data, such as
	// batching and transforming.

	if (stride == 0)
		stride = sizeof(GLubyte) * size;

	purGLColorPointerVal.size = size;
	purGLColorPointerVal.type = type;
	purGLColorPointerVal.stride = stride;
	purGLColorPointerVal.pointer = pointer;
}

/*
 * purGLPointSizePointer specifies the location and data of an array of point
 * sizes to use when rendering points. type specifies the data type of the
 * coordinates. stride specifies the byte stride from one point size to the
 * next, allowing vertices and attributes to be packed into a single array or
 * stored in separate arrays. (Single-array storage may be more efficient on
 * some implementations).
 *
 * @param GLint size - Specifies the number of coordinates per array element.
 * Must be 2.
 * @param GLenum type - Specifies the data type of each texture coordinate.
 * Must be GL_FLOAT.
 * @param GLsizei stride - Specifies the byte offset between consecutive array
 * elements. If stride is 0, the array elements are understood to be tightly
 * packed. The initial value is 0.
 * @param GLvoid * pointer - Specifies a pointer to the first coordinate of the
 * first element in the array. The initial value is 0.
 */
void purGLPointSizePointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	assert(type == GL_FLOAT);
	// We actually store this because we need to manipulate the data, such as
	// batching and transforming.

	if (stride == 0)
		stride = sizeof(GLfloat);

	purGLPointSizePointerVal.type = type;
	purGLPointSizePointerVal.stride = stride;
	purGLPointSizePointerVal.pointer = pointer;
}

/*
 * purGLTexCoordPointer specifies the location and data of an array of texture
 * coordinates to use when rendering.  size specifies the number of coordinates
 * per element, and must be 2.  type specifies the data type of each texture
 * coordinate and stride specifies the byte stride from one array element to
 * the next allowing vertices and attributes to be packed into a single array
 * or stored in separate arrays. (Single-array storage may be more efficient on
 * some implementations).
 *
 * @param GLint size - Specifies the number of coordinates per array element.
 * Must be 2.
 * @param GLenum type - Specifies the data type of each texture coordinate.
 * Must be GL_FLOAT.
 * @param GLsizei stride - Specifies the byte offset between consecutive array
 * elements. If stride is 0, the array elements are understood to be tightly
 * packed. The initial value is 0.
 * @param GLvoid * pointer - Specifies a pointer to the first coordinate of the
 * first element in the array. The initial value is 0.
 */
void purGLTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	assert(type == GL_FLOAT);

	// We actually store this because we need to manipulate the data, such as
	// batching and transforming.

	if (stride == 0)
		stride = sizeof(GLfloat) * size;

	// Lets copy the values over
	purGLTexCoordPointerVal.size = size;
	purGLTexCoordPointerVal.type = type;
	purGLTexCoordPointerVal.stride = stride;
	purGLTexCoordPointerVal.pointer = pointer;
}

/*
 * purGLVertexPointer specifies the location and data of an array of vertex
 * coordinates to use when rendering. size specifies the number of coordinates
 * per vertex and type the data type of the coordinates. stride specifies the
 * byte stride from one vertex to the next allowing vertices and attributes to
 * be packed into a single array or stored in separate arrays. (Single-array
 * storage may be more efficient on some implementations).
 *
 * @param GLint size - Specifies the number of coordinates per array element.
 * Must be 2.
 * @param GLenum type - Specifies the data type of each texture coordinate.
 * Must be GL_FLOAT.
 * @param GLsizei stride - Specifies the byte offset between consecutive array
 * elements. If stride is 0, the array elements are understood to be tightly
 * packed. The initial value is 0.
 * @param GLvoid * pointer - Specifies a pointer to the first coordinate of the
 * first element in the array. The initial value is 0.
 */
void purGLVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	assert(type == GL_FLOAT);
	// We actually store this because we need to manipulate the data, such as
	// batching and transforming.

	if (stride == 0)
		stride = sizeof(GLfloat) * size;

	purGLVertexPointerVal.size = size;
	purGLVertexPointerVal.type = type;
	purGLVertexPointerVal.stride = stride;
	purGLVertexPointerVal.pointer = pointer;
}

void purGLShadeModel(GLenum mode)
{
	// TODO: When using this function to turn shade mode to flat, it doesn't
	// work.

	// If you are asking for flat
	if (mode == GL_FLAT)
	{
		PUR_ENABLE_BIT(purGLCurState.state, PUR_GL_SHADE_MODEL_FLAT);
	}
	else
	{
		PUR_DISABLE_BIT(purGLCurState.state, PUR_GL_SHADE_MODEL_FLAT);
	}
}

void purGLGetBooleanv(GLenum pname, GLboolean *params)
{
	if (params == NULL)
		return;

	purGLFlush();

	glGetBooleanv(pname, params);
}

void purGLGetFloatv(GLenum pname, GLfloat *params)
{
	if (params == NULL)
		return;

	purGLFlush();

	glGetFloatv(pname, params);
}

void purGLGetIntegerv(GLenum pname, GLint *params)
{
	if (params == NULL)
		return;

	purGLFlush();

	glGetIntegerv(pname, params);
}

void purGLGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
	if (params == NULL)
		return;

	purGLFlush();

	glGetTexParameteriv(target, pname, params);
}

void purGLTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
	purGLFlush();

	glTexEnvf(target, pname, param);
}
void purGLTexEnvi(GLenum target, GLenum pname, GLint param)
{
	purGLFlush();

	glTexEnvi(target, pname, param);
}
void purGLTexEnvx(GLenum target, GLenum pname, GLfixed param)
{
	purGLFlush();

	glTexEnvx(target, pname, param);
}
void purGLTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
	purGLFlush();

	glTexEnvfv(target, pname, params);
}
void purGLTexEnviv(GLenum target, GLenum pname, const GLint *params)
{
	purGLFlush();

	glTexEnviv(target, pname, params);
}
void purGLTexEnvxv(GLenum target, GLenum pname, const GLfixed *params)
{
	purGLFlush();

	glTexEnvxv(target, pname, params);
}

inkInline void purGLDefineVertex(purGLColoredTextureVertex *point,
							   GLfloat *pointSize,
							   const GLfloat **verticesPtr,
							   const GLfloat **texCoordsPtr,
							   const GLubyte **colorsPtr,
							   const GLfloat **pointSizesPtr,
							   float a, float b, float c, float d, float tx, float ty)
{
	float x = **verticesPtr; ++(*verticesPtr);
	float y = **verticesPtr;

	// Do the matrix multiplication on them.
	point->x = x * a + y * c + tx;
	point->y = x * b + y * d + ty;

	// If it is textured we need to grab the texture info
	if (*texCoordsPtr)
	{
		point->s = **texCoordsPtr; ++(*texCoordsPtr);
		point->t = **texCoordsPtr;
	}

	// If it is colored we need to grab the color info
	if (*colorsPtr)
	{
#if (!PUR_ACCURATE_COLOR_TRANSFORMATION_MODE)
		// If we are going to round the color value, we are going to use this
		// method.
		point->r = ((**colorsPtr) * purGLRed)   >> 8; ++(*colorsPtr);
		point->g = ((**colorsPtr) * purGLGreen) >> 8; ++(*colorsPtr);
		point->b = ((**colorsPtr) * purGLBlue)  >> 8; ++(*colorsPtr);
		point->a = ((**colorsPtr) * purGLAlpha) >> 8;
#else
		// If we want accurate info, then we will use this method.
		point->r = ((**colorsPtr) * purGLCurrentColor->redMultiplier);   ++(*colorsPtr);
		point->g = ((**colorsPtr) * purGLCurrentColor->greenMultiplier); ++(*colorsPtr);
		point->b = ((**colorsPtr) * purGLCurrentColor->blueMultiplier);  ++(*colorsPtr);
		point->a = ((**colorsPtr) * purGLCurrentColor->alphaMultiplier);
#endif
	}
	else
	{
		// If we weren't colored, and are using our special color array method,
		// then we have to set the values for the array.
		point->r = purGLRed;
		point->g = purGLGreen;
		point->b = purGLBlue;
		point->a = purGLAlpha;
	}

	// If we haven't had multiple values for colors yet, then we have to check
	// if this addition will make it so.
	if (purGLBufferVertexColorState != PUR_GL_VERTEX_COLOR_MULTIPLE)
		purGLSetBufferLastVertexColor(point->r, point->g, point->b, point->a);

	if (*pointSizesPtr)
	{
		*pointSize = **pointSizesPtr * purGLScaleFactor;
	}
}

/*
 * When purGLDrawArrays is called, it uses count sequential elements from each
 * enabled array to construct a sequence of geometric primitives, beginning
 * with element first. mode specifies what kind of primitives are constructed,
 * and how the array elements construct those primitives. If GL_VERTEX_ARRAY
 * is not enabled, no geometric primitives are generated.
 *
 * purGLDrawArrays batches similar draw calls together prior to sending the
 * information to gl.
 *
 * @param GLenum mode - Specifies what kind of primitives to render. Symbolic
 * constants GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES,
 * GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, and GL_TRIANGLES are accepted.
 * @param GLint first - Specifies the starting index in the enabled arrays.
 * @param GLsizei count - Specifies the number of indices to be rendered.
 */
void purGLDrawArrays(GLenum mode, GLint first, GLsizei count)
{
	// If our pointer is empty, then lets just return.
	if (!purGLVertexPointerVal.pointer || count == 0) //|| purGLCurrentColor->alphaMultiplier < 0.001f )
		return;

	PUR_DISABLE_BIT(purGLCurState.state, PUR_GL_DRAW_ELEMENTS);
	purGLSetupEnables();

	// Lets change the draw mode.
	purGLSetDrawMode(mode);

	// This variable is for tracking the current point we are manipulating.
	purGLColoredTextureVertex *point;
	GLfloat *pointSize;

	// Lets store the strides of each of these so we do not need to access them
	// again.
	GLsizei vertexStride = purGLVertexPointerVal.stride;
	GLsizei texStride = purGLTexCoordPointerVal.stride;
	GLsizei colorStride = purGLColorPointerVal.stride;
	GLsizei pointSizeStride = purGLPointSizePointerVal.stride;

	// What was the vertex index before we added points.
	unsigned int oldVertexIndex = purGLGetCurrentVertexIndex();
	unsigned int oldPointSizeIndex = purGLGetCurrentPointSizeIndex();

	bool isTextured = PUR_IS_BIT_ENABLED(purGLCurState.clientState, PUR_GL_TEXTURE_COORD_ARRAY);
	bool isColored = PUR_IS_BIT_ENABLED(purGLCurState.clientState, PUR_GL_COLOR_ARRAY);
	bool isPointSizeArray = PUR_IS_BIT_ENABLED(purGLCurState.clientState, PUR_GL_POINT_SIZE_ARRAY) && mode == GL_POINTS;
	bool isStrip = (mode == GL_TRIANGLE_STRIP);

	// Lets set the pointer to the starting point of the vertices, texture
	// coords, colors and point sizes; however we only want to grab the pointer
	// info if it is applicable.
	const GLfloat *vertices = purGLVertexPointerVal.pointer + first * vertexStride;
	const void *currentVertex = vertices;
	const GLfloat *texCoords = isTextured ? purGLTexCoordPointerVal.pointer + first * texStride : NULL;
	const void *currentTexCoord = texCoords;
	const GLubyte *colors = isColored ? purGLColorPointerVal.pointer + first * colorStride : NULL;
	const void *currentColor = colors;
	const GLfloat *pointSizes = isPointSizeArray ? purGLPointSizePointerVal.pointer + first * pointSizeStride : NULL;
	const void *currentPointSize = pointSizes;

	float a = purGLCurMatrix->a;
	float b = purGLCurMatrix->b;
	float c = purGLCurMatrix->c;
	float d = purGLCurMatrix->d;
	float tx = purGLCurMatrix->tx;
	float ty = purGLCurMatrix->ty;

	// If the old vertex is 0, then it is the first vertex being used... thus we
	// do not need to add points at the start if we were going to.
	if (oldVertexIndex == 0)
	{
		isStrip = false;
	}

	// This is set up for the bounding box of the item being drawn.
	purGLAABB aabb = purGLAABBReset;

	signed int nX;
	signed int nY;

	unsigned int usedPointCount = isStrip ? count + 2 : count;
	// Grab an array of vertices
	point = purGLAskForVertices(usedPointCount);

	// For strips
	purGLColoredTextureVertex *preFirstPoint;
	purGLColoredTextureVertex *firstPoint;

	if (isStrip)
	{
		// If it is a strip, copy the last point (this won't happen if this is
		// the first object ever in this array)
		*point = *(point - 1);
		++point;

		// We will also want to copy our first point, so we are setting up
		// pointers to get that ready. The preFirstPoint is a pointer to the
		// value prior to the first value we are going to manipulate. The
		// firstPoint is the first point we will read and manipulate. After we
		// manipulate the point, we will copy it back into preFirstPoint. This
		// will create a degenerate triangle that gl will optimize out.
		preFirstPoint = point;
		++point;
		firstPoint = point;
	}

	if (isPointSizeArray)
	{
		pointSize = purGLAskForPointSizes(count);
	}
	else
		pointSize = NULL;

//	GLfloat halfPointSize;

	for (GLsizei index = 0; index < count; ++index, ++point)
	{
		purGLDefineVertex(point,
						 pointSize,
						 &vertices,
						 &texCoords,
						 &colors,
						 &pointSizes,
						 a, b, c, d, tx, ty);
		nX = point->x;
		nY = point->y;

		vertices = currentVertex + vertexStride;
		currentVertex = vertices;

		if (isTextured)
		{
			texCoords = currentTexCoord + texStride;
			currentTexCoord = texCoords;
		}
		if (isColored)
		{
			// No matter what, we need to increment the pointer from the current
			// beginning point.
			colors = currentColor + colorStride;
			currentColor = colors;
		}
		if (isPointSizeArray)
		{
		//	halfPointSize = *pointSize * 0.5f;

			++pointSize;

			pointSizes = currentPointSize + pointSizeStride;
			currentPointSize = pointSizes;

		//	purGLAABBExpandv(&aabb, nX - halfPointSize, nY - halfPointSize);
		//	purGLAABBExpandv(&aabb, nX + halfPointSize, nY + halfPointSize);
		}
		/*else if (mode == GL_POINTS)
		{
			purGLAABBExpandv(&aabb, nX - purGLHalfPointSize, nY - purGLHalfPointSize);
			purGLAABBExpandv(&aabb, nX + purGLHalfPointSize, nY + purGLHalfPointSize);
		}
		else
		{*/
			// Lets figure out the bounding box
			purGLAABBExpandv(&aabb, nX, nY);
		//}
	}

	if (isStrip)
	{
		*preFirstPoint = *firstPoint;
	}

	purGLUsedVertices(usedPointCount);
	if (isPointSizeArray)
	{
		purGLUsedPointSizes(count);
	}

	// Updates the points and colors based upon the parents rotation and
	// position and yours, also updates bounding box of the display object!
	// pointer, start , end

	// Check to see if the bounding area is within the screen area, if not then
	// we don't need to draw it.  To resolve this, lets set the index back to
	// the old one, this way it negates us adding more on.

	if (!_purGLRectContainsAABB(&purGLRectClip, &aabb))
	{
		purGLSetCurrentPointSizeIndex(oldPointSizeIndex);
		purGLSetCurrentVertexIndex(oldVertexIndex);
		if (isPointSizeArray)
		{
			purGLSetCurrentPointSizeIndex(oldPointSizeIndex);
		}
	}

	// We are adding a 1 pixel buffer to the bounding box
	purGLAABBInflatev(&aabb, 1, 1);

	// then we are going to calculate the overall bounding box for the object
	// that was drawn.
	purGLAABBUpdate(&purGLCurAABB, &aabb);
}

/*
 * When purGLDrawElements is called, it uses count sequential elements from each
 * enabled array to construct a sequence of geometric primitives, beginning
 * with element first. mode specifies what kind of primitives are constructed,
 * and how the array elements construct those primitives. If GL_VERTEX_ARRAY
 * is not enabled, no geometric primitives are generated.
 *
 * purGLDrawElements batches similar draw calls together prior to sending the
 * information to gl.
 *
 * @param GLenum mode - Specifies what kind of primitives to render. Symbolic
 * constants GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES,
 * GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, and GL_TRIANGLES are accepted.
 * @param GLsizei count - Specifies the number of elements to be rendered.
 * @param GLenum type - Specifies the type of the values in indices. Must be
 * GL_UNSIGNED_SHORT.
 * @param const GLvoid * ids - Specifies a pointer to the location where the
 * indices are stored.
 */
void purGLDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *ids)
{
	if (!purGLVertexPointerVal.pointer || count == 0)
		return;

	const GLushort *indices = ids;

	PUR_ENABLE_BIT(purGLCurState.state, PUR_GL_DRAW_ELEMENTS);
	purGLSetupEnables();

	purGLSetDrawMode(mode);

	purGLColoredTextureVertex *point;
	GLushort *index;
	GLfloat *pointSize;

	GLsizei vertexStride = purGLVertexPointerVal.stride;
	GLsizei texStride = purGLTexCoordPointerVal.stride;
	GLsizei colorStride = purGLColorPointerVal.stride;
	GLsizei pointSizeStride = purGLPointSizePointerVal.stride;

	bool isTextured = PUR_IS_BIT_ENABLED(purGLCurState.clientState, PUR_GL_TEXTURE_COORD_ARRAY);
	bool isColored = PUR_IS_BIT_ENABLED(purGLCurState.clientState, PUR_GL_COLOR_ARRAY);
	bool isPointSizeArray = PUR_IS_BIT_ENABLED(purGLCurState.clientState, PUR_GL_POINT_SIZE_ARRAY) && mode == GL_POINTS;
	bool isStrip = (mode == GL_TRIANGLE_STRIP);

	const void const *startVertex = purGLVertexPointerVal.pointer;
	const GLfloat *vertices;

	const void const *startTex = isTextured ? purGLTexCoordPointerVal.pointer : NULL;
	const GLfloat *texCoords;

	const void const *startColor = isColored ? purGLColorPointerVal.pointer : NULL;
	const GLubyte *colors;

	const void const *startPointSizes = isPointSizeArray ? purGLPointSizePointerVal.pointer : NULL;
	const GLfloat *pointSizes;

	GLuint eVal = 0; // HAS TO BE 'unsigned int SHORT' OR LARGER

	float a = purGLCurMatrix->a;
	float b = purGLCurMatrix->b;
	float c = purGLCurMatrix->c;
	float d = purGLCurMatrix->d;
	float tx = purGLCurMatrix->tx;
	float ty = purGLCurMatrix->ty;

	unsigned int vertexIndex = 0;

	unsigned int oldIndex = purGLGetCurrentIndex();
	unsigned int oldVertexIndex = purGLGetCurrentVertexIndex();
	unsigned int oldPointSizeIndex = purGLGetCurrentPointSizeIndex();

	vertexIndex = oldVertexIndex;

	if (oldIndex == 0)
	{
		isStrip = false;
	}

	unsigned int usedIndexCount = isStrip ? count + 2 : count;
	unsigned int usedVertexCount = 0;

	// Grab a sequencial array of indices and vertices
	index = purGLAskForIndices(usedIndexCount);
	point = purGLAskForVertices(count);

	// For strips
	GLushort *preFirstIndex;
	GLushort *firstIndex;

	if (isStrip)
	{
		// If it is a strip, copy the last index (this won't happen if this is
		// the first object ever in this array)
		*index = *(index - 1);
		++index;

		preFirstIndex = index;
		++index;
		firstIndex = index;
	}

	if (isPointSizeArray)
	{
		pointSize = purGLAskForPointSizes(count);
	}
	else
		pointSize = NULL;

//	GLfloat halfPointSize;

	// These values are used for creating a bounding box for the object drawn.

	purGLAABB aabb = purGLAABBReset;

	int nX;
	int nY;

	const GLushort *curIndex;
	GLsizei counter;

	// Create an arbitrary amount of buckets. We will expand this if needed.
	GLushort maxIndex = (count * 0.5f) + 1;//*indices;
	/*for (counter = 1, curIndex = indices + counter; counter < count; ++counter, ++curIndex)
	{
		if (maxIndex < *curIndex)
			maxIndex = *curIndex;
	}*/

	purGLElementBucket *buckets = purGLGetElementBuckets(maxIndex + 1);
	purGLElementBucket *bucket;

	for (counter = 0, curIndex = indices + counter; counter < count; ++counter, ++curIndex, ++index)
	{
		// Get the next available point, this method needs to also change the
		// size of the array accordingly. If the array ever gets larger then
		// MAX_VERTICES, we should flush it. Keep in mind that if we do that
		// here, then offset and translation needs to change also.

		eVal = *curIndex;

		if (eVal > maxIndex)
		{
			maxIndex = eVal;
			// Grabbing the new array of buckets.
			// Note:	This will not mess up the previous array as it will be
			//			copied by realloc. We are also always getting the bucket
			//			from the starting array (aka this) so it will never be
			//			incorrect.
			buckets = purGLGetElementBuckets(maxIndex + 1);
		}

		bucket = buckets + eVal;

		if (!(bucket->vertex))
		{
			++usedVertexCount;

			bucket->vertex = point;
			++point;
			if (isPointSizeArray)
			{
				bucket->pointSize = pointSize;
				++pointSize;
			}
			bucket->vertexIndex = vertexIndex;
			++vertexIndex;

			// Lets grab the next vertex, which is done by getting the actual
			// index value of the vertex held by eVal, then we can multiply it
			// by the stride to find the pointers location.
			vertices = startVertex + eVal * vertexStride;

			if (isTextured)
				texCoords = startTex + eVal * texStride;
			else
				texCoords = NULL;

			if (isColored)
				colors = startColor + eVal * colorStride;
			else
				colors = NULL;

			if (isPointSizeArray)
				pointSizes = startPointSizes + eVal * pointSizeStride;
			else
				pointSizes = NULL;

			purGLDefineVertex(bucket->vertex,
							 bucket->pointSize,
							 &vertices,
							 &texCoords,
							 &colors,
							 &pointSizes,
							 a, b, c, d, tx, ty);

			nX = bucket->vertex->x;
			nY = bucket->vertex->y;

			/*if (isPointSizeArray)
			{
				halfPointSize = *(bucket->pointSize) * 0.5f;

				purGLAABBExpandv(&aabb, nX - halfPointSize, nY - halfPointSize);
				purGLAABBExpandv(&aabb, nX + halfPointSize, nY + halfPointSize);
			}
			else if (mode == GL_POINTS)
			{
				purGLAABBExpandv(&aabb, nX - purGLHalfPointSize, nY - purGLHalfPointSize);
				purGLAABBExpandv(&aabb, nX + purGLHalfPointSize, nY + purGLHalfPointSize);
			}
			else
			{*/
				// Lets figure out the bounding box
				purGLAABBExpandv(&aabb, nX, nY);
			//}
		}

		*index = bucket->vertexIndex;
	}

	if (isStrip)
	{
		*preFirstIndex = *firstIndex;
	}

	purGLUsedIndices(usedIndexCount);
	purGLUsedVertices(usedVertexCount);

	if (isPointSizeArray)
	{
		purGLUsedPointSizes(usedVertexCount);
	}

	// Updates the points and colors based upon the parents rotation and
	// position and yours, also updates bounding box of the display object!
	// pointer, start, end

	// Check to see if the bounding area is within the screen area, if not then
	// we don't need to draw it. To resolve this, lets set the index back to the
	// old one, this way it negates us adding more on.
	if (!_purGLRectContainsAABB(&purGLRectClip, &aabb))
	{
		purGLSetCurrentIndex(oldIndex);
		purGLSetCurrentPointSizeIndex(oldPointSizeIndex);
		purGLSetCurrentVertexIndex(oldVertexIndex);

		if (isPointSizeArray)
		{
			purGLSetCurrentPointSizeIndex(oldPointSizeIndex);
		}
	}

	// We are adding a 1 pixel buffer to the bounding box
	purGLAABBInflatev(&aabb, 1, 1);

	// then we are going to calculate the overall bounding box for the object
	// that was drawn.
	purGLAABBUpdate(&purGLCurAABB, &aabb);
}

void purGLBlendFunc(GLenum sfactor, GLenum dfactor)
{
	purGLCurState.blendSource = sfactor;
	purGLCurState.blendDestination = dfactor;
}

/*
 * purGLPopMatrix pops the current matrix stack, replacing the current matrix
 * with the one below it on the stack.
 */
void purGLPopMatrix()
{
	//purDebugLog(@"purGLPopMatrix has failed: There is no matrix to pop.");
	assert(purGLCurrentMatrixIndex);
	
	purGLCurMatrix = &purGLMatrices[--purGLCurrentMatrixIndex];
}

/*
 * purGLPushMatrix pushes the current matrix stack down by one, duplicating the
 * current matrix. That is, after a purGLPushMatrix call, the matrix on top of
 * the stack is identical to the one below it.
 */
void purGLPushMatrix()
{
	assert(purGLCurrentMatrixIndex < _purGLMatrixStackSize - 1);

	inkMatrix *oldMatrix = purGLCurMatrix;
	purGLCurMatrix = &purGLMatrices[++purGLCurrentMatrixIndex];

	purGLCurMatrix->a = oldMatrix->a;
	purGLCurMatrix->b = oldMatrix->b;
	purGLCurMatrix->c = oldMatrix->c;
	purGLCurMatrix->d = oldMatrix->d;
	purGLCurMatrix->tx = oldMatrix->tx;
	purGLCurMatrix->ty = oldMatrix->ty;
}

/*
 * purGLLoadIdentity replaces the current matrix with the identity matrix.
 */
void purGLLoadIdentity()
{
	*purGLCurMatrix = inkMatrixIdentity;
}

/*
 * purGLTranslate translates the current matrix by the given values.
 *
 * @param GLfloat x - The x coordinate of the translation vector.
 * @param GLfloat y - The y coordinate of the translation vector.
 */
void purGLTranslate(GLfloat x, GLfloat y)
{
	purGLCurMatrix->tx += x;
	purGLCurMatrix->ty += y;
}

/*
 * purGLScale produces a nonuniform scaling along the x and y axes.
 *
 * @param GLfloat x - Scaling factor along the x axis.
 * @param GLfloat y - Scaling factor along the y axis.
 */
void purGLScale(GLfloat x, GLfloat y)
{
	// Multiply the matrix by the scaling factors
	purGLCurMatrix->a *= x;
	purGLCurMatrix->d *= y;
	purGLCurMatrix->tx *= x;
	purGLCurMatrix->ty *= y;
}

/*
 * purGLRotate produces a rotation matrix of angle degrees. The current matrix is
 * multiplied by the rotation matrix with the product replacing the current
 * matrix.
 *
 * @param GLfloat angle - Specifies the angle of rotation, in degrees.
 */
void purGLRotate(GLfloat angle)
{
	GLfloat sinVal = sinf(angle);
	GLfloat cosVal = cosf(angle);

	GLfloat a = purGLCurMatrix->a;
	GLfloat b = purGLCurMatrix->b;
	GLfloat c = purGLCurMatrix->c;
	GLfloat d = purGLCurMatrix->d;
	GLfloat tx = purGLCurMatrix->tx;
	GLfloat ty = purGLCurMatrix->ty;

	purGLCurMatrix->a = a * cosVal - b * sinVal;
	purGLCurMatrix->b = a * sinVal + b * cosVal;
	purGLCurMatrix->c = c * cosVal - d * sinVal;
	purGLCurMatrix->d = c * sinVal + d * cosVal;
	purGLCurMatrix->tx = tx * cosVal - ty * sinVal;
	purGLCurMatrix->ty = tx * sinVal + ty * cosVal;
}

/*
 * purGLMultMatrix multplies the current matrix by the matrix given.
 *
 * @param inkMatrix * mat - The matrix to be multiplied with.
 */
void purGLMultMatrix(inkMatrix mat)
{
	*purGLCurMatrix = inkMatrixMultiply(*purGLCurMatrix, mat);
}

/*
 * purGLMultMatrix multplies the current matrix by the matrix given.
 *
 * @param inkMatrix * mat - The matrix to be multiplied with.
 */
void purGLAABBMult(purGLAABB *aabb)
{
	inkMatrixConvertAABBv(*purGLCurMatrix,
						  &(aabb->xMin), &(aabb->yMin),
						  &(aabb->xMax), &(aabb->yMax));
}

/*
 * purGLResetMatrixStack resets the matrix stack back to the first matrix, and
 * sets it to the identity.
 */
void purGLResetMatrixStack()
{
	purGLCurrentMatrixIndex = 0;
	purGLCurMatrix = purGLMatrices;
	purGLLoadIdentity();
}

inkMatrix purGLCurrentMatrix()
{
	inkMatrix mat;

	if (purGLCurrentMatrix)
		mat = *purGLCurMatrix;
	else
		mat = inkMatrixIdentity;

	return mat;
}

/*
 * This method loads our matrix into gl.
 */
void purGLLoadMatrixToGL()
{
	purUploadMatrix[0] = purGLCurMatrix->a;
	purUploadMatrix[1] = purGLCurMatrix->b;
	purUploadMatrix[4] = purGLCurMatrix->c;
	purUploadMatrix[5] = purGLCurMatrix->d;
	purUploadMatrix[12] = purGLCurMatrix->tx;
	purUploadMatrix[13] = purGLCurMatrix->ty;

	glLoadMatrixf(purUploadMatrix);
}

/*
 * purGLPopColorTransform pops the color transform stack, replacing the current
 * color transform with the one below it on the stack.
 */
void purGLPopColorTransform()
{
	assert(purGLCurrentColorIndex);

	purGLCurrentColor = &purGLColors[--purGLCurrentColorIndex];

	GLubyte red   = (float)0xFF * purGLCurrentColor->redMultiplier;
	GLubyte green = (float)0xFF * purGLCurrentColor->greenMultiplier;
	GLubyte blue  = (float)0xFF * purGLCurrentColor->blueMultiplier;
	GLubyte alpha = (float)0xFF * purGLCurrentColor->alphaMultiplier;

	// If popping the transform leaves the colors the same as they were, then we
	// don't need to go any further.
	if (red == purGLRed && green == purGLGreen && blue == purGLBlue && alpha == purGLAlpha)
		return;

	// Set the current color
	purGLRed   = red;
	purGLGreen = green;
	purGLBlue  = blue;
	purGLAlpha = alpha;
}

/*
 * purGLPushColorTransform pushes the current transform stack down by one,
 * duplicating the current transform. That is, after a purGLPushMatrix call, the
 * transform on top of the stack is identical to the one below it.
 */
void purGLPushColorTransform()
{
	//purDebugLog(@"purGLPushColor has failed: Reached color transform capacity.");
	assert(purGLCurrentColorIndex < _purGLColorStackSize - 1);

	purColorTransform *purOldColor = purGLCurrentColor;
	purGLCurrentColor = &purGLColors[++purGLCurrentColorIndex];

	purGLCurrentColor->redMultiplier   = purOldColor->redMultiplier;
	purGLCurrentColor->greenMultiplier = purOldColor->greenMultiplier;
	purGLCurrentColor->blueMultiplier  = purOldColor->blueMultiplier;
	purGLCurrentColor->alphaMultiplier = purOldColor->alphaMultiplier;

	purGLRed   = PUR_COLOR_FLOAT_TO_BYTE(purGLCurrentColor->redMultiplier);
	purGLGreen = PUR_COLOR_FLOAT_TO_BYTE(purGLCurrentColor->greenMultiplier);
	purGLBlue  = PUR_COLOR_FLOAT_TO_BYTE(purGLCurrentColor->blueMultiplier);
	purGLAlpha = PUR_COLOR_FLOAT_TO_BYTE(purGLCurrentColor->alphaMultiplier);
}

/*
 * purGLSetColorTransform sets the current transform to the one provided
 * multiplied by the parent color transform (if one exists).
 *
 * @param purColorTransform * transform - The transform you wish to set.
 */
void purGLSetColorTransform(purColorTransform transform)
{
	if (purGLCurrentColorIndex != 0)
	{
		purColorTransform *purOldColor  = &purGLColors[purGLCurrentColorIndex - 1];
		purGLCurrentColor->redMultiplier   = purOldColor->redMultiplier;
		purGLCurrentColor->greenMultiplier = purOldColor->greenMultiplier;
		purGLCurrentColor->blueMultiplier  = purOldColor->blueMultiplier;
		purGLCurrentColor->alphaMultiplier = purOldColor->alphaMultiplier;
	}

	purGLCurrentColor->redMultiplier   *= transform.redMultiplier;
	purGLCurrentColor->greenMultiplier *= transform.greenMultiplier;
	purGLCurrentColor->blueMultiplier  *= transform.blueMultiplier;
	purGLCurrentColor->alphaMultiplier *= transform.alphaMultiplier;

	purGLRed   = PUR_COLOR_FLOAT_TO_BYTE(purGLCurrentColor->redMultiplier  );
	purGLGreen = PUR_COLOR_FLOAT_TO_BYTE(purGLCurrentColor->greenMultiplier);
	purGLBlue  = PUR_COLOR_FLOAT_TO_BYTE(purGLCurrentColor->blueMultiplier );
	purGLAlpha = PUR_COLOR_FLOAT_TO_BYTE(purGLCurrentColor->alphaMultiplier);
}

/*
 * purGLLoadColorTransformIdentity sets the current color's transform to the
 * identity (multiplied by the parent if one exists).
 */
void purGLLoadColorTransformIdentity()
{
	if (purGLCurrentColorIndex != 0)
	{
		purColorTransform *purOldColor = &purGLColors[purGLCurrentColorIndex - 1];

		purGLCurrentColor->redMultiplier = purOldColor->redMultiplier;
		purGLCurrentColor->greenMultiplier = purOldColor->greenMultiplier;
		purGLCurrentColor->blueMultiplier = purOldColor->blueMultiplier;
		purGLCurrentColor->alphaMultiplier = purOldColor->alphaMultiplier;

		purGLRed   = PUR_COLOR_FLOAT_TO_BYTE(purGLCurrentColor->redMultiplier  );
		purGLGreen = PUR_COLOR_FLOAT_TO_BYTE(purGLCurrentColor->greenMultiplier);
		purGLBlue  = PUR_COLOR_FLOAT_TO_BYTE(purGLCurrentColor->blueMultiplier );
		purGLAlpha = PUR_COLOR_FLOAT_TO_BYTE(purGLCurrentColor->alphaMultiplier);
	}
	else
		*purGLCurrentColor = purColorTransformIdentity;
}

/*
 * purGLResetColorTransformStack resets the transform stack back to the first
 * transform, and sets it to the identity.
 */
void purGLResetColorTransformStack()
{
	purGLCurrentColorIndex = 0;
	purGLCurrentColor = purGLColors;
	purGLLoadColorTransformIdentity();
	purGLColor4ub(0xFF, 0xFF, 0xFF, 0xFF);
}

/*inkMatrix inkMatrixTransform(inkMatrix mat, GLfloat angle, GLfloat scaleX, GLfloat scaleY, GLfloat x, GLfloat y)
{
	// Needs to exist
	assert(mat);

	mat = inkMatrixScale(mat, scaleX, scaleY);
	mat = inkMatrixRotate(mat, angle);
	mat = inkMatrixTranslate(mat, x, y);

	return mat;
}
*/
void purGLResetStates(purGLState desiredState)
{
	//purGLState = purGLDefaultState;
	purGLCurState = desiredState;
}

void purGLSetupEnables()
{
	bool breakBatch = false;
//	bool clientStateNotEqual = purGLClientState != purGLClientStateInGL;
//	bool stateNotEqual = purGLState != purGLStateInGL;
//	bool blendModeNotEqual = purGLBlendMode.asUInt != purGLBlendModeInGL.asUInt;

	bool clientStateNotEqual = purGLCurState.clientState != purGLStateInGL.clientState;
	bool stateNotEqual = purGLCurState.state != purGLStateInGL.state;
	bool blendModeNotEqual = ((purGLCurState.blendSource != purGLStateInGL.blendSource) ||
							  (purGLCurState.blendDestination != purGLStateInGL.blendDestination));

	if (clientStateNotEqual)
	{
		breakBatch = true;

		// TODO: Generate a more intelligent check for this
		if (PUR_IS_BIT_ENABLED_IN_BOTH(purGLCurState.clientState, purGLStateInGL.clientState, PUR_GL_COLOR_ARRAY) == false &&
			PUR_IS_BIT_ENABLED_IN_BOTH(purGLCurState.clientState, purGLStateInGL.clientState, GL_VERTEX_ARRAY) == true &&
			PUR_IS_BIT_ENABLED_IN_BOTH(purGLCurState.clientState, purGLStateInGL.clientState, GL_TEXTURE_COORD_ARRAY) == true)
		{
			breakBatch = false;
		}
	}

	if (stateNotEqual)
	{
		// Draw elements vs draw arrays is auto taken care of by this check, 
		breakBatch = true;
	}

	if (blendModeNotEqual)
	{
		breakBatch = true;
	}

	if (breakBatch)
	{
		purGLFlushBuffer();

#define purGLCompareAndSetClientState(_PUR_state_, _gl_state_) \
{ \
	if (!PUR_IS_BIT_ENABLED_IN_BOTH(purGLCurState.clientState, purGLStateInGL.clientState, _PUR_state_)) \
	{ \
		if (PUR_IS_BIT_ENABLED(purGLCurState.clientState, _PUR_state_)) \
			glEnableClientState(_gl_state_); \
		else \
			glDisableClientState(_gl_state_); \
	} \
}
#define purGLCompareAndSetState(_PUR_state_, _gl_state_) \
{ \
	if (!PUR_IS_BIT_ENABLED_IN_BOTH(purGLCurState.state, purGLStateInGL.state, _PUR_state_)) \
	{ \
		if (PUR_IS_BIT_ENABLED(purGLCurState.state, _PUR_state_)) \
			glEnable(_gl_state_); \
		else \
			glDisable(_gl_state_); \
	} \
}

		purGLCompareAndSetClientState(PUR_GL_POINT_SIZE_ARRAY, GL_POINT_SIZE_ARRAY_OES);
		purGLCompareAndSetClientState(PUR_GL_TEXTURE_COORD_ARRAY, GL_TEXTURE_COORD_ARRAY);
		purGLCompareAndSetClientState(PUR_GL_VERTEX_ARRAY, GL_VERTEX_ARRAY);

		purGLCompareAndSetState(PUR_GL_POINT_SPRITE, GL_POINT_SPRITE_OES);
		purGLCompareAndSetState(PUR_GL_LINE_SMOOTH, GL_LINE_SMOOTH);
		purGLCompareAndSetState(PUR_GL_POINT_SMOOTH, GL_POINT_SMOOTH);
		purGLCompareAndSetState(PUR_GL_TEXTURE_2D, GL_TEXTURE_2D);

		/*if (PUR_IS_BIT_ENABLED_IN_BOTH(purGLState, purGLStateInGL, PUR_GL_SHADE_MODEL_FLAT))
		{
			if (PUR_IS_BIT_ENABLED(purGLState, PUR_GL_SHADE_MODEL_FLAT))
				glShadeModel(GL_FLAT);
			else
				glShadeModel(GL_SMOOTH);
		}*/

		if (PUR_IS_BIT_ENABLED_IN_BOTH(purGLCurState.state, purGLStateInGL.state, PUR_GL_SHADE_MODEL_FLAT))
		{
			if (PUR_IS_BIT_ENABLED(purGLCurState.state, PUR_GL_SHADE_MODEL_FLAT))
				glShadeModel(GL_FLAT);
			else
				glShadeModel(GL_SMOOTH);
		}

		if (blendModeNotEqual)
		{
			glBlendFunc(purGLCurState.blendSource, purGLCurState.blendDestination);
			
			// glBlendFuncSeparateOES(purGLCurState.blendSource, purGLCurState.blendDestination,
			//					   GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		}
	}

	purGLStateInGL = purGLCurState;
}

purGLState _purGLDefaultState()
{
	return purGLDefaultState;
}

void _purGLStateEnable(purGLState *state, GLenum cap)
{
	assert(state); // Must exist
	PUR_ENABLE_BIT(state->state, purGLGLStateTopurState(cap));
}
void _purGLStateDisable(purGLState *state, GLenum cap)
{
	assert(state); // Must exist
	PUR_DISABLE_BIT(state->state, purGLGLStateTopurState(cap));
}
void _purGLStateEnableClientState(purGLState *state, GLenum array)
{
	assert(state); // Must exist
	PUR_ENABLE_BIT(state->clientState, purGLGLClientStateTopurClientState(array));
}
void _purGLStateDisableClientState(purGLState *state, GLenum array)
{
	assert(state); // Must exist
	PUR_DISABLE_BIT(state->clientState, purGLGLClientStateTopurClientState(array));
}
//void _purGLStateBindTexture(purGLState *state, GLuint texture)
//{
//	assert(state);
//	state->texture = texture;
//}
void _purGLStateBlendFunc(purGLState *state, GLenum sfactor, GLenum dfactor)
{
	assert(state); // Must exist

	state->blendSource = sfactor;
	state->blendDestination = dfactor;
}

bool _purGLStateIsEnabled(purGLState *state, GLenum cap)
{
	assert(state); // Must exist

	switch (cap)
	{
		// CLIENT STATE
		case GL_COLOR_ARRAY:
			return PUR_IS_BIT_ENABLED(state->clientState, PUR_GL_COLOR_ARRAY);
		case GL_POINT_SIZE_ARRAY_OES:
			return PUR_IS_BIT_ENABLED(state->clientState, PUR_GL_POINT_SIZE_ARRAY);
		case GL_TEXTURE_COORD_ARRAY:
			return PUR_IS_BIT_ENABLED(state->clientState, PUR_GL_TEXTURE_COORD_ARRAY);
		case GL_VERTEX_ARRAY:
			return PUR_IS_BIT_ENABLED(state->clientState, PUR_GL_VERTEX_ARRAY);
		// STATE
		case GL_POINT_SPRITE_OES:
			return PUR_IS_BIT_ENABLED(state->state, PUR_GL_POINT_SPRITE);
		case GL_LINE_SMOOTH:
			return PUR_IS_BIT_ENABLED(state->state, PUR_GL_LINE_SMOOTH);
		case GL_POINT_SMOOTH:
			return PUR_IS_BIT_ENABLED(state->state, PUR_GL_POINT_SMOOTH);
		case GL_TEXTURE_2D:
			return PUR_IS_BIT_ENABLED(state->state, PUR_GL_TEXTURE_2D);
		default:
			return false;
	}

	return false;
}

void _purGLStateGetIntegerv(purGLState *state, GLenum pname, GLint *params)
{
	assert(state);	// Must exist
	assert(params);	// Must exist

	switch (pname)
	{
		case GL_BLEND_DST:
			*params = state->blendDestination;
			break;
		case GL_BLEND_SRC:
			*params = state->blendSource;
			break;
	}
}

inkInline GLuint purGLGLStateTopurState(GLenum cap)
{
	switch (cap)
	{
	case GL_POINT_SPRITE_OES:
		return PUR_GL_POINT_SPRITE;
	case GL_LINE_SMOOTH:
		return PUR_GL_LINE_SMOOTH;
	case GL_POINT_SMOOTH:
		return PUR_GL_POINT_SMOOTH;
	case GL_TEXTURE_2D:
		return PUR_GL_TEXTURE_2D;
	}

	return 0;
}

inkInline GLenum purGLpurStateToGLState(GLuint cap)
{
	switch (cap)
	{
	case PUR_GL_POINT_SPRITE:
		return GL_POINT_SPRITE_OES;
	case PUR_GL_LINE_SMOOTH:
		return GL_LINE_SMOOTH;
	case PUR_GL_POINT_SMOOTH:
		return GL_POINT_SMOOTH;
	case PUR_GL_TEXTURE_2D:
		return GL_TEXTURE_2D;
	}

	return 0;
}

inkInline GLuint purGLGLClientStateTopurClientState(GLenum array)
{
	switch (array)
	{
	case GL_COLOR_ARRAY:
		return PUR_GL_COLOR_ARRAY;
	case GL_POINT_SIZE_ARRAY_OES:
		return PUR_GL_POINT_SIZE_ARRAY;
	case GL_TEXTURE_COORD_ARRAY:
		return PUR_GL_TEXTURE_COORD_ARRAY;
	case GL_VERTEX_ARRAY:
		return PUR_GL_VERTEX_ARRAY;
	}

	return 0;
}

inkInline GLuint purGLpurClientStateToGLClientState(GLenum array)
{
	switch (array)
	{
	case PUR_GL_COLOR_ARRAY:
		return GL_COLOR_ARRAY;
	case PUR_GL_POINT_SIZE_ARRAY:
		return GL_POINT_SIZE_ARRAY_OES;
	case PUR_GL_TEXTURE_COORD_ARRAY:
		return GL_TEXTURE_COORD_ARRAY;
	case PUR_GL_VERTEX_ARRAY:
		return GL_VERTEX_ARRAY;
	}

	return 0;
}

inkInline GLuint purSizeOfGLEnum(GLenum type)
{
	switch (type)
	{
	case GL_BYTE:
		return sizeof(GLbyte);
	case GL_UNSIGNED_BYTE:
		return sizeof(GLubyte);
	case GL_SHORT:
		return sizeof(GLshort);
	case GL_UNSIGNED_SHORT:
		return sizeof(GLushort);
	case GL_FLOAT:
		return sizeof(GLfloat);
	}

	return 0;
}

// width and height passed in POINTS
// @param orientationEnabled
//		set to true when rendering to the screen. set to false when rendering
//		to an off-the-screen surface
void purGLSetViewSize(unsigned int width, unsigned int height, float scaleFactor, bool orientationEnabled)
{
	purGLScaleFactor = scaleFactor;
	purGLOne_ScaleFactor = 1.0f / purGLScaleFactor;
	purGLWidthInPoints  = width;
	purGLHeightInPoints = height;

	// in PIXELS
	glViewport(0.0f,									// x
			   0.0f,									// y
			   purGLWidthInPoints  * purGLScaleFactor,	// width
			   purGLHeightInPoints * purGLScaleFactor);	// height
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// in POINTS
	glOrthof(0,						// xMin
			 purGLWidthInPoints,		// xMax
			 purGLHeightInPoints,	// yMin
			 0,						// yMax
			 -100.0f,				// zMin
			  100.0f);				// zMax
	glMatrixMode(GL_MODELVIEW);
}
