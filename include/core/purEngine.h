/*
 *  purEngine.h
 *  pur
 *
 *  Created by John Lattin on 12/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _PUR_ENGINE_H_
#define _PUR_ENGINE_H_

#include "purHeader.h"
#include "purTypes.h"

#include "inkGeometry.h"

typedef struct
{
} purInfo;

typedef struct
{
	purInfo info;
} purContext;

purExtern purContext* purCreate(purInfo info);
purExtern void purDestroy(purContext* pur);

// MARK: -
// MARK: Events
// MARK: -

// MARK: Input

purExtern void purInputPosition(purContext* pur, purType type, purID identifier, inkPoint point);
purExtern void purInputButton(purContext* pur, purType type, unsigned int keycode);
purExtern void purInputEnable(purContext* pur, purType type);
purExtern void purInputDisable(purContext* pur, purType type);

purExtern void purSetViewState(purContext* pur, inkSize size);

#endif
