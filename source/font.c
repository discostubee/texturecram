#include "font.h"
#include <ft2build.h>

#ifdef FREETYPE_2
#	include <freetype/ftglyph.h>
#else
#	include <freetype2/ftglyph.h>
#endif

#include FT_FREETYPE_H

const char * FONT_SEARCH_PATTERN = "*.txt";
const char * FONT_FILEHEADER = "make font file";
const char   FONT_CONFIG_DELIM = '=';
const char * FONT_FILE = "file";
const char * FONT_SIZE = "size";
const char * FONT_COLOUR = "colour";

sFontInfo** genFontInfos(const char *strDir, const char *strIgnores){
	char buff[256];
	unsigned int i, lenFts = 0;
	sFontInfo **dynarrFts = NULL;
	sFileList files;	memset(&files, 0, sizeof(sFileList));

	getFiles(strDir, &files, strIgnores, FONT_SEARCH_PATTERN);
	for(i=0; i < files.num; ++i){
		sCfgFileEntry **entries = NULL;
		sprintf(buff, "%s/%s", strDir, files.dynarrFiles[i]); 
		if(genConfigFileEntries(buff, &entries, FONT_CONFIG_DELIM) == NOPROB){
			if(entries != NULL){
				sFontInfo *fnt = malloc_chk(sizeof(sFontInfo));
				
				memset(fnt, 0, sizeof(sFontInfo));
				memset(fnt->colour, (char)-1, sizeof(char) *3);
				copyStringUntil(&fnt->strName, files.dynarrFiles[i], '.');

				unsigned int e;
				for(e=0; entries[e] != NULL; ++e){
					if(strcmp(FONT_FILE, entries[e]->strName)==0){
						snprintf(buff, 246, "%s/%s", strDir, entries[e]->strValue);
						copyString(&fnt->strFile, buff);
					}
					else if(strcmp(FONT_SIZE, entries[e]->strName)==0){
						fnt->size = atoi(entries[e]->strValue);
					}
					else if(strcmp(FONT_COLOUR, entries[e]->strName)==0){
						size_t end;
						short channel;
						char const * delim;
						char const * prevDelim = entries[e]->strValue;

						for(channel=0; channel < 3; ++channel){
							delim = strchr(prevDelim, channel==2 ?'\0' :'.');
							if(delim != NULL){
								end = (delim - prevDelim) / sizeof(char);
								strncpy(buff, prevDelim, end);
								buff[end] = '\0';
								fnt->colour[channel] = (char)atoi(buff);
								prevDelim = &delim[1];
							}else{
								XTRA_LOG("Bad colour format");
								break;
							}
						}
					}
				}

				if(fnt->strFile != NULL && fnt->size > 0){
					++lenFts;
					dynarrFts = realloc_chk(dynarrFts, sizeof(sFontInfo*) * (lenFts +1));
					dynarrFts[lenFts -1] = fnt;
					dynarrFts[lenFts] = NULL;
				}else{
					cleanupFontInfo(fnt);
				}
			}
		}
		cleanupCfgFileEntries(&entries);	
	}
	cleanupFileList(&files);
	return dynarrFts;
}


errCode genTexFromFonts(sFontInfo **pArrFntIfo, sTex ***pDynarrOutTex, sFontList *pOutList){
	static const size_t MAX_STRBUFF = 256, MAX_FNTNAME = 128, MAX_CHARCODE = 127;

	if(pArrFntIfo == NULL || pDynarrOutTex == NULL)
		return PROBLEM;

	sTex *		refTexCur =NULL;
	unsigned int	lenCur =0;
	unsigned int	lenArrTex =0;
	size_t		lenFntName;
	FT_Face		face;
	FT_Glyph	glyph;
	FT_BitmapGlyph  bitmap;
	FT_UInt		idxGlyph;
	FT_ULong	charcode;
	char		strBuff[MAX_STRBUFF];
	unsigned int	r, c;
	sFontInfo **	itr;

	FT_Library handFF;
	if(FT_Init_FreeType(&handFF) != 0){
		WARN("Can't initialise free type library");
		return PROBLEM;
	}

	for(itr = pArrFntIfo; *itr != NULL; ++itr){

		if(FT_New_Face(handFF, (*itr)->strFile, 0, &face) != 0)
			goto genTexFromFonts_fail;

		if(FT_Select_Charmap(face, FT_ENCODING_UNICODE) != 0)
			goto genTexFromFonts_fail;

		if(face->num_glyphs == 0)
			goto genTexFromFonts_fail;

		(*itr)->numGlyphs = face->num_glyphs;

		lenFntName = strlen((*itr)->strName);
		if(lenFntName > MAX_FNTNAME -1)
			goto genTexFromFonts_fail;

		memcpy(strBuff, (*itr)->strName, lenFntName * sizeof(char)); /** leaving room in the buffer for the ending. */
		strBuff[lenFntName] = '_';
		++lenFntName;

		{
			const FT_F26Dot6 size = (*itr)->size *64;
			const FT_UInt dpi = 300; /** 300 is a common DPI for photos. */
			if(FT_Set_Char_Size(face, size, size, dpi, dpi) != 0){
				WARN("Unable to set size for font %s", (*itr)->strName);
				continue;
			}
		}

		const unsigned int lenOrig = lenArrTex;
		lenCur = 0;
		lenArrTex += face->num_glyphs;
		(*pDynarrOutTex) = realloc_chk((*pDynarrOutTex), (lenArrTex +1) * sizeof(sTex*));
		memset(&(*pDynarrOutTex)[lenOrig], 0, (face->num_glyphs +1) * sizeof(sTex*)); /* ensure the end, and any fails are null. Don't worry if there are spare slots. These will be used up on the next reallocation. */
		(*itr)->dynarrCharcodes = calloc_chk(face->num_glyphs, sizeof(unsigned int));
		(*itr)->dynarrOffsetY = calloc_chk(face->num_glyphs, sizeof(unsigned int));
	
		for( 	charcode = FT_Get_First_Char(face, &idxGlyph); 
			idxGlyph != 0;
			charcode = FT_Get_Next_Char(face, charcode, &idxGlyph)
		){
			if(lenOrig + lenCur >= lenArrTex)
				goto genTexFromFonts_failloop;

			refTexCur = malloc_chk(sizeof(sTex));
			memset(refTexCur, 0, sizeof(sTex));

			(*itr)->dynarrCharcodes[lenCur] = charcode;
			snprintf(&strBuff[lenFntName], MAX_CHARCODE, "%lu", charcode);
			copyString(&refTexCur->name, strBuff);

			(*itr)->dynarrOffsetY[lenCur] = (face->glyph->metrics.vertBearingY) / 64;

			if(FT_Load_Glyph(face, idxGlyph, FT_LOAD_DEFAULT) != 0){
				WARN("failed to load glyph %i, for font %s", lenCur, (*itr)->strName);
				goto genTexFromFonts_failloop;
			}

			if(FT_Get_Glyph(face->glyph, &glyph) != 0){
				WARN("Failed to get glyph %i, from font %s", lenCur, (*itr)->strName);
				goto genTexFromFonts_failloop;
			}
		
			if(FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1) != 0){
				WARN("Failed to render glyph %i, from font %s", lenCur, (*itr)->strName);
				goto genTexFromFonts_failloop;
			}
		
			bitmap = (FT_BitmapGlyph)glyph;

			switch(bitmap->bitmap.pixel_mode){
				case FT_PIXEL_MODE_GRAY:
				case FT_PIXEL_MODE_LCD:
					break;

				default:
					WARN("Mode %i unsupported. Font %s", bitmap->bitmap.pixel_mode, (*itr)->strName);
					FT_Done_Glyph(glyph);
					goto genTexFromFonts_failloop;
			}

			refTexCur->w = bitmap->bitmap.width;
			refTexCur->h = bitmap->bitmap.rows;

			if(refTexCur->w != 0 && refTexCur->h != 0){	/** it's not unusual for there to be glyphs with no graphics, so still add it to the array (a null would terminate it). */
				refTexCur->dynarrRows = calloc_chk(refTexCur->h, sizeof(png_byte*));

				for(r=0; r < refTexCur->h; ++r){
					refTexCur->dynarrRows[r] = calloc_chk(refTexCur->w, 4);

					for(c=0; c < refTexCur->w; ++c){
						switch(bitmap->bitmap.pixel_mode){
							case FT_PIXEL_MODE_GRAY:
								memset(
									&refTexCur->dynarrRows[r][c*4],
									bitmap->bitmap.buffer[(r *bitmap->bitmap.pitch) +c],
									sizeof(char) *4
								);
								break;

							case FT_PIXEL_MODE_LCD:
								memcpy(
									&refTexCur->dynarrRows[r][c*4],
									&bitmap->bitmap.buffer[(r *bitmap->bitmap.pitch) +(c*3)],
									sizeof(char) *3
								);
								refTexCur->dynarrRows[r][(c*4) +3] = (refTexCur->dynarrRows[r][(c*4)] /3)
									+ (refTexCur->dynarrRows[r][(c*4) +1] /3)
									+ (refTexCur->dynarrRows[r][(c*4) +2] /3)
								;
								break;
						}

						refTexCur->dynarrRows[r][(c*4)]    /= 255 -(*itr)->colour[0] +1;
						refTexCur->dynarrRows[r][(c*4) +1] /= 255 -(*itr)->colour[1] +1;
						refTexCur->dynarrRows[r][(c*4) +2] /= 255 -(*itr)->colour[2] +1;
					}

				}

				refTexCur->pngptrData = NULL;	/** these shouldn't matter at the moment because they aren't needed for copying onto the sprite. */
				refTexCur->pngptrInfo = NULL;
				refTexCur->colorType = 0;

				(*pDynarrOutTex)[lenOrig +lenCur] = refTexCur;
				++lenCur;
			}

			FT_Done_Glyph(glyph);

			continue;

		genTexFromFonts_failloop: ;
			cleanupTex(refTexCur);
			free(refTexCur);
			WARN("Char %i failed", lenCur);
		}

		if(lenCur > 0){
			(*itr)->numLoaded = lenCur;
			XTRA_LOG("Success with font: %s, with %i characters\n", (*itr)->strFile, lenCur);
		}else{
			WARN("No glyphs loaded from font %s", (*itr)->strFile);
		}


	genTexFromFonts_fail: ;

		lenArrTex -= face->num_glyphs - lenCur;	/** trim bad characters */
		FT_Done_Face(face);
	}

	return NOPROB;
}

errCode genFontFromInfo(const sFontInfo **refInfo, sFontList *pOutList, unsigned int startTexIdx){
	unsigned int i, j;
	sFont *refFnt;

	if(refInfo == NULL || pOutList == NULL)
		return ERROR;

	for(i=0; refInfo[i] != NULL; ++i)
		++pOutList->num;

	pOutList->dynarrFonts = calloc_chk(pOutList->num, sizeof(sFont*));

	for(i=0; i < pOutList->num; ++i){
		refFnt = pOutList->dynarrFonts[i] = malloc_chk(sizeof(sFont));
		memset(refFnt, 0, sizeof(sFont));
		refFnt->num = refInfo[i]->numLoaded;
		if(refFnt->num == 0)
			continue;

		refFnt->dynarrTexIDs = calloc_chk(refFnt->num, sizeof(unsigned int));
		refFnt->dynarrCharcodes = calloc_chk(refFnt->num, sizeof(unsigned int));
		memcpy(
			refFnt->dynarrCharcodes,
			refInfo[i]->dynarrCharcodes,
			refFnt->num *sizeof(unsigned int)
		);
		refFnt->dynarrOffsetY = calloc_chk(refFnt->num, sizeof(unsigned int));
		memcpy(
			refFnt->dynarrOffsetY,
			refInfo[i]->dynarrOffsetY,
			refFnt->num *sizeof(unsigned int)
		);

		for(j=0; j < refFnt->num; ++j){
			refFnt->dynarrTexIDs[j] = startTexIdx;
			++startTexIdx;
		}
	}

	return NOPROB;
}

void cleanupFontInfos(sFontInfo ***pDynarrFonts){
	if(pDynarrFonts == NULL || *pDynarrFonts == NULL)
		return;

	sFontInfo **itr;

	for(itr = *pDynarrFonts; *itr != NULL; ++itr){
		cleanupFontInfo(*itr);
	}

	SAFE_DELETE(*pDynarrFonts);
}

void cleanupFontInfo(sFontInfo *font){
	if(font == NULL)
		return;

	SAFE_DELETE(font->strName);
	SAFE_DELETE(font->strFile);
	SAFE_DELETE(font->dynarrCharcodes);
	free(font);
}
