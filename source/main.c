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

#include "strtools.h"
#include "filetools.h"
#include "texturepacker.h"
#include "font.h"
#include "utils.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const char SWITCH_DIR[] = "-d";	/*!< This is the switch used to specify the directory which holds our textures. */
const char SWITCH_OUTPUT[] = "-o";	/*!< Use this to specify a location and filename for output packs and manifest. */
const char SWITCH_MAXSQUARE[] = "-s";	/*!< This is the maximum texture size (width and height) our texture pack can be. The default is 1024x1024. */
const char SWITCH_NEARPOW2[] = "-p";	/*!< Forces all output sheets to be to a power of 2. */
const char SWITCH_OVERWRITE[] = "-w"; /*!< With this option, we can overwrite packs. Otherwise the pack is given a different name. */
const char SWITCH_MAN_FORMAT[] = "-f"; /*!< Specify a special format for the output to be in. */
const char SWITCH_PAD[] = "-pad"; /*!< Use padding to avoid possibly getting pixels from neighbor sprites. */
const char SWITCH_JAVAPAK[] = "-jpak"; /*!< When writing the manifest in java, you need to also specify the package name */
const char SWITCH_CLASS[] = "-class"; /*!< Used so that java and C manifests write stills and frames as inheriting off the given class */
const char SEARCH_PATTERN[] = "*.png";
const char MANIFEST_EXTENSION[] = ".txt";
const char DEFAULT_SOURCE[] = "./";
const char DEFAULT_OUTPUT[] = "./output";
const char MAN_FORMAT_C[] ="c";
const char MAN_FORMAT_CPP_1[] ="c++";
const char MAN_FORMAT_CPP_2[] ="cpp";
const char MAN_FORMAT_JAVA[] ="java";


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

const size_t PNGHEAD_SIZE = 8;
const int DEFAULT_BITDEPTH = 8;
const int DEFAULT_BYTE_PP = 4;
const int DEFAULT_COLOURTYPE = PNG_COLOR_TYPE_RGB_ALPHA;
const int DEFAULT_INTERLACE = PNG_INTERLACE_NONE;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*!\brief	Writes the standard pack info file. */
errCode writeManifestInTxt(
	const char *manPath,
	const sManifest *writeMe,
	const sTex **refarrTexs
);

/*!\brief	Writes the manifest as a C file that you can include in your project. */
errCode writeManifestInC(
	const char *manPath,
	const sManifest *writeMe,
	const sTex **refarrTexs
);

/*!\brief	Writes the manifest as a java file that you can includes in your project.
 *!\note	Assumes this is the last step, so it modifies the strings used for various names.
 */
errCode writeManifestInJava(
	const char *manPath,
	const char *javapakName,
	const char *refstrClass,
	const sManifest *writeMe,
	const sTex **refarrTexs
);


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*!\brief	Gets all the file names of every .png image found in the given directory. Outputs a series of power 2 sized textures which contain all
 *			the individual images as one sheet (or several depending on how many images you have and what your max sized square is). It also outputs
 *			a single textfile which lists which texture pack contains which images. They are listed using their file names minus the png extension.
 *			If a series of images are found, they are listed as an animation with a single reference name. So if you have cat_1.png cat_2.png cat_3.png
 *			you will get animation 'cat' with 3 frames.
 */
int main(int argc, const char *argv[], const char **envp ){
	char const *refstrSourceDir = DEFAULT_SOURCE;
	char const *refstrOutputFile = DEFAULT_OUTPUT;
	char const *refstrJavapak = NULL;
	char const *refstrManClass = NULL;
	unsigned int maxSquare = 1024;
	eOutputFormat format = eFormatDefault;
	char ignoreOutputFiles[256];	memset(ignoreOutputFiles, 0, sizeof(ignoreOutputFiles));
	short usePadding=FALSE;
	bool enforcePow2=FALSE;

	printf("---Texture Cram---\n");

	/** parse command arguments */
	while(argc > 0){
		if(argc > 1 && strncmp(argv[argc-2], SWITCH_DIR, 2)==0 ){
			refstrSourceDir = argv[argc-1];
			printf("Source directory is %s\n", refstrSourceDir);
			--argc;

		}else if(argc > 1 && strncmp(argv[argc-2], SWITCH_OUTPUT, 2)==0 ){
			refstrOutputFile = argv[argc-1];
			printf("Output path is %s\n", refstrOutputFile);
			--argc;

		}else if(argc > 1 && strncmp(argv[argc-2], SWITCH_MAXSQUARE, 2)==0 ){
			maxSquare = atoi(argv[argc-1]);
			--argc;

		}else if(argc > 1 && strncmp(argv[argc-2], SWITCH_MAN_FORMAT, 2)==0 ){
			if( strncmp(argv[argc-1], MAN_FORMAT_C, strlen(MAN_FORMAT_C) ) == 0 ){
				format = eFormatC;
				printf("Manifest is C\n");
			}else if( strncmp(argv[argc-1], MAN_FORMAT_JAVA, strlen(MAN_FORMAT_JAVA) ) == 0 ){
				format = eFormatJava;
				printf("Manifest is java\n");
			}else{
				printf("Manifest is default\n");
			}
			//- todo others
			--argc;

		}else if(argc > 1 && strncmp(argv[argc-2], SWITCH_JAVAPAK, 5)==0 ){
			refstrJavapak = argv[argc-1];
			printf("Java pack is: %s\n", refstrJavapak);
			--argc;

		}else if(argc > 1 && strncmp(argv[argc-2], SWITCH_CLASS, 5)==0 ){
			refstrManClass = argv[argc-1];
			printf("parent class is: %s\n", refstrManClass);
			--argc;

		}else if(argc > 1 && strncmp(argv[argc-2], SWITCH_PAD, 4)==0 ){
			usePadding=TRUE;
			printf("Padding used\n");
			--argc;
			
		}else if(strncmp(argv[argc-1], SWITCH_NEARPOW2, 2)==0){
			enforcePow2 = TRUE;
		}

		--argc;
	}

	printf("The max size is %i\n", maxSquare);
		
	char *strBaseOut=NULL;
	getBaseDir(&strBaseOut, refstrOutputFile);

	char *strBaseName=NULL;
	getOutputNameFromFullPath(&strBaseName, refstrOutputFile);

	char **subDirs = calloc(2, sizeof(char*));
	size_t numDirs = 1;

	copyString(&subDirs[0], refstrSourceDir);
	subDirs[1] = NULL;

	char buffCurOut[256];

	size_t iDir;
	for(iDir=0; iDir < numDirs; ++iDir){
		sTex **dynarrTextures = NULL;
		sSheetList sheets;	memset(&sheets, 0, sizeof(sheets));
		sFileList files;	memset(&files, 0, sizeof(files));
		sStillList stills;	memset(&stills, 0, sizeof(stills));
		sSeqList seqs;		memset(&seqs, 0, sizeof(seqs));
		sFontList fonts;	memset(&fonts, 0, sizeof(fonts));
		sManifest theMan;	memset(&theMan, 0, sizeof(theMan));

		if(getFiles(subDirs[iDir], &files, ignoreOutputFiles, SEARCH_PATTERN) != NOPROB)
			continue;

		if(strBaseName == NULL)
			getOutputNameFromFullPath(&strBaseName, subDirs[iDir]);

		snprintf(ignoreOutputFiles, 256, "%s*", strBaseName);
		if(strBaseOut != NULL)
			snprintf(buffCurOut, 256, "%s%s", strBaseOut, strBaseName);
		else
			strncpy(buffCurOut, strBaseName, 256);

		if(files.num > 0){
			if(genTextures(subDirs[iDir], &files, &dynarrTextures) != NOPROB)
				goto LOOP_PROB;	
		}

		if(dynarrTextures != NULL){	/** we only need to sort sequences and stills */
			if(sortTextures(dynarrTextures, &seqs, &stills) != NOPROB)
				goto LOOP_PROB;
		}

		{	/** fonts */
			sTex **dynarrFntTexs =NULL;
			sFontInfo **dynarrFntIfo = genFontInfos(subDirs[iDir], ignoreOutputFiles);
			if(dynarrFntIfo != NULL && genTexFromFonts(dynarrFntIfo, &dynarrFntTexs, &fonts) != NOPROB)
				goto LOOP_PROB;

			unsigned int startIdx;
			if(dynarrTextures == NULL){
				startIdx = 0;
			}else{
				for(startIdx=0; dynarrTextures[startIdx] != NULL; ++startIdx)
					;
			}

			appendTexArr(&dynarrTextures, dynarrFntTexs);
			genFontFromInfo((const sFontInfo **)dynarrFntIfo, &fonts, startIdx);
			cleanupFontInfos(&dynarrFntIfo);
		}

		if(dynarrTextures == NULL){
			DBUG_WARN("No textures found");
			goto LOOP_PROB;
		}

		if(arrangeTextures(dynarrTextures, &seqs, &stills, &fonts, &sheets, maxSquare) != NOPROB)
			goto LOOP_PROB;
	
		if(genMan(
			strBaseName,
			&seqs,
			&stills,
			&fonts,
			&sheets,
			(const sTex **)dynarrTextures,
			&theMan
		) != NOPROB)
			goto LOOP_PROB;

		switch(format){
			case eFormatDefault:
				if(writeManifestInTxt(buffCurOut, &theMan, (const sTex **)dynarrTextures) != NOPROB)
					goto LOOP_PROB;	
				break;

			case eFormatC:
				if(writeManifestInC(buffCurOut, &theMan, (const sTex **)dynarrTextures) != NOPROB)
					goto LOOP_PROB;	
				break;

			case eFormatJava:
				if(writeManifestInJava(buffCurOut, refstrJavapak, refstrManClass, &theMan, (const sTex **)dynarrTextures)!=NOPROB)
					goto LOOP_PROB;	
				break;

			default:
				break;
		}

		if(writeSheets(strBaseOut, strBaseName, dynarrTextures, &sheets) != NOPROB)
			goto LOOP_PROB;

		{
			char **moreDirs = NULL;
			getDirs(subDirs[iDir], &moreDirs);

			if(moreDirs != NULL){
				size_t iMore;
				for(iMore = 0; moreDirs[iMore] != NULL; ++iMore){
					++numDirs;
					subDirs = realloc(subDirs, (numDirs + 1) * sizeof(char*));
					snprintf(buffCurOut, 245, "%s/%s", subDirs[iDir], moreDirs[iMore]);
					SAFE_DELETE(moreDirs[iMore]);
					copyString(&subDirs[ numDirs - 1], buffCurOut);
					subDirs[numDirs] = NULL;
				}
			}

			SAFE_DELETE(moreDirs);
		}

	LOOP_PROB:
		cleanupFileList(&files);
		cleanupTextures(&dynarrTextures);
		cleanupSheetList(&sheets);
		cleanupStillList(&stills);
		cleanupSeqList(&seqs);
		cleanupManifest(&theMan);

		SAFE_DELETE(subDirs[iDir]);
		SAFE_DELETE(strBaseName);
	}

	SAFE_DELETE(strBaseName);
	SAFE_DELETE(strBaseOut);
	SAFE_DELETE(subDirs);

	LOG("Finished!\n");
	return 0;
}


