
#ifndef _PUR_GL_STATE_PRIVATE_H_
#define _PUR_GL_STATE_PRIVATE_H_

/*	              STATE					*
 * EF0000000000SLPT - 16 bits, 6 used	*
 * ----------------------------------	*
 * E = Draw elements			1 bit	*
 * F = Shade Model Flat			1 bit	*
 * S = Point sprite				1 bit	*
 * L = Line smooth				1 bit	*
 * P = Point smooth				1 bit	*
 * T = Texture 2D				1 bit	*/
#define PUR_GL_DRAW_ELEMENTS			0x8000
#define PUR_GL_SHADE_MODEL_FLAT			0x4000
#define PUR_GL_POINT_SPRITE				0x0008
#define PUR_GL_LINE_SMOOTH				0x0004
#define PUR_GL_POINT_SMOOTH				0x0002
#define PUR_GL_TEXTURE_2D				0x0001

/*	          CLIENT STATE				*
 * 000000000000CPTV - 16 bits, 4 used	*
 * ----------------------------------	*
 * C = Color array				1 bit	*
 * P = Point Size array			1 bit	*
 * T = Texture coord array		1 bit	*
 * V = Vertex array				1 bit	*/
#define PUR_GL_COLOR_ARRAY				0x0008
#define PUR_GL_POINT_SIZE_ARRAY			0x0004
#define PUR_GL_TEXTURE_COORD_ARRAY		0x0002
#define PUR_GL_VERTEX_ARRAY				0x0001

#endif
