/*
 *
 *  Copyright (C) 2012 Stuart Bridgens
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
 */

#ifndef FONT_H
#define FONT_H

#include "texturepacker.h"

typedef struct defFontInfo{
	char *strName;
	char *strFile;
	unsigned int numGlyphs;
	unsigned int size;
	unsigned int numLoaded;
	unsigned char colour[3];	/** 32 bit */
	unsigned int *dynarrCharcodes;
	unsigned int *dynarrOffsetY;
} sFontInfo;

	/** Generate info structure for each font info file found at provided path. Null terminated array. */
sFontInfo** genFontInfos(const char *strPath, const char *strIgnores);

	/** Generate a null terminated list of textures from the null terminated list of fonts. */
errCode genTexFromFonts(sFontInfo **pDynarrFonts, sTex ***pDynarrOutTex, sFontList *pOutList);

	/** */
errCode genFontFromInfo(const sFontInfo **refInfo, sFontList *pOutList, unsigned int startTexIdx);

void cleanupFontInfos(sFontInfo ***pDynarrFonts);

void cleanupFontInfo(sFontInfo *font);


#endif
