/*
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

#include "texturepacker.h"
#include "squarefit.h"

errCode writeManifestInTxt(const char *manPath, const sManifest *writeMe, const sTex **refarrTexs){
	static const char *HEAD_SHEET = "sheet_count=";
	static const char *HEAD_SEQUENCES = "sequences_count=";
	static const char *HEAD_STILLS = "stills_count=";
	static const char *HEAD_FONTS = "fonts_count=";
	static const char *FILE_TYPE = ".txt";
	static const char *NEW_LINE="\n";
	static const size_t LENBUFF = 128;

	char buff[ LENBUFF ];
	FILE *handFile; 
	sTex const *refTex;
	unsigned int i, j;

	snprintf(buff, LENBUFF, "%s%s", manPath, FILE_TYPE);
	handFile = fopen(buff, "w");

	if(manPath == NULL)
		return ERROR;

	if(writeMe == NULL)
		return ERROR;

	/** Sheets */
	{
		sSheet const *refSheet;

		fprintf(handFile, "%s%i%s", HEAD_SHEET, writeMe->refSheets->num, NEW_LINE);
		for(i=0; i < writeMe->refSheets->num; ++i){
			refSheet = writeMe->refSheets->dynarrSheets[i];
			fprintf(handFile, "%s,%i,%i%s", 
				refSheet->name,
				refSheet->w, 
				refSheet->h, 
				NEW_LINE
			);
		}
	}

	/** Stills */
	fprintf(handFile, "%s%i%s", HEAD_STILLS, writeMe->refStills->num, NEW_LINE);
	for(i=0; i < writeMe->refStills->num; ++i){
		refTex = refarrTexs[
			writeMe->refStills->dynarrTexIDs[i]
		];

		strncpy(buff, refTex->name, LENBUFF);
		sanitiseString(buff);

		fprintf(handFile, "%s,%i,%i,%i,%i%s", 
			refTex->name, 
			refTex->x, 
			refTex->y, 
			refTex->w, 
			refTex->h,
			NEW_LINE
		); 
	}

	/** Sequences */
	{
		sTexSeq const* refSeq;
		size_t posDelim;

		fprintf(handFile, "%s%i%s", HEAD_SEQUENCES, writeMe->refSeqs->num, NEW_LINE);
		for(i=0; i < writeMe->refSeqs->num; ++i){
			refSeq = &writeMe->refSeqs->dynarrSeqs[i];

			if(refSeq == NULL)
				continue;

			if(refSeq->num == 0){
				WARN("Empty sequence");
				continue;
			}

			refTex = refarrTexs[ refSeq->dynarrTexIDs[0] ];
			posDelim = strcspn(refTex->name, "1234567890");

			if(posDelim > 0){
				memcpy(buff, refTex->name, posDelim);
				buff[ 
					(posDelim < LENBUFF) ? posDelim : LENBUFF -1
				] = '\0';
			}else{
				strncpy(buff, refTex->name, LENBUFF);
			}

			sanitiseString(buff);
			
			fprintf(handFile, "%s,%i,%i,%i,(", buff, refSeq->num, refTex->w, refTex->h);
			for(j=0; j < refSeq->num; ++j){
				refTex = refarrTexs[ refSeq->dynarrTexIDs[j] ];
				fprintf(handFile, "(%i,%i,%i),", refTex->x, refTex->y, refSeq->dynarrSheetIDs[j]);
			}

			fprintf(handFile, ")%s", NEW_LINE);
		}
	}

	/** Fonts */
	{
		sFont *refFnt;
		size_t posDelim;

		fprintf(handFile, "%s%i%s", HEAD_FONTS, writeMe->refFonts->num, NEW_LINE);
		for(i=0; i < writeMe->refFonts->num; ++i){
			refFnt = writeMe->refFonts->dynarrFonts[i];
			if(refFnt->num == 0){
				WARN("Empty font");
				continue;
			}	

			refTex = refarrTexs[ refFnt->dynarrTexIDs[0] ];
			posDelim = strcspn(refTex->name, "1234567890");

			if(posDelim > 0){
				memcpy(buff, refTex->name, posDelim);
				buff[ 
					(posDelim < LENBUFF) ? posDelim : LENBUFF -1
				] = '\0';
			}else{
				strncpy(buff, refTex->name, LENBUFF);
			}

			sanitiseString(buff);

			fprintf(handFile, "%s,%i,(", buff, refFnt->num);

			for(j=0; j < refFnt->num; ++j){
				refTex = refarrTexs[ refFnt->dynarrTexIDs[j] ];
				fprintf(handFile, "(%i,%i,%i,%i,%i,%i,%i),", 
					refFnt->dynarrCharcodes[j], 
					refFnt->dynarrSheetIDs[j],
					refTex->x, refTex->y,
					refTex->w, refTex->h,
					refFnt->dynarrOffsetY[j]
				);
			}

			fprintf(handFile, ")%s", NEW_LINE);
		}
	}

	fclose(handFile);

	return NOPROB;

/*
	if(handFile==NULL){
		WARN("Unable to write file %s.", manPath);
		return PROBLEM;
	}

	if(writeMe->dynarrSheets != NULL){
		sSheet **itrSheet;
		unsigned int numSheets=0;

		for(itrSheet = writeMe->dynarrSheets; (*itrSheet) != NULL; ++itrSheet)
			++numSheets;

		fprintf(handFile, "%s%i%s", HEAD_SHEET, numSheets, NEW_LINE);
		for(itrSheet = writeMe->dynarrSheets; (*itrSheet) != NULL; ++itrSheet){
			fprintf(handFile, "%i,%i%s", (*itrSheet)->w, (*itrSheet)->h, NEW_LINE);
		}
	}else{
		fclose(handFile);
		return PROBLEM;
	}

	if(writeMe->dynarrSequences != NULL){
		sSequence **itrSeq;
		unsigned int numSeq=0;

		for(itrSeq= writeMe->dynarrSequences; (*itrSeq) != NULL; ++itrSeq)
				++numSeq;

		fprintf(handFile, "%s%i%s", HEAD_SEQUENCES, numSeq, NEW_LINE);

		unsigned int idxSeqSheet, idxFrame;

		itrSeq= writeMe->dynarrSequences;
		while( *itrSeq!=NULL ){
			if((*itrSeq)==NULL || (*itrSeq)->name==NULL)
				continue;

			fprintf(handFile, "\"%s\",%i,%i,%i,%i,(",
					(*itrSeq)->name,
					(*itrSeq)->numFrames,
					(*itrSeq)->w, (*itrSeq)->h,
					(*itrSeq)->sizeSheets
			);

			for(idxFrame=0; idxFrame < (*itrSeq)->numFrames; ++idxFrame)
				fprintf(handFile, "%u %u,", (*itrSeq)->dynarrFrames[idxFrame].x, (*itrSeq)->dynarrFrames[idxFrame].y);

			fprintf(handFile, "),(");

			for(idxSeqSheet=0; idxSeqSheet < (*itrSeq)->sizeSheets; ++idxSeqSheet)
				fprintf(handFile, "%u,", (*itrSeq)->dynarrSheetID[idxSeqSheet]);

			fprintf(handFile, "),(");

			if((*itrSeq)->dynarrNextSheetOnFrame!=NULL){
				for(idxSeqSheet=0; idxSeqSheet < (*itrSeq)->sizeSheets -1; ++idxSeqSheet)
					fprintf(handFile, "%u,", (*itrSeq)->dynarrNextSheetOnFrame[idxSeqSheet]);
			}

			fprintf(handFile, ")%s", NEW_LINE);

			++itrSeq;
		}
	}

	if(writeMe->dynarrStills != NULL){
		sStill **itrStill;
		unsigned int numStills=0;

		for(itrStill = writeMe->dynarrStills; (*itrStill) != NULL; ++itrStill )
			++numStills;

		fprintf(handFile, "%s%i%s", HEAD_STILLS, numStills, NEW_LINE);

		for(itrStill = writeMe->dynarrStills; (*itrStill) != NULL; ++itrStill ){
			fprintf(handFile, "\"%s\",%i,%i,%i,%i,%i%s",
				(*itrStill)->name,
				(*itrStill)->x, (*itrStill)->y,
				(*itrStill)->w, (*itrStill)->h,
				(*itrStill)->sheetID,
				NEW_LINE
			);
		}
	}


	fclose(handFile);

	return NOPROB;
*/
}


