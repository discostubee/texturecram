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

#include "strtools.h"
#include "texturepacker.h"

errCode writeManifestInJava(
	const char *manPath,
	const char *javapakName,
	const char *refstrClass,
	const sManifest *writeMe,
	const sTex **refarrTexs
){
	static const char *NEW_LINE="\n";
	static const char *TAB = "   ";
	static const char *CLASS="public static class";
	static const char *ENTRY = "public static";

	unsigned int i, j;
	sTex const *refTex;

	FILE *handFile = NULL;
	{
		char *buff = (char*)calloc(strlen(manPath)+strlen(".java")+1, sizeof(char));
		sprintf(buff, "%s.java", manPath);

		handFile = fopen(buff, "w");
		free(buff);
	}

	if(javapakName != NULL)
		fprintf(handFile, "package %s;%s%s", javapakName, NEW_LINE, NEW_LINE);

	fprintf(handFile, "import java.util.Map;%s%s", NEW_LINE, NEW_LINE);
	fprintf(handFile, "import java.util.HashMap;%s%s", NEW_LINE, NEW_LINE);

	{
		char *buff=NULL;
		getOutputNameFromFullPath(&buff, manPath);
		fprintf(handFile, "public class %s {%s", buff, NEW_LINE);
		free(buff);
	}

	/*** sheets ***/
	fprintf(handFile, "%s%s%s Sheet {%s", NEW_LINE, TAB, CLASS, NEW_LINE);
	fprintf(handFile, "%s%spublic final int w, h;%s%s", TAB, TAB, NEW_LINE, NEW_LINE);
	fprintf(handFile, "%s%spublic final String name;%s", TAB, TAB, NEW_LINE);
	fprintf(handFile,
		"%s%spublic Sheet(int pw, int ph, String pName){ w=pw; h=ph; name=pName;}%s%s",
		TAB, TAB, NEW_LINE, NEW_LINE
	);

	fprintf(handFile, "%s}%s%s", TAB, NEW_LINE, NEW_LINE);

	sSheet *refSheet;
	for(i=0; i < writeMe->refSheets->num; ++i){
		refSheet = writeMe->refSheets->dynarrSheets[i];
		fprintf(handFile, "%s%s Sheet sheet%i = new Sheet(%i, %i, \"%s\");%s",
			TAB, ENTRY, i,
			refSheet->w, refSheet->h, refSheet->name,
			NEW_LINE
		);
	}

	/*** stills ***/
	if(refstrClass!=NULL){
		fprintf(handFile, "%s%s%s Still extends %s {%s", NEW_LINE, TAB, CLASS, refstrClass, NEW_LINE);
	}else{
		fprintf(handFile, "%s%s%s Still {%s", NEW_LINE, TAB, CLASS, NEW_LINE);
	}

	fprintf(handFile, "%s%spublic final Sheet mSheet;%s", TAB, TAB, NEW_LINE);
	fprintf(handFile, "%s%spublic final int x, y, w, h;%s", TAB, TAB, NEW_LINE);
	fprintf(handFile,  "%s%s%spublic Still(Sheet pSheet, int px, int py, int pw, int ph){ mSheet=pSheet; x=px; y=py; w=pw; h=ph; }%s%s",
		NEW_LINE, TAB, TAB, NEW_LINE, NEW_LINE
	);

	if(refstrClass!=NULL){
		fprintf(handFile,
			"%s%s@Override%s%s%spublic void load(){ load(mSheet.name, x, y, w, h); }%s",
			TAB, TAB, NEW_LINE, TAB, TAB, NEW_LINE
		);
	}


	for(i=0; i < writeMe->refStills->num; ++i){
		refTex = refarrTexs[ writeMe->refStills->dynarrTexIDs[i] ];

		if(refTex->name == NULL)	/* ignore entries that were blanked for not being convertable */
			continue;

		fprintf(handFile, "%s%s%s Still %s = new Still(sheet%i, %i, %i, %i, %i);%s",
			TAB, TAB, ENTRY, writeMe->dynarrStrStillNames[i],
			writeMe->refStills->dynarrSheetIDs[i], 
			refTex->x, refTex->y, refTex->w, refTex->h,
			NEW_LINE
		);
	}
	fprintf(handFile, "%s}%s%s", TAB, NEW_LINE, NEW_LINE);

	/*** sequence ***/
	if(refstrClass!=NULL){
		fprintf(handFile, "%s%s Sequence extends %s {%s", TAB, CLASS, refstrClass, NEW_LINE);
	}else{
		fprintf(handFile, "%s%s Sequence{%s", TAB, CLASS, NEW_LINE);
	}

	/*** frame ***/
	if(refstrClass!=NULL){
		fprintf(handFile, "%s%s%s Frame extends %s{%s", TAB, TAB, CLASS, refstrClass, NEW_LINE);
	}else{
		fprintf(handFile, "%s%s%s Frame{%s", TAB, TAB, CLASS, NEW_LINE);
	}

	fprintf(handFile, "%s%s%spublic final Sheet mSheet;%s", TAB, TAB, TAB, NEW_LINE);
	fprintf(handFile, "%s%s%spublic final int x, y;%s", TAB, TAB, TAB, NEW_LINE);
	fprintf(handFile,
		"%s%s%spublic Frame(Sheet pSheet, int px, int py) { mSheet=pSheet; x=px; y=py; }%s",
		TAB, TAB, TAB, NEW_LINE
	);
	fprintf(handFile, "%s%s}%s", TAB, TAB, NEW_LINE);

	/*** sequence, again ***/
	fprintf(handFile, "%s%spublic final int w, h;%s", TAB, TAB, NEW_LINE);
	fprintf(handFile, "%s%spublic Frame frames[];%s", TAB, TAB, NEW_LINE);
	fprintf(handFile,
		"%s%s%spublic Sequence(Frame pFrames[], int pw, int ph) { w=pw; h=ph; frames = pFrames.clone(); }%s",
		NEW_LINE, TAB, TAB, NEW_LINE
	);
	fprintf(handFile,
		"%s%s%s@Override%s%s%spublic void load() { for(Frame f : frames) f.load(f.mSheet.name, f.x, f.y, w, h); }%s%s",
		NEW_LINE, TAB, TAB, NEW_LINE, TAB, TAB, NEW_LINE, NEW_LINE
	);
	fprintf(handFile,
		"%s%s%s@Override%s%s%spublic %s[] getRelated() { return frames; }%s%s",
		NEW_LINE, TAB, TAB, NEW_LINE, TAB, TAB, refstrClass, NEW_LINE, NEW_LINE
	);

	{
		sTexSeq *refSeq;
		char *refSeqName;
		for(i=0; i < writeMe->refSeqs->num; ++i){
			refSeq = &writeMe->refSeqs->dynarrSeqs[i];
			refSeqName = writeMe->dynarrStrSeqNames[i];

			/** We have to first make our frames, and then assign them to the sequence. */
			fprintf(handFile, "%s%s%sprivate static Frame[] framesFor_%s = {%s",
				NEW_LINE, TAB, TAB, refSeqName, NEW_LINE
			);
			for(j=0; j < refSeq->num; ++j){ 
				refTex = refarrTexs[ refSeq->dynarrTexIDs[j] ];

				fprintf(handFile, "%s%s%snew Frame(sheet%i, %i, %i),%s",
					TAB, TAB, TAB,
					refSeq->dynarrSheetIDs[j], refTex->x, refTex->y,
					NEW_LINE
				);
			}
			fprintf(handFile, "%s%s};%s", TAB, TAB, NEW_LINE);

			refTex = refarrTexs[ refSeq->dynarrTexIDs[0] ];
			fprintf(handFile, "%s%s%s%s Sequence %s = new Sequence(", NEW_LINE, TAB, TAB, ENTRY, refSeqName);
			fprintf(handFile, "framesFor_%s, %i, %i);%s", refSeqName, refTex->w, refTex->h, NEW_LINE);
		}
	}

	fprintf(handFile, "%s}%s", TAB, NEW_LINE);

	/*** font ***/
	if(refstrClass!=NULL){
		fprintf(handFile, "%s%s%s Font extends %s {%s", NEW_LINE, TAB, CLASS, refstrClass, NEW_LINE);
	}else{
		fprintf(handFile, "%s%s%s Font {%s",NEW_LINE, TAB, CLASS, NEW_LINE);
	}

	/*** glyph ***/
	if(refstrClass!=NULL){
		fprintf(handFile, "%s%s%s Glyph extends %s {%s", TAB, TAB, CLASS, refstrClass, NEW_LINE);
	}else{
		fprintf(handFile, "%s%s%s Glyph {%s", TAB, TAB, CLASS, NEW_LINE);
	}
	fprintf(handFile,
		"%s%s%spublic final Sheet mSheet;%s",
		TAB, TAB, TAB, NEW_LINE
	);
	fprintf(handFile,
		"%s%s%spublic final int ccode, x, y, w, h, offsetY;%s", 
		TAB, TAB, TAB, NEW_LINE
	);
	fprintf(handFile,
		"%s%s%spublic Glyph(Sheet pSheet, int pCCode, int px, int py, int pw, int ph, int pOY)%s",
		TAB, TAB, TAB, NEW_LINE
	);
	fprintf(handFile,
		"%s%s%s%s{ mSheet=pSheet; ccode=pCCode; x=px; y=py; w=pw; h=ph; offsetY=pOY; }%s",
		TAB, TAB, TAB, TAB, NEW_LINE
	);
	fprintf(handFile, "%s%s}%s%s", TAB, TAB, NEW_LINE, NEW_LINE);


	/*** font again ***/
	fprintf(handFile, "%s%spublic Glyph glyphs[];%s", TAB, TAB, NEW_LINE);
	fprintf(handFile, "%s%spublic Map<Integer, %s> mapped;%s", TAB, TAB, refstrClass, NEW_LINE);
	fprintf(handFile, "%s%spublic Map<Integer, Integer> yOffsets;%s", TAB, TAB, NEW_LINE);
	fprintf(handFile,
		"%s%spublic Font(Glyph pGlyphs[]) {\
 glyphs = pGlyphs.clone(); mapped = new HashMap<Integer, %s>(); yOffsets = new HashMap<Integer, Integer>();\
 }%s",
		TAB, TAB, refstrClass, NEW_LINE
	);

	fprintf(handFile,"%s%s%s@Override%s%s%spublic void load(){%s", NEW_LINE, TAB, TAB, NEW_LINE, TAB, TAB, NEW_LINE);
	fprintf(handFile,"%s%s%sfor(Glyph f : glyphs){%s", TAB, TAB, TAB, NEW_LINE);
	fprintf(handFile,"%s%s%s%sf.load(f.mSheet.name, f.x, f.y, f.w, f.h);%s", TAB, TAB, TAB, TAB, NEW_LINE);
	fprintf(handFile,"%s%s%s%smapped.put(f.ccode, f);%s", TAB, TAB, TAB, TAB, NEW_LINE);
	fprintf(handFile,"%s%s%s%syOffsets.put(f.ccode, f.offsetY);%s", TAB, TAB, TAB, TAB, NEW_LINE);
	fprintf(handFile,"%s%s%s}%s%s%s}%s", TAB, TAB, TAB, NEW_LINE, TAB, TAB, NEW_LINE);

	fprintf(handFile,
		"%s%s%s@Override%s%s%spublic %s[] getRelated() { return glyphs; }%s%s",
		NEW_LINE, TAB, TAB, NEW_LINE, TAB, TAB, refstrClass, NEW_LINE, NEW_LINE
	);

	{
		sFont *refFnt;
		for(i=0; i < writeMe->refFonts->num; ++i){
			refFnt = writeMe->refFonts->dynarrFonts[i];
			fprintf(handFile, "%s%sprivate static Glyph glyphsFor_%s [] ={%s", 
				TAB, TAB, writeMe->dynarrStrFontNames[i], NEW_LINE
			);
			for(j=0; j < refFnt->num; ++j){
				refTex = refarrTexs[ refFnt->dynarrTexIDs[j] ];
				fprintf(handFile, "%s%s%snew Glyph(sheet%i, %i, %i, %i, %i, %i, %i),%s",
					TAB, TAB, TAB,
					refFnt->dynarrSheetIDs[j],
					refFnt->dynarrCharcodes[j],
					refTex->x, refTex->y, refTex->w, refTex->h, refFnt->dynarrOffsetY[j],
					NEW_LINE
				);
			}
			fprintf(handFile, "%s%s};%s", TAB, TAB, NEW_LINE);
			fprintf(handFile, "%s%s%s Font  %s = new Font(glyphsFor_%s);%s",
				 TAB, TAB, ENTRY, writeMe->dynarrStrFontNames[i], writeMe->dynarrStrFontNames[i], NEW_LINE
			);
		}
	}

	fprintf(handFile, "%s}%s", TAB, NEW_LINE);

	/*** end man ***/
	fprintf(handFile, "}%s", NEW_LINE );

	return NOPROB;
}

