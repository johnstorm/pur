/*
 *  purTypes.h
 *  pur
 *
 *  Created by John Lattin on 12/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#ifndef _PUR_TYPES_H_
#define _PUR_TYPES_H_

#include <stdio.h>

typedef const char*				purType;
typedef unsigned int			purID;

typedef FILE (*purFileLoaderFunction)(const char*);
typedef void (*purExitFunction)(int);
typedef void (*purSetFPSFunction)(float fps);

#endif
