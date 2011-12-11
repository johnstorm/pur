#ifndef _PUR_GL_STATE_H_
#define _PUR_GL_STATE_H_

#include "purGLInclude.h"

#ifdef __cplusplus
extern "C" {
#endif

// NOTE:
//		The state is reset to your default, and upon a purGLDrawArrays or
//		purGLDrawElements call it compares the states. If they are different then
//		the batch is broken prior to setting the actual state in gl.

// NEVER set this variable directly, always use _purGLState... to change it!
// NEVER set this variable directly, always use _purGLState... to change it!
// NEVER set this variable directly, always use _purGLState... to change it!
typedef struct
{
	// @see BLEND - source
	GLushort blendSource;
	// @see BLEND - destination
	GLushort blendDestination;

	// For glBindTexture
	GLuint texture;

	// @see CLIENT STATE
	GLushort clientState;
	// @see STATE
	GLushort state;
} purGLState;

#ifdef __cplusplus
}
#endif

#endif
