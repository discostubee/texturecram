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

#ifndef TEXTUREPACKER_H
#define TEXTUREPACKER_H

#include <png.h>
#include "strtools.h"
#include "filetools.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern const int DEFAULT_BITDEPTH;
extern const int DEFAULT_BYTE_PP;
extern const int DEFAULT_COLOURTYPE;
extern const int DEFAULT_INTERLACE;
extern const size_t PNGHEAD_SIZE;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef enum defOutputFormat{
	eFormatDefault, eFormatC, eFormatCPP, eFormatJava
} eOutputFormat;

typedef struct defsTex{
	char *name;		/** Filename with the extension stripped off. */
	unsigned int x, y;	 /** These are the pack coordinates. */
	unsigned int w, h;	/** These are the pixel sizes of the image. */

	png_byte **dynarrRows;		/** Number of rows is equal to the height of this texture. */
	png_struct *pngptrData;
	png_info *pngptrInfo;
	png_byte colorType;
} sTex;

typedef struct defsPack{
	char *name; /*!< Cleanup */
	unsigned int idxStart, idxEnd;	/** Start and end indices for the array of textures referred to by the manifest output file. */
} sPack;

typedef struct defStillList{
	unsigned int *dynarrTexIDs;
	unsigned int *dynarrSheetIDs;
	unsigned int num;
} sStillList;

typedef struct defTexSeq{
	unsigned int *dynarrTexIDs;	/*!< Cleanup the array but not the content. */
	unsigned int *dynarrSheetIDs;
	unsigned int num;
} sTexSeq;

typedef struct defSeqList{
	sTexSeq *dynarrSeqs;	/*!< Cleanup. */
	unsigned int num;
} sSeqList;

typedef struct defsSheet{
	unsigned int w, h;
	char *name;	/*!< This is the PNG filename for the packed texture. Cleanup. */

	unsigned int *dynarrTexIDs;
	unsigned int num;
} sSheet;

typedef struct defSheetList{
	sSheet **dynarrSheets;
	unsigned int num;
} sSheetList;

typedef struct defFont{
	unsigned int *dynarrTexIDs;
	unsigned int *dynarrSheetIDs;
	unsigned int *dynarrCharcodes;
	unsigned int *dynarrOffsetY;
	unsigned int num;
} sFont;

typedef struct defFontList{
	sFont **dynarrFonts;
	unsigned int num;
} sFontList;

typedef struct defsMenifest{
	sSheetList const *refSheets;

	char **dynarrStrStillNames;
	sStillList const *refStills;

	char **dynarrStrSeqNames;
	sSeqList const *refSeqs;

	char **dynarrStrFontNames;
	sFontList const *refFonts;
} sManifest;

//*!\brief	Stores info about the spot we last wrote to the sheet. */
typedef struct defsSpotSheetWrite{
	unsigned int x, y, h, w, lineH;
} sSpotSheetWrite;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*!\brief	Creates a dynamic array of loaded textures. This dynamic array is null terminated.
 *!\return	Non zero if it fails horribly.
 */
errCode genTextures(const char *rootDir, const sFileList *files, sTex ***dynarrTextures);

/*!\brief	Sorts the list of textures into stills and sequences.
 */
errCode sortTextures(sTex **arrSortMe, sSeqList *outSeqs, sStillList *outStills);

/*!\brief	Arrange the textures onto squares.
 *!\param	pSeqs		
 *!\param	pStills		
 *!\param	pFonts
 *!\param	dynarrOutSheets	Output a NULL terminated list of sheets which relates the textures to the sheets. You'll need to clean this list up.
 *!\param	maxSquare	The max size each sheet can reach.	
 */
errCode arrangeTextures(
	sTex **arrTexs,
	sSeqList *pSeqs,
	sStillList *pStills,
	sFontList *pFonts,
	sSheetList *pOutSheets, 
	unsigned int maxSquare
);

/*!\brief	Generates the manifest.
 */
errCode genMan(
	const char *strManName, 
	const sSeqList *refSeqs, 
	const sStillList *refStills, 
	const sFontList *refFonts, 
	const sSheetList *refSheets, 
	const sTex **refarrTexs,
	sManifest *outMan
);

/*!\brief	Outputs the sheets.
 *!\param	strPath		Path to write the sheet to.
 *!\param	strManName	The name of the file.
 *!\param	arrSheets	The
 *!\param	pow2		Should the sheets be padded to be a
 */
errCode writeSheets(const char *strPath, const char *strManName, sTex **refArrTex, sSheetList *pSheets);

/*!\brief	*/
void appendTexArr(sTex ***pAppendHere, sTex **pFrom);

/*!\brief	Cleanups up a single textures. */
void cleanupTex(sTex *pTex);

/*!\brief	Cleans up the textures and their dynamic array.	*/
void cleanupTextures(sTex ***dynarrTextures);

/*!\brief	*/
void cleanupStillList(sStillList *list);

/*!\brief	*/
void cleanupSeqList(sSeqList *list);

/*!\brief	*/
void cleanupSheetList(sSheetList *dynarrSheets);

/*!\brief	*/
void cleanupFontList(sFontList *list);

/*!\brief	*/
void cleanupManifest(sManifest *man);

/*!brief	Converts the frame name to a search string. */
void imgFNameToWSearch(const char *fName, char *outBuff, unsigned int sizeOutBuff);

#endif
