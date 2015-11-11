/*
 *
 *  Copyright (C) 2011  Stuart Bridgens
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License (version 3) as published by
 *  the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *	Special thanks to:
 *	Rod Stephens
 *	http://www.devx.com/dotnet/Article/36005/1954
 *
 */

#ifndef STRTOOLS_H
#define STRTOOLS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum defCompare{
	GREATER, LESS, SAME
} compare;

static const unsigned short asciNumStart = 48;
static const unsigned short asciNumEnd = 57;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* !\brief	*/
bool hasWildStr(const char *findIn, const char *wildStr);

/* !\brief	 Creates memory and copies a string into that memory location */
void copyString(char **strGoesHere, const char *copyMe);

/* !\brief	 Creates memory and copies characters into that memory location, until the end of a give character is found. */
void copyStringUntil(char **strGoesHere, const char *copyMe, char until);

/*!\brief	Sorts a dynamic array of string pointers. */
void sortStrings(char **dynarrStrings, size_t count);

/*!\brief	*/
void sanitiseString(char *opInPlace);

/* !\brief	Allocates and copies the output name (minus any file extensions), into the outputStr. */
void getOutputNameFromFullPath(char **outputStr, const char *strFullPath);

/* !\brief	Similar to copyString, but it doesn't copy certain characters */
void copyWithoutChars(char **strGoesHere, const char *strCopyMe, const char *strNotThese);

#endif
