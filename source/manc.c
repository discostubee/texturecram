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

errCode writeManifestInC(const char *manPath, const sManifest *writeMe, const sTex **refarrTexs){

#if 0
	static const char *STRUCT_SHEET="sSheet";
	//static const char *STRUCT = "struct";
	static const char *CONST = "const";
	static const char *NEW_LINE="\n";

	FILE *handFile = fopen(manPath, "w");

	if(handFile==NULL)
		return PROBLEM;

	if(writeMe->dynarrSheets != NULL){
		sSheet **itrSheet;

		fprintf(handFile,
			"struct de%s{%s\tunsigned int w;%s\tunsigned int h;%s} %s;%s",
			STRUCT_SHEET, NEW_LINE, NEW_LINE, NEW_LINE, STRUCT_SHEET, NEW_LINE
		);

		fprintf(handFile, "%s %s arrSheet[] = {%s", CONST, STRUCT_SHEET, NEW_LINE);
		for(itrSheet=writeMe->dynarrSheets; (*itrSheet) != NULL; ++itrSheet)
			fprintf(handFile, "\t{%i, %i}%c;%s",
				(*itrSheet)->w, (*itrSheet)->h,
				(itrSheet+1)!=NULL ? ',' : ' ',
				NEW_LINE
			);
		fprintf(handFile, "};%s%s", NEW_LINE, NEW_LINE);

	}else{
		fclose(handFile);
		return PROBLEM;
	}

	if(writeMe->dynarrSequences != NULL){
		//sSequence **itrSeq;
	}

	fclose(handFile);
#endif
	return NOPROB;
}

