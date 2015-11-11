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

#ifndef png_jmpbuf
#	define png_jmpbuf(png_ptr) ((png_ptr)->png_jmpbuf)
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void myPNGWarnFoo(png_structp pngptrData, png_const_charp warning_msg){
	printf("%s\n", warning_msg);
}

static errCode readTexToSheet(
	const sTex *refptrTex, 
	png_byte **buffImg, 
	unsigned int sheetW, 
	unsigned int sheetH,
	const png_size_t sizeRow
){
	if(refptrTex->x + refptrTex->w > sheetW || refptrTex->y + refptrTex->h > sheetH){
		WARN("Sheet overflow");
		return PROBLEM;
	}
	
	if(refptrTex->dynarrRows==NULL){
		ERROR_LOG("Texture isn't allocated");
		return ERROR;
	}

	const unsigned int sizePix = (unsigned int)((sizeRow / sheetW) / sizeof(png_byte));
	
	unsigned int row;
	for(row=0; row < refptrTex->h; ++row){
		if(refptrTex->dynarrRows[row] == NULL){
			WARN("Texture underflow");
			return PROBLEM;
		}
		memcpy(
			&buffImg[ refptrTex->y + row ][ refptrTex->x * sizePix ],
			refptrTex->dynarrRows[row],
			refptrTex->w * sizePix
		);
	}
	return NOPROB;
}

errCode cleanupPNGImg(png_struct *pngptrData, png_byte ***buffImg){
	if(buffImg == NULL || pngptrData == NULL)
		return NOPROB;
	
	png_byte **itrRow;
	for(itrRow = (*buffImg); (*itrRow) != NULL; ++itrRow)
		png_free(pngptrData, *itrRow);
	
	SAFE_DELETE(*buffImg);

	return NOPROB;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

errCode genTextures(const char *rootDir, const sFileList *files, sTex ***dynarrTextures){
	static const size_t BUFFLEN = 1024;

	int numTexs=0;
	int idxFile = files->num;
	sTex *refTex;

	if(idxFile <= 0 || rootDir == NULL || rootDir[0] == '\0' || dynarrTextures == NULL)
		return PROBLEM;

	size_t lenRootDir = strlen(rootDir);
	char filePath[BUFFLEN];
	FILE *handFile;
	png_byte header[PNGHEAD_SIZE];

	strncpy(filePath, rootDir, 1024);
	if(lenRootDir>0 && filePath[lenRootDir-1]!='/'){
		filePath[lenRootDir] = '/';
		lenRootDir += 1;
	}

	while(idxFile > 0){
		--idxFile;

		strncpy(&filePath[lenRootDir], files->dynarrFiles[idxFile], 1024 - lenRootDir);

		handFile = fopen(filePath, "rb");

		if(handFile == NULL){
			WARN("Can't open file %s", filePath);
			continue;
		}

		if(	fread(header, sizeof(png_byte), PNGHEAD_SIZE, handFile) == PNGHEAD_SIZE
			&& png_sig_cmp(header, 0, PNGHEAD_SIZE) == 0
		){
			XTRA_LOG("File %s opened as PNG", filePath);

		}else{
			WARN("File %s isn't a PNG", filePath);
			fclose(handFile);
			continue;
		}

		++numTexs;
		(*dynarrTextures) = (sTex**)realloc_chk((*dynarrTextures), (numTexs+1) * sizeof(sTex*));
		(*dynarrTextures)[numTexs]=NULL;

		refTex = (*dynarrTextures)[numTexs-1] = (sTex*)malloc_chk(sizeof(sTex));
		memset(refTex, 0, sizeof(sTex));

		refTex->pngptrData = png_create_read_struct(
			PNG_LIBPNG_VER_STRING, NULL, NULL, &myPNGWarnFoo
		);

		if(refTex->pngptrData == NULL){
			WARN("Unable to create png struct");
			fclose(handFile);
			continue;
		}

		if(setjmp(png_jmpbuf(refTex->pngptrData)) != 0){ 	/** jumps here on errors. */
			cleanupPNGImg(
				refTex->pngptrData,
				&refTex->dynarrRows
			);
			png_destroy_read_struct(
				&(refTex->pngptrData),
				&(refTex->pngptrInfo),
				NULL
			);
			refTex->h = refTex->w = 0;

		}else{
			refTex->pngptrInfo = png_create_info_struct(
				refTex->pngptrData
			);

			if(refTex->pngptrInfo == NULL){
				WARN("Unable to create png info struct");
				fclose(handFile);
				continue;
			}

			/** setup PNG decode */
			png_init_io(refTex->pngptrData, handFile);
			png_set_sig_bytes(refTex->pngptrData, PNGHEAD_SIZE);
			png_read_info(refTex->pngptrData, refTex->pngptrInfo);

			png_byte colorCurrent = png_get_color_type(
				refTex->pngptrData, refTex->pngptrInfo
			);

			png_set_filler(refTex->pngptrData, 0, PNG_FILLER_AFTER);
			png_set_packing(refTex->pngptrData);	/** if < 8 bits */

			{
				png_color_8p sig_bit;
				if (png_get_sBIT(refTex->pngptrData, refTex->pngptrInfo, &sig_bit))
					png_set_shift(refTex->pngptrData, sig_bit);
			}

			switch(colorCurrent){
				case PNG_COLOR_TYPE_RGB_ALPHA:
					/** already good */
				break;
				case PNG_COLOR_TYPE_RGB:
					png_set_tRNS_to_alpha(refTex->pngptrData);
				break;
				case PNG_COLOR_TYPE_PALETTE:
					png_set_palette_to_rgb( refTex->pngptrData );
					png_set_tRNS_to_alpha(refTex->pngptrData);
				break;
				case PNG_COLOR_TYPE_GRAY:
					png_set_expand_gray_1_2_4_to_8( refTex->pngptrData );
					png_set_tRNS_to_alpha(refTex->pngptrData);
				break;
				default:
					WARN("unsupported colour");
				break;
			}

			png_read_update_info(refTex->pngptrData, refTex->pngptrInfo);

			colorCurrent = png_get_color_type(
				refTex->pngptrData, refTex->pngptrInfo
			);

			if(colorCurrent != PNG_COLOR_TYPE_RGB_ALPHA){
				printf("<warning> %s didn't convert to the right colour type\n", files->dynarrFiles[idxFile]);
				png_destroy_read_struct(&refTex->pngptrData, &refTex->pngptrInfo, (png_infopp)NULL);
				fclose(handFile);
				continue;
			}

			refTex->colorType = colorCurrent;

			const int numPasses = png_set_interlace_handling( refTex->pngptrData );

			/** setup other data */
			copyString(&refTex->name, files->dynarrFiles[idxFile]);

			/** setup texture */
			unsigned int row;
			const png_uint_32 sizeRow = png_get_rowbytes( refTex->pngptrData, refTex->pngptrInfo );

			refTex->w = png_get_image_width(
				refTex->pngptrData, refTex->pngptrInfo
			);
			refTex->h = png_get_image_height(
				refTex->pngptrData, refTex->pngptrInfo
			);

			if(sizeRow/refTex->w != DEFAULT_BYTE_PP){
				WARN("%s didn't convert to the right bit depth", refTex->name);
				png_destroy_read_struct(&refTex->pngptrData, &refTex->pngptrInfo, (png_infopp)NULL);
				fclose(handFile);
				continue;
			}

			refTex->dynarrRows = calloc_chk((refTex->h +1), sizeof(png_byte*));
			refTex->dynarrRows[refTex->h] = NULL;

			for(row = 0; row < refTex->h; ++row)
				refTex->dynarrRows[row] = png_malloc(refTex->pngptrData, sizeRow);

			unsigned int pass;
			for(pass=0; pass < numPasses; ++pass){
				for(row = 0; row < refTex->h; ++row){
					png_read_rows(
						refTex->pngptrData,
						&refTex->dynarrRows[row],
						NULL,
						1
					);
				}
			}

			png_read_end(refTex->pngptrData, refTex->pngptrInfo);
		}

		fclose(handFile);
	}
	return NOPROB;
}

errCode sortTextures(sTex **arrSortMe, sSeqList *outSeqs, sStillList *outStills){
	char buff[128];
	sTexSeq *currentSeq=NULL;
	sTex *refTex;
	bool makeSeq = FALSE;

	if(arrSortMe == NULL || outSeqs == NULL || outStills==NULL){
		WARN("bad arguments");
		return ERROR;
	}

	size_t i;
	for(i=0; arrSortMe[i] != NULL; ++i){
		refTex = arrSortMe[i];

		if(refTex == NULL){
			WARN("null in texture array.");
			continue;
		}

		if(refTex->name == '\0'){
			WARN("texture has no name.");
			continue;
		}

		if(refTex->w == 0 || refTex->h == 0){
			WARN("texture %s has bad dimensions.", refTex->name);
			continue;
		}

		if(currentSeq == NULL){
			if(arrSortMe[i+1] != NULL){
				imgFNameToWSearch(refTex->name, buff, 128);
			
				if(hasWildStr(arrSortMe[i+1]->name, buff) == TRUE){
					makeSeq = TRUE;
				}
			}

		}else{
			if(hasWildStr(refTex->name, buff) == FALSE)
				currentSeq = NULL;
			
			if(currentSeq == NULL && arrSortMe[i+1] != NULL){
				imgFNameToWSearch(refTex->name, buff, 128);
				
				if(hasWildStr(arrSortMe[i+1]->name, buff) == TRUE){
					makeSeq = TRUE;
				}
			}
		}
		
		if(makeSeq == TRUE){
			++outSeqs->num;
			outSeqs->dynarrSeqs = realloc_chk(
				outSeqs->dynarrSeqs, 
				outSeqs->num * sizeof(sTexSeq)
			);
			currentSeq = &outSeqs->dynarrSeqs[outSeqs->num -1];
			memset(currentSeq, 0, sizeof(sTexSeq));
			makeSeq = FALSE;
		}

		if(currentSeq == NULL){
			++outStills->num;
			outStills->dynarrTexIDs = realloc_chk(
				outStills->dynarrTexIDs,
				(outStills->num) * sizeof(unsigned int)
			);
			outStills->dynarrSheetIDs = realloc_chk(
				outStills->dynarrSheetIDs,
				(outStills->num) * sizeof(unsigned int)
			);
			outStills->dynarrTexIDs[outStills->num -1] = i;
			outStills->dynarrSheetIDs[outStills->num -1] = -1;
			
		}else{
			if(currentSeq->num > 0){
				sTex *firstFrame = arrSortMe[ currentSeq->dynarrTexIDs[0] ];
				if(refTex->w != firstFrame->w || refTex->h != firstFrame->h){
					WARN("Frame %s has a different size to other frames.", arrSortMe[i]->name);
					return ERROR;
				}
			}

			++currentSeq->num;
			currentSeq->dynarrTexIDs = realloc_chk(
				currentSeq->dynarrTexIDs,
				(currentSeq->num) * sizeof(unsigned int)
			);
			currentSeq->dynarrSheetIDs = realloc_chk(
				currentSeq->dynarrSheetIDs,
				(currentSeq->num) * sizeof(unsigned int)
			);
			currentSeq->dynarrTexIDs[currentSeq->num -1] = i;
			currentSeq->dynarrSheetIDs[currentSeq->num -1] = -1;
		}
	}
	return NOPROB;
}

errCode arrangeTextures(
	sTex **arrTexs,
	sSeqList *pSeqs, 
	sStillList *pStills,
	sFontList *pFonts,
	sSheetList *pOutSheets,
	unsigned int maxSquare
){
	unsigned int curW, curH;
	int idxFit;
	bool makeSheet, freshSheet;
	unsigned int curSeq, curStill, curFrame, curFont;
	sTex *curTex;
	sSheet *curSheet;
	sTexSeq *refSeq;
	sSquare *refSqr;
	sFont *refFnt;
	unsigned int *dynarrSeqIdxs;
	unsigned int *dynarrPrevFrame;	/** If not -1, we're trying to split an animation over multiple sheets */ 
	unsigned int *dynarrStillIdxs;
	unsigned int *dynarrFontIdxs;
	
	if(pSeqs == NULL || pStills == NULL || pFonts == NULL)
		return ERROR;

	/** Arrays used to keep track of things that still need assigning */
	if(pSeqs->num > 0){
		dynarrSeqIdxs = calloc_chk(pSeqs->num, sizeof(unsigned int));
		dynarrPrevFrame = calloc_chk(pSeqs->num, sizeof(unsigned int));

		unsigned int i;
		for(i=0; i < pSeqs->num; ++i){
			dynarrSeqIdxs[i] = i;
			dynarrPrevFrame[i] = (unsigned int)-1;
		}
		
	}else{
		dynarrSeqIdxs = NULL;
		dynarrPrevFrame = NULL;
	}
	
	if(pStills->num > 0){
		dynarrStillIdxs = calloc_chk(pStills->num, sizeof(unsigned int));
		
		unsigned int i;
		for(i=0; i < pStills->num; ++i)
			dynarrStillIdxs[i] = i;
		
	}else{
		dynarrStillIdxs = NULL;
	}

	if(pFonts->num > 0){
		dynarrFontIdxs = calloc_chk(pFonts->num, sizeof(unsigned int));

		unsigned int i;
		for(i=0; i < pFonts->num; ++i)
			dynarrFontIdxs[i] = 0;
	}else{
		dynarrFontIdxs = NULL;
	}
	
	/** Main arrangement stuff */
	do{
		sListSquares holes;	memset(&holes, 0, sizeof(sListSquares));

		++pOutSheets->num;
		pOutSheets->dynarrSheets = realloc_chk(
			pOutSheets->dynarrSheets, 
			pOutSheets->num * sizeof(sSheet*)
		);
		curSheet = malloc_chk(sizeof(sSheet));
		memset(curSheet, 0, sizeof(sSheet));
		pOutSheets->dynarrSheets[ pOutSheets->num -1 ] = curSheet;
		
		makeSheet = FALSE;
		freshSheet = TRUE;
		curSeq =curStill =curFont =0; /** retry all the differed stills and sequences*/
		curW =curH =0;
	
		XTRA_LOG("Making sheet %i", (int)pOutSheets->num);
		
		while(makeSheet==FALSE && (dynarrSeqIdxs != NULL || dynarrStillIdxs != NULL || dynarrFontIdxs != NULL) ){
			const bool endOfSeqs = (curSeq < pSeqs->num) ? FALSE : TRUE;
			const bool endOfStills = (curStill < pStills->num) ? FALSE : TRUE;
			const bool endOfFonts = (curFont < pFonts->num) ? FALSE : TRUE;
			
			//consolidate(&holes); /** a bit broken */

			if(dynarrSeqIdxs != NULL && endOfSeqs == FALSE){ 
				if(dynarrSeqIdxs[curSeq] != (unsigned int)-1){
					refSeq = &(pSeqs->dynarrSeqs[ dynarrSeqIdxs[curSeq] ]);

					if(refSeq->num > 0){	/** See if the entire sequence can fit first. */
						{
							sListSquares tmpHoles;	memset(&tmpHoles, 0, sizeof(sListSquares));

							XTRA_LOG("Probing sheet");
							copySquares(&tmpHoles, &holes);

							if(dynarrPrevFrame[curSeq] != (unsigned int)-1)
								curFrame = dynarrPrevFrame[curSeq];
							else
								curFrame = 0;

							idxFit = (unsigned int)-1;
							while(curFrame < refSeq->num){
								curTex = arrTexs[ refSeq->dynarrTexIDs[curFrame] ];

								if(curTex->w > maxSquare || curTex->h > maxSquare){	/** We can't do much else here besides bomb out, because we can't mark the texture as a dud here. */
									ERROR_LOG("arrangeTextures: Texture %s is too large for the max texture size.", curTex->name);
									cleanupListSquares(&holes);
									cleanupListSquares(&tmpHoles);
									goto arrangeTextures_fail;
								}

								idxFit = fitNFillSquare(&tmpHoles, curTex->w, curTex->h, maxSquare, maxSquare);
								if(idxFit < 0)
									break;
							
								++curFrame;
							}

							cleanupListSquares(&tmpHoles);
							XTRA_LOG("Done with probe");
						}

						if(idxFit >= 0 || freshSheet == TRUE){
							refSeq->dynarrSheetIDs = calloc_chk(refSeq->num, sizeof(unsigned int));

							if(dynarrPrevFrame[curSeq] != (unsigned int)-1)
								curFrame = dynarrPrevFrame[curSeq];
							else
								curFrame = 0;

							while(curFrame < refSeq->num){
								curTex = arrTexs[ refSeq->dynarrTexIDs[curFrame] ];
								idxFit = fitNFillSquare(&holes, curTex->w, curTex->h, maxSquare, maxSquare);
								if(idxFit >= 0){
									refSqr = &(holes.dynarrSquares[idxFit]);
									refSeq->dynarrSheetIDs[curFrame] = (unsigned int)(pOutSheets->num -1);
									curTex->x = refSqr->x;
									curTex->y = refSqr->y;

									++curSheet->num;
									curSheet->dynarrTexIDs = realloc_chk(
										curSheet->dynarrTexIDs,
										curSheet->num *sizeof(unsigned int)
									);
									curSheet->dynarrTexIDs[curSheet->num -1] = refSeq->dynarrTexIDs[curFrame];
									freshSheet = FALSE;

								}else{	/** Try again on a fresh sheet. */
									dynarrPrevFrame[curSeq] = curFrame;
									break;
								}
								++curFrame;
							}
						}
					}else{
						curFrame = 0;
					}

					if(curFrame == refSeq->num)	/** success */
						dynarrPrevFrame[curSeq] = dynarrSeqIdxs[curSeq] = (unsigned int)-1;
				}
				++curSeq;
				
			
			}else if(dynarrStillIdxs != NULL && endOfStills == FALSE){
				if(dynarrStillIdxs[curStill] != (unsigned int)-1){
					curTex = arrTexs[ 
						pStills->dynarrTexIDs[
							dynarrStillIdxs[curStill]
						] 
					];
					
					if(pStills->dynarrSheetIDs == NULL)
						pStills->dynarrSheetIDs = calloc_chk(pStills->num, sizeof(unsigned int));
							
					if(curTex->w > maxSquare || curTex->h > maxSquare){	/** We can't do much else here besides bomb out, because we can't mark the texture as a dud here. */
						ERROR_LOG("arrangeTextures: Texture %s is too large for the max texture size.", curTex->name);
						cleanupListSquares(&holes);
						goto arrangeTextures_fail;
					}
				
					idxFit = fitNFillSquare(&holes, curTex->w, curTex->h, maxSquare, maxSquare);
					if(idxFit >= 0){
						refSqr = &(holes.dynarrSquares[idxFit]);
						curTex->x = refSqr->x;
						curTex->y = refSqr->y;
						pStills->dynarrSheetIDs[curStill] = (unsigned int)(pOutSheets->num -1);
						curSheet->dynarrTexIDs = realloc_chk(
							curSheet->dynarrTexIDs, 
							(curSheet->num +1) * sizeof(unsigned int)
						);
						curSheet->dynarrTexIDs[curSheet->num] = pStills->dynarrTexIDs[ dynarrStillIdxs[curStill] ];
						++curSheet->num;
						freshSheet = FALSE;

						dynarrStillIdxs[curStill] = (unsigned int)-1;

					}else if(freshSheet == TRUE){
						ERROR_LOG("Unable to fit still");
						goto arrangeTextures_fail;
					}

				}
				++curStill;

			}else if(dynarrFontIdxs != NULL && endOfFonts == FALSE){
				if(dynarrFontIdxs[curFont] != (unsigned int)-1){
					unsigned int idxGlyph;

					refFnt = pFonts->dynarrFonts[curFont];
					idxFit = (unsigned int)-1;

					{	/** probe sheet */
						sListSquares tmpHoles;	memset(&tmpHoles, 0, sizeof(sListSquares));

						XTRA_LOG("Probing sheet");
						copySquares(&tmpHoles, &holes);

						for(idxGlyph= dynarrFontIdxs[curFont]; idxGlyph < refFnt->num; ++idxGlyph){
							curTex = arrTexs[ refFnt->dynarrTexIDs[idxGlyph] ];
							idxFit = fitNFillSquare(&tmpHoles, curTex->w, curTex->h, maxSquare, maxSquare);
							if(idxFit < 0)
								break;
						}

						cleanupListSquares(&tmpHoles);
						XTRA_LOG("Done with probe");
					}

					if(idxFit >= 0 || freshSheet == TRUE){
						unsigned int i = dynarrFontIdxs[curFont];

						while(dynarrFontIdxs[curFont] < idxGlyph){
							curTex = arrTexs[
								refFnt->dynarrTexIDs[
									dynarrFontIdxs[curFont]
								]
							];
							idxFit = fitNFillSquare(&holes, curTex->w, curTex->h, maxSquare, maxSquare);
							if(idxFit < 0){
								ERROR_LOG("Unable to fit font, somehow?");
								goto arrangeTextures_fail;
							}

							refSqr = &(holes.dynarrSquares[idxFit]);
							curTex->x = refSqr->x;
							curTex->y = refSqr->y;
							++dynarrFontIdxs[curFont];
						}

						if(dynarrFontIdxs[curFont] -i > 0){
							unsigned int numPrev = curSheet->num;
							refFnt->dynarrSheetIDs = realloc_chk(
								refFnt->dynarrSheetIDs,
								dynarrFontIdxs[curFont] *sizeof(unsigned int)
							);

							unsigned int s;
							for(s=i; s < dynarrFontIdxs[curFont]; ++s)
								refFnt->dynarrSheetIDs[s] = (unsigned int)(pOutSheets->num -1);

							curSheet->num += dynarrFontIdxs[curFont] -i;
							curSheet->dynarrTexIDs = realloc_chk(
								curSheet->dynarrTexIDs,
								curSheet->num *sizeof(unsigned int)
							);
		
							memcpy(
								&curSheet->dynarrTexIDs[numPrev],
								&refFnt->dynarrTexIDs[i],
								(curSheet->num -numPrev) *sizeof(unsigned int)
							);
						}else{
							WARN("fonts not added to sheet");
						}

						if(dynarrFontIdxs[curFont] == refFnt->num)
							dynarrFontIdxs[curFont] = (unsigned int)-1;

						freshSheet = FALSE;
					}
				}
				++curFont;

			}else{	/** Check if we need to make another sheet, and cleanup any arrays that are finished. */

				if(dynarrSeqIdxs != NULL && endOfSeqs == TRUE){
					for(curSeq=0; curSeq < pSeqs->num; ++curSeq){
						if(dynarrSeqIdxs[curSeq] != (unsigned int)(-1))
							break;
					}

					if(curSeq == pSeqs->num){
						SAFE_DELETE(dynarrSeqIdxs);
						SAFE_DELETE(dynarrPrevFrame);
					}else{
						makeSheet = TRUE;
					}
				}

				if(dynarrFontIdxs != NULL && endOfFonts == TRUE){
					for(curFont=0; curFont < pFonts->num; ++curFont){
						if(dynarrFontIdxs[curFont] != (unsigned int)-1)
							break;
					}

					if(curFont == pFonts->num){
						SAFE_DELETE(dynarrFontIdxs);
					}else{
						makeSheet = TRUE;
					}
				}

				if(dynarrStillIdxs != NULL && endOfStills == TRUE){
					for(curStill=0; curStill < pStills->num; ++curStill){
						if(dynarrStillIdxs[curStill] != (unsigned int)(-1))
							break;
					}

					if(curStill == pStills->num){
						SAFE_DELETE(dynarrStillIdxs);
					}else{
						makeSheet = TRUE;
					}
				}
			}
			
			/** keep the sheet up to date. */
			curSheet->w = holes.boundryW;
			curSheet->h = holes.boundryH;
		}

		cleanupListSquares(&holes);
		
	}while(makeSheet==TRUE);
	
	if(dynarrStillIdxs != NULL || dynarrSeqIdxs != NULL){
		ERROR_LOG("Didn't cleanup memory");
		return ERROR;
	}
	
	return NOPROB;
	
arrangeTextures_fail:
	
	SAFE_DELETE(dynarrSeqIdxs);
	SAFE_DELETE(dynarrStillIdxs);
	SAFE_DELETE(dynarrPrevFrame);
	return ERROR;
}



errCode genMan(
	const char *strManName,
	const sSeqList *refSeqs,
	const sStillList *refStills,
	const sFontList *refFonts,
	const sSheetList *refSheets,
	const sTex **refarrTexs,
	sManifest *outMan
){
	static const size_t LENBUFF = 256;

	sTex const *refTex;	
	char buff[LENBUFF];
	unsigned int i;

	outMan->refSeqs = refSeqs;
	outMan->refStills = refStills;
	outMan->refFonts = refFonts;
	outMan->refSheets = refSheets;

	for(i=0; i < refSheets->num; ++i){
		snprintf(buff, LENBUFF, "%s%i", strManName, i);
		copyString(&refSheets->dynarrSheets[i]->name, buff);
	}

	if(refStills->num > 0){
		char *refDelim;
		size_t posDelim;

		outMan->dynarrStrStillNames = calloc(refStills->num, sizeof(char*));
		memset(outMan->dynarrStrStillNames, 0, refStills->num *sizeof(char*));
		for(i=0; i < refStills->num; ++i){
			refTex = refarrTexs[ refStills->dynarrTexIDs[i] ];
			refDelim = strrchr(refTex->name, '.');
			posDelim = (refDelim != NULL && (refDelim - refTex->name) / sizeof(char) < LENBUFF)
					? (refDelim - refTex->name) / sizeof(char)
					: LENBUFF
			;
			memcpy(buff, refTex->name, posDelim);
			buff[posDelim] = '\0';
			sanitiseString(buff);
			copyString(&outMan->dynarrStrStillNames[i], buff);
		}
	}

	if(refSeqs->num > 0){
		outMan->dynarrStrSeqNames = calloc_chk(refSeqs->num, sizeof(char*));
		memset(outMan->dynarrStrSeqNames, 0, refSeqs->num *sizeof(char*));
		for(i=0; i < refSeqs->num; ++i){
			if(refSeqs->dynarrSeqs[i].num == 0)
				continue;

			refTex = refarrTexs[ refSeqs->dynarrSeqs[i].dynarrTexIDs[0] ];
			imgFNameToWSearch(refTex->name, buff, LENBUFF);
			copyWithoutChars(&(outMan->dynarrStrSeqNames[i]), buff, "#*");
			sanitiseString(outMan->dynarrStrSeqNames[i]);
		}
	}

	if(refFonts->num > 0){
		char const *refCCodeDelim;

		outMan->dynarrStrFontNames = calloc_chk(refFonts->num, sizeof(char*));
		memset(outMan->dynarrStrFontNames, 0, refFonts->num * sizeof(char*));

		for(i=0; i < refFonts->num; ++i){
			refTex = refarrTexs[ refFonts->dynarrFonts[i]->dynarrTexIDs[0] ];
			if(refFonts->dynarrFonts[i]->num == 0)
				continue;

			refCCodeDelim = strrchr(refTex->name, (unsigned int)'_');
			if(refCCodeDelim == NULL)
				continue;

			size_t lenName = (size_t)( refCCodeDelim - refTex->name ) / sizeof(char) ;
			
			outMan->dynarrStrFontNames[i] = calloc_chk(lenName +1, sizeof(char));
			memcpy(
				outMan->dynarrStrFontNames[i],
				refTex->name, 
				lenName *sizeof(char)
			);
			outMan->dynarrStrFontNames[i][lenName] = '\0';
			sanitiseString(outMan->dynarrStrFontNames[i]);
		}
	}

	return NOPROB;
}


errCode writeSheets(
	const char *strPath, 
	const char *strManName,
	sTex **refArrTex,
	sSheetList *pSheets
){
	char buff[128];
	size_t s, r, i;
	FILE *handFile;
	png_byte **dynarrImg;
	png_structp pngptrWriteData;
	png_infop pngptrWriteInfo;
	png_size_t sizeRow;
	unsigned int w, h;
	
	for(s=0; s < pSheets->num; ++s){
		handFile = NULL;
		dynarrImg = NULL;
		pngptrWriteData = NULL;
		pngptrWriteInfo = NULL;
		
		w = pSheets->dynarrSheets[s]->w;
		h = pSheets->dynarrSheets[s]->h;
		
		if(w==0 || h==0){
			WARN("bad sheet dimensions.");
			continue;
		}
		
		snprintf(buff, 128, "%s%s%i.png", 
			(strPath!=NULL) ? strPath : "", 
			strManName, 
			(int)s
		);
		handFile = fopen(buff, "wb");
		
		if(handFile == NULL){
			WARN("Unable to write to file %s", buff);
			goto fail_loop_writeSheets;
		}
				
		pngptrWriteData = png_create_write_struct(
			PNG_LIBPNG_VER_STRING, NULL, NULL, myPNGWarnFoo
		);
		
		if(pngptrWriteData == NULL){
			WARN("Can't make png write data");
			goto fail_loop_writeSheets;
		}
		
		pngptrWriteInfo = png_create_info_struct(pngptrWriteData);

		if(pngptrWriteInfo == NULL){
			WARN("Can't make png write info");
			goto fail_loop_writeSheets;
		}
		
		if(setjmp (png_jmpbuf (pngptrWriteData))){
			goto fail_loop_writeSheets;
		}
		
		png_set_IHDR(
			pngptrWriteData, pngptrWriteInfo,
			w, h,
			DEFAULT_BITDEPTH, DEFAULT_COLOURTYPE, DEFAULT_INTERLACE,
			PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
		);

		png_set_gamma(pngptrWriteData, 2.2, 1.0/2.2);
		
		sizeRow = png_get_rowbytes( pngptrWriteData, pngptrWriteInfo );
		dynarrImg = calloc_chk( (h +1), sizeof(png_byte*) );
		dynarrImg[h] = NULL;
		
		for(r=0; r < h; ++r){
			dynarrImg[r] = png_malloc(pngptrWriteData, sizeRow);
			memset(dynarrImg[r], 0, sizeRow);
		}
		
		for(i=0; i < pSheets->dynarrSheets[s]->num; ++i){
			if(readTexToSheet(
				refArrTex[
					pSheets->dynarrSheets[s]->dynarrTexIDs[i]
				], 
				dynarrImg, 
				w, 
				h,
				sizeRow
			)==PROBLEM)
				goto fail_loop_writeSheets;
		}
		
		XTRA_LOG("About to write %s\n", buff);
		png_init_io(pngptrWriteData, handFile);
		png_set_rows(pngptrWriteData, pngptrWriteInfo, dynarrImg);
		png_write_info(pngptrWriteData, pngptrWriteInfo);
		png_write_image(pngptrWriteData, dynarrImg);
		png_write_end(pngptrWriteData, pngptrWriteInfo);
		
	fail_loop_writeSheets:
		if(pngptrWriteInfo != NULL && pngptrWriteData != NULL){
			if(dynarrImg != NULL)
				cleanupPNGImg(pngptrWriteData, &dynarrImg);
			
			png_destroy_write_struct(&pngptrWriteData, &pngptrWriteInfo);
		}
		
		if(handFile != NULL)
			fclose(handFile);
	}


	return NOPROB;
}

void appendTexArr(sTex ***pAppendHere, sTex **pFrom){
	if(pAppendHere	== NULL)
		return;

	unsigned int lenH, lenF, i;
	
	for(lenF = 0; pFrom != NULL && pFrom[lenF] != NULL; ++lenF)
		;

	if(lenF == 0)
		return;

	for(lenH = 0; (*pAppendHere) != NULL && (*pAppendHere)[lenH] != NULL; ++lenH)
		;

	(*pAppendHere) = realloc_chk((*pAppendHere), sizeof(sTex*) * (lenF +lenH +1));

	for(i=0; i < lenF; ++i)
		(*pAppendHere)[i +lenH] = pFrom[i];

	(*pAppendHere)[lenF +lenH] = NULL;
}

void cleanupTex(sTex *pTex){
	SAFE_DELETE(pTex->name);
	cleanupPNGImg(pTex->pngptrData, &(pTex->dynarrRows) );
	png_destroy_read_struct(
		&(pTex->pngptrData),
		&(pTex->pngptrInfo),
		(png_infopp)NULL
	);
}

void cleanupTextures(sTex ***textures){
	if(textures == NULL || (*textures) == NULL)
		return;

	int idx = 0;
	while((*textures)[idx] != NULL){
		cleanupTex((*textures)[idx]);
		SAFE_DELETE((*textures)[idx]);
		++idx;
	}
	SAFE_DELETE(*textures);
}

void cleanupSeqList(sSeqList *list){
	size_t i;
	for(i=0; i < list->num; ++i){
		SAFE_DELETE(list->dynarrSeqs[i].dynarrTexIDs);
		SAFE_DELETE(list->dynarrSeqs[i].dynarrSheetIDs);
	}
	SAFE_DELETE(list->dynarrSeqs);
	memset(list, 0, sizeof(sSeqList));
}

void cleanupStillList(sStillList *list){
	if(list == NULL)
		return;

	SAFE_DELETE(list->dynarrTexIDs);
	SAFE_DELETE(list->dynarrSheetIDs);
	memset(list, 0, sizeof(sStillList));
}

void cleanupSheetList(sSheetList *list){
	if(list == NULL)
		return;

	unsigned int i;
	for(i=0; i < list->num; ++i){
		SAFE_DELETE(list->dynarrSheets[i]->name);
		SAFE_DELETE(list->dynarrSheets[i]->dynarrTexIDs);
	}

	SAFE_DELETE(list->dynarrSheets);
	memset(list, 0, sizeof(list));
}

void cleanupFontList(sFontList *list){
	if(list == NULL)
		return;

	sFont *refFnt;
	unsigned int i;
	for(i=0; i < list->num; ++i){
		refFnt = list->dynarrFonts[i];
		SAFE_DELETE(refFnt->dynarrTexIDs);
		SAFE_DELETE(refFnt->dynarrSheetIDs);
		SAFE_DELETE(refFnt->dynarrCharcodes);
		SAFE_DELETE(refFnt->dynarrOffsetY);
	}

	memset(list, 0, sizeof(list));
}

void cleanupManifest(sManifest *man){
	unsigned i;

	if(man->refStills != NULL){
		for(i=0; i <  man->refStills->num; ++i){
			SAFE_DELETE(man->dynarrStrStillNames[i]);
		}
	}
	SAFE_DELETE(man->dynarrStrStillNames);

	if(man->refSeqs != NULL){
		for(i=0; i <  man->refSeqs->num; ++i){
			SAFE_DELETE(man->dynarrStrSeqNames[i]);
		}
	}
	SAFE_DELETE(man->dynarrStrSeqNames);

	if(man->refFonts != NULL){
		for(i=0; i < man->refFonts->num; ++i)
			SAFE_DELETE(man->dynarrStrFontNames[i]);
	}
	SAFE_DELETE(man->dynarrStrFontNames);

	memset(man, 0, sizeof(sManifest));

}

void imgFNameToWSearch(const char *fName, char *outBuff, unsigned int sizeOutBuff){

	if(fName == NULL) goto FAIL_imgFNameToWSearch;

	size_t end = strlen(fName);

	if(end == 0) goto FAIL_imgFNameToWSearch;

	--end;

	while(end > 0 && fName[end] != '.')
		--end;

	if(end == 0) goto FAIL_imgFNameToWSearch;

	--end; /* go past . character */

	while(end > 0 && (unsigned short)(fName[end]) >= asciNumStart && (unsigned short)(fName[end]) <= asciNumEnd )
		--end;

	if(end == 0) goto FAIL_imgFNameToWSearch;

	if(end >= sizeOutBuff-2) goto FAIL_imgFNameToWSearch;

	memcpy(outBuff, fName, sizeof(char) * (end+1));
	memcpy(&outBuff[end+1], "#*\0", 3);

	return;

FAIL_imgFNameToWSearch:
	outBuff[0] = '\0';
}

