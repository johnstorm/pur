/*
 *  purEngine.c
 *  pur
 *
 *  Created by John Lattin on 12/12/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "purEngine.h"

purContext* purCreate(purInfo info)
{
	purContext* pur = (purContext*)calloc(1, sizeof(purContext));

	if (pur != NULL)
	{
		pur->info = info;
	}

	return pur;
}

void purDestroy(purContext* pur)
{
	if (pur != NULL)
	{
		free(pur);
	}
}

// MARK: -
// MARK: Events
// MARK: -

// MARK: Input

void purInputPosition(purContext* pur, purType type, purID identifier, inkPoint point)
{
}

void purInputButton(purContext* pur, purType type, unsigned int keycode)
{
}

void purInputEnable(purContext* pur, purType type)
{
}

void purInputDisable(purContext* pur, purType type)
{
}

void purSetViewState(purContext* pur, inkSize size)
{
}
