/* Copyright (c) 2007 Scott Lembcke
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "chipmunk_private.h"
#include "chipmunk_unsafe.h"

#define CP_DefineShapeGetter(struct, type, member, name) \
CP_DeclareShapeGetter(struct, type, name){ \
	cpAssertHard(shape->klass == &struct##Class, "shape is not a "#struct); \
	return ((struct *)shape)->member; \
}

static cpHashValue cpShapeIDCounter = 0;

void
cpResetShapeIdCounter(void)
{
	cpShapeIDCounter = 0;
}


cpShape*
cpShapeInit(cpShape *shape, const cpShapeClass *klass, cpBody *body)
{
	shape->klass = klass;
	
	shape->hashid = cpShapeIDCounter;
	cpShapeIDCounter++;
	
	shape->body = body;
	shape->sensor = 0;
	
	shape->e = 0.0f;
	shape->u = 0.0f;
	shape->surface_v = cpvzero;
	
	shape->collision_type = 0;
	shape->group = CP_NO_GROUP;
	shape->layers = CP_ALL_LAYERS;
	
	shape->data = NULL;
	shape->next = NULL;
	shape->prev = NULL;
	
	return shape;
}

void
cpShapeDestroy(cpShape *shape)
{
	if(shape->klass && shape->klass->destroy) shape->klass->destroy(shape);
}

void
cpShapeFree(cpShape *shape)
{
	if(shape){
		cpShapeDestroy(shape);
		cpfree(shape);
	}
}

void
cpShapeSetBody(cpShape *shape, cpBody *body)
{
	cpAssertHard(!cpShapeActive(shape), "You cannot change the body on an active shape. You must remove the shape, then ");
	shape->body = body;
}

cpBB
cpShapeCacheBB(cpShape *shape)
{
	cpBody *body = shape->body;
	return cpShapeUpdate(shape, body->p, body->rot);
}

cpBB
cpShapeUpdate(cpShape *shape, cpVect pos, cpVect rot)
{
	return (shape->bb = shape->klass->cacheData(shape, pos, rot));
}

cpBool
cpShapePointQueryExtended(cpShape *shape, cpVect p, cpPointQueryExtendedInfo *info){
	cpPointQueryExtendedInfo blank = {NULL, 0.0f, cpvzero};
	if(info){(*info) = blank;} else {info = &blank;}
	
	shape->klass->pointQuery(shape, p, info);
	return (info->shape != NULL);
}

cpBool
cpShapePointQuery(cpShape *shape, cpVect p){
	return cpShapePointQueryExtended(shape, p, NULL);
}

cpBool
cpShapeSegmentQuery(cpShape *shape, cpVect a, cpVect b, cpSegmentQueryInfo *info){
	cpSegmentQueryInfo blank = {NULL, 0.0f, cpvzero};
	if(info){(*info) = blank;} else {info = &blank;}
	
	shape->klass->segmentQuery(shape, a, b, info);
	return (info->shape != NULL);
}

cpCircleShape *
cpCircleShapeAlloc(void)
{
	return (cpCircleShape *)cpcalloc(1, sizeof(cpCircleShape));
}

static cpBB
cpCircleShapeCacheData(cpCircleShape *circle, cpVect p, cpVect rot)
{
	cpVect c = circle->tc = cpvadd(p, cpvrotate(circle->c, rot));
	return cpBBNewForCircle(c, circle->r);
}

static void
cpCircleShapePointQuery(cpCircleShape *circle, cpVect p, cpPointQueryExtendedInfo *info){
	cpVect delta = cpvsub(p, circle->tc);
	cpFloat distsq = cpvlengthsq(delta);
	cpFloat r = circle->r;
	
	if(distsq < r*r){
		info->shape = (cpShape *)circle;
		
		cpFloat dist = cpfsqrt(distsq);
		info->d = r - dist;
		info->n = cpvmult(delta, 1.0/dist);
	}
}

static void
circleSegmentQuery(cpShape *shape, cpVect center, cpFloat r, cpVect a, cpVect b, cpSegmentQueryInfo *info)
{
	// offset the line to be relative to the circle
	a = cpvsub(a, center);
	b = cpvsub(b, center);
	
	cpFloat qa = cpvdot(a, a) - 2.0f*cpvdot(a, b) + cpvdot(b, b);
	cpFloat qb = -2.0f*cpvdot(a, a) + 2.0f*cpvdot(a, b);
	cpFloat qc = cpvdot(a, a) - r*r;
	
	cpFloat det = qb*qb - 4.0f*qa*qc;
	
	if(det >= 0.0f){
		cpFloat t = (-qb - cpfsqrt(det))/(2.0f*qa);
		if(0.0f<= t && t <= 1.0f){
			info->shape = shape;
			info->t = t;
			info->n = cpvnormalize(cpvlerp(a, b, t));
		}
	}
}

static void
cpCircleShapeSegmentQuery(cpCircleShape *circle, cpVect a, cpVect b, cpSegmentQueryInfo *info)
{
	circleSegmentQuery((cpShape *)circle, circle->tc, circle->r, a, b, info);
}

static const cpShapeClass cpCircleShapeClass = {
	CP_CIRCLE_SHAPE,
	(cpShapeCacheDataImpl)cpCircleShapeCacheData,
	NULL,
	(cpShapePointQueryExtendedImpl)cpCircleShapePointQuery,
	(cpShapeSegmentQueryImpl)cpCircleShapeSegmentQuery,
};

cpCircleShape *
cpCircleShapeInit(cpCircleShape *circle, cpBody *body, cpFloat radius, cpVect offset)
{
	circle->c = offset;
	circle->r = radius;
	
	cpShapeInit((cpShape *)circle, &cpCircleShapeClass, body);
	
	return circle;
}

cpShape *
cpCircleShapeNew(cpBody *body, cpFloat radius, cpVect offset)
{
	return (cpShape *)cpCircleShapeInit(cpCircleShapeAlloc(), body, radius, offset);
}

CP_DefineShapeGetter(cpCircleShape, cpVect, c, Offset)
CP_DefineShapeGetter(cpCircleShape, cpFloat, r, Radius)

cpSegmentShape *
cpSegmentShapeAlloc(void)
{
	return (cpSegmentShape *)cpcalloc(1, sizeof(cpSegmentShape));
}

static cpBB
cpSegmentShapeCacheData(cpSegmentShape *seg, cpVect p, cpVect rot)
{
	seg->ta = cpvadd(p, cpvrotate(seg->a, rot));
	seg->tb = cpvadd(p, cpvrotate(seg->b, rot));
	seg->tn = cpvrotate(seg->n, rot);
	
	cpFloat l,r,b,t;
	
	if(seg->ta.x < seg->tb.x){
		l = seg->ta.x;
		r = seg->tb.x;
	} else {
		l = seg->tb.x;
		r = seg->ta.x;
	}
	
	if(seg->ta.y < seg->tb.y){
		b = seg->ta.y;
		t = seg->tb.y;
	} else {
		b = seg->tb.y;
		t = seg->ta.y;
	}
	
	cpFloat rad = seg->r;
	return cpBBNew(l - rad, b - rad, r + rad, t + rad);
}

static void
cpSegmentShapePointQuery(cpSegmentShape *seg, cpVect p, cpPointQueryExtendedInfo *info){
	if(!cpBBContainsVect(seg->shape.bb, p)) return;
	
	cpVect a = seg->ta;
	cpVect b = seg->tb;
	
	cpVect seg_delta = cpvsub(b, a);
	cpFloat closest_t = cpfclamp01(cpvdot(seg_delta, cpvsub(p, a))/cpvlengthsq(seg_delta));
	cpVect closest = cpvadd(a, cpvmult(seg_delta, closest_t));
	
	cpVect delta = cpvsub(p, closest);
	cpFloat distsq = cpvlengthsq(delta);
	cpFloat r = seg->r;
	
	if(distsq < r*r){
		info->shape = (cpShape *)seg;
		
		cpFloat dist = cpfsqrt(distsq);
		info->d = r - dist;
		info->n = cpvmult(delta, 1.0/dist);
	}

}

static void
cpSegmentShapeSegmentQuery(cpSegmentShape *seg, cpVect a, cpVect b, cpSegmentQueryInfo *info)
{
	cpVect n = seg->tn;
	cpFloat d = cpvdot(cpvsub(seg->ta, a), n);
	cpFloat r = seg->r;
	
	cpVect flipped_n = (d > 0.0f ? cpvneg(n) : n);
	cpVect seg_offset = cpvsub(cpvmult(flipped_n, r), a);
	
	// Make the endpoints relative to 'a' and move them by the thickness of the segment.
	cpVect seg_a = cpvadd(seg->ta, seg_offset);
	cpVect seg_b = cpvadd(seg->tb, seg_offset);
	cpVect delta = cpvsub(b, a);
	
	if(cpvcross(delta, seg_a)*cpvcross(delta, seg_b) <= 0.0f){
		cpFloat d_offset = d + (d > 0.0f ? -r : r);
		cpFloat ad = -d_offset;
		cpFloat bd = cpvdot(delta, n) - d_offset;
		
		if(ad*bd < 0.0f){
			info->shape = (cpShape *)seg;
			info->t = ad/(ad - bd);
			info->n = flipped_n;
		}
	} else if(r != 0.0f){
		cpSegmentQueryInfo info1 = {NULL, 1.0f, cpvzero};
		cpSegmentQueryInfo info2 = {NULL, 1.0f, cpvzero};
		circleSegmentQuery((cpShape *)seg, seg->ta, seg->r, a, b, &info1);
		circleSegmentQuery((cpShape *)seg, seg->tb, seg->r, a, b, &info2);
		
		if(info1.t < info2.t){
			(*info) = info1;
		} else {
			(*info) = info2;
		}
	}
}

static const cpShapeClass cpSegmentShapeClass = {
	CP_SEGMENT_SHAPE,
	(cpShapeCacheDataImpl)cpSegmentShapeCacheData,
	NULL,
	(cpShapePointQueryExtendedImpl)cpSegmentShapePointQuery,
	(cpShapeSegmentQueryImpl)cpSegmentShapeSegmentQuery,
};

cpSegmentShape *
cpSegmentShapeInit(cpSegmentShape *seg, cpBody *body, cpVect a, cpVect b, cpFloat r)
{
	seg->a = a;
	seg->b = b;
	seg->n = cpvperp(cpvnormalize(cpvsub(b, a)));
	
	seg->r = r;
	
	seg->a_tangent = cpvzero;
	seg->b_tangent = cpvzero;
	
	cpShapeInit((cpShape *)seg, &cpSegmentShapeClass, body);
	
	return seg;
}

cpShape*
cpSegmentShapeNew(cpBody *body, cpVect a, cpVect b, cpFloat r)
{
	return (cpShape *)cpSegmentShapeInit(cpSegmentShapeAlloc(), body, a, b, r);
}

CP_DefineShapeGetter(cpSegmentShape, cpVect, a, A)
CP_DefineShapeGetter(cpSegmentShape, cpVect, b, B)
CP_DefineShapeGetter(cpSegmentShape, cpVect, n, Normal)
CP_DefineShapeGetter(cpSegmentShape, cpFloat, r, Radius)

void
cpSegmentShapeSetNeighbors(cpShape *shape, cpVect prev, cpVect next)
{
	cpAssertHard(shape->klass == &cpSegmentShapeClass, "Shape is not a segment shape.");
	cpSegmentShape *seg = (cpSegmentShape *)shape;
	
	seg->a_tangent = cpvsub(prev, seg->a);
	seg->b_tangent = cpvsub(next, seg->b);
}

// Unsafe API (chipmunk_unsafe.h)

void
cpCircleShapeSetRadius(cpShape *shape, cpFloat radius)
{
	cpAssertHard(shape->klass == &cpCircleShapeClass, "Shape is not a circle shape.");
	cpCircleShape *circle = (cpCircleShape *)shape;
	
	circle->r = radius;
}

void
cpCircleShapeSetOffset(cpShape *shape, cpVect offset)
{
	cpAssertHard(shape->klass == &cpCircleShapeClass, "Shape is not a circle shape.");
	cpCircleShape *circle = (cpCircleShape *)shape;
	
	circle->c = offset;
}

void
cpSegmentShapeSetEndpoints(cpShape *shape, cpVect a, cpVect b)
{
	cpAssertHard(shape->klass == &cpSegmentShapeClass, "Shape is not a segment shape.");
	cpSegmentShape *seg = (cpSegmentShape *)shape;
	
	seg->a = a;
	seg->b = b;
	seg->n = cpvperp(cpvnormalize(cpvsub(b, a)));
}

void
cpSegmentShapeSetRadius(cpShape *shape, cpFloat radius)
{
	cpAssertHard(shape->klass == &cpSegmentShapeClass, "Shape is not a segment shape.");
	cpSegmentShape *seg = (cpSegmentShape *)shape;
	
	seg->r = radius;
}
