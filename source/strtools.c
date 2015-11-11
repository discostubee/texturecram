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

bool hasWildStr(const char *findIn, const char *wildStr){

	char nextNonWild = '\0';
	unsigned int idxA=0, idxB=0;

	if(findIn == NULL || wildStr == NULL)
		return 0;

	unsigned int lenA = strlen(findIn);
	unsigned int lenB = strlen(wildStr);

	if(lenA == 0 || lenB == 0)
		return 0;

	while(idxA < lenA && idxB < lenB){
		if(wildStr[idxB]=='*'){

			while(idxB < lenB && wildStr[idxB]=='*'){
				++idxB;
			}
			if(idxB == lenB) /** We've made it this far, so we've found everything. */
				return 1;

			nextNonWild = wildStr[idxB];

		}else if(wildStr[idxB]=='#'){
			nextNonWild = '#';
			++idxB;
		}

		if(nextNonWild != '\0'){
			if(nextNonWild=='#'){
				if(
					findIn[idxA] == wildStr[idxB]
					|| (wildStr[idxB]=='*' && (findIn[idxA] < asciNumStart || findIn[idxA] > asciNumEnd))
				){
					nextNonWild = '\0';
					++idxB;

					if(idxA==0 || findIn[idxA-1] < asciNumStart || findIn[idxA-1] > asciNumEnd)	/* We expect at least 1 number */
						return 0;

				}else if(findIn[idxA] < asciNumStart || findIn[idxA] > asciNumEnd){
					return 0;
				}

			}else if(findIn[idxA] == nextNonWild){
				nextNonWild = '\0';
				++idxB;
			}

		}else if(findIn[idxA] != wildStr[idxB]){	/** rewind B and try again. */
			while(idxB > 0 && wildStr[idxB] != '*' && wildStr[idxB] != '#')
				--idxB;

			if(idxB == 0 && wildStr[0] != '*' && wildStr[0] != '#')	/** Didn't start with wild. */
				return 0;
		}else{
			nextNonWild = '\0';
			++idxB;
		}
		++idxA;
	}

	if(wildStr[idxB] == '*')
		return TRUE;
	else if(nextNonWild == '\0' && findIn[idxA-1] == wildStr[idxB-1])	/** B doesn't end with a wild, but it ends in a match with A. */
		return TRUE;

	return FALSE;
}

void copyString(char **strGoesHere, const char *copyMe){
	size_t len = strlen(copyMe);
	(*strGoesHere) = calloc(len + 1, sizeof(char));
	strcpy(*strGoesHere, copyMe);
}

void copyStringUntil(char **strGoesHere, const char *copyMe, char until){
	const char *found = strrchr(copyMe, (int)until);

	if(found == NULL){
		copyString(strGoesHere, copyMe);
	}else{
		size_t len = (size_t)(found - copyMe) / sizeof(char);
		(*strGoesHere) = calloc(len +1, sizeof(char));
		strncpy(*strGoesHere, copyMe, len);
		(*strGoesHere)[len] = '\0';
	}
}

bool isNum(char c){
	return !(c < asciNumStart || c > asciNumEnd);
}

compare higherString(const char *higher, const char *lower){
	if(higher == NULL && lower == NULL)
		return SAME;

	if(higher == NULL)
		return LESS;

	if(lower == NULL)
		return GREATER;
		
	size_t ih=0;
	size_t il=0;
	size_t lenH = strlen(higher);
	size_t lenL = strlen(lower);

	while(ih < lenH && il < lenL){
		if(isNum(higher[ih]) && isNum(lower[il])){
			size_t sh=ih;
			size_t sl=il;

			//while(higher[sh]=='0')  ++sh;
			//while(lower[sl]=='0')	++sl;

			for(ih=sh; higher[ih]!=0 && isNum(higher[ih]); ++ih);
			for(il=sl; lower[il]!=0 && isNum(lower[il]); ++il);

			int nh = atoi(&higher[sh]);
			int nl = atoi(&lower[sl]);
			
			if(nh > nl)
				return GREATER;
			else if(nl > nh)
				return LESS;
			
		}else if(higher[ih] > lower[il]){
			return GREATER;
		}else if(lower[il] > higher[ih]){
			return LESS;
		}else{
			++ih;
			++il;
		}
	}

	if(ih < lenH)
		return LESS;
	else if(il < lenL)
		return GREATER;

	return SAME;
}

void sortStrings(char **dynarrStrings, size_t count){
	char **dynarr = calloc(count, sizeof(char*));
	memset(dynarr, 0, sizeof(char*)*count);

	size_t cur, scan;
	char *bubble;
	for(cur=0; cur < count; ++cur){
		scan=0;
		bubble = dynarrStrings[cur];

		while(scan < cur){
			if(dynarr[scan]==NULL)
				break;

			if(higherString(bubble, dynarr[scan]) == GREATER){
				char *tmp = bubble;
				bubble = dynarr[scan];
				dynarr[scan] = tmp;
			}

			++scan;
		}

		dynarr[scan] = bubble;
	}

	memcpy(dynarrStrings, dynarr, sizeof(char*)*count);
	free(dynarr);
}

void sanitiseString(char *opInPlace){
	size_t i=0;
	size_t len=strlen(opInPlace);
	while(i<len){
		switch(opInPlace[i]){
			case ' ':
			case '"':
			case '\'':
			case ')':
			case '(':
			case '-':
			case '.':
			case ',':
				opInPlace[i]='_';
				break;
		}
		++i;
	}
}


void getOutputNameFromFullPath(char **outputStr, const char *strFullPath){
	if(outputStr == NULL || strFullPath == NULL)
		return;

	const char *tmpStart = strrchr(strFullPath, (int)'/');

	if(tmpStart == NULL)
		tmpStart = strFullPath;
	else
		tmpStart = tmpStart + sizeof(char);	/* skip past '/' */

	const char *tmpEnd = strrchr(tmpStart, (int)'.');

	if(tmpEnd == NULL)
		tmpEnd = strrchr(tmpStart, (int)'\0');

	size_t lenPackName = (size_t)((long)tmpEnd - (long)tmpStart) +1;

	if(*outputStr!=NULL)
		free(*outputStr);

	*outputStr = (char*)malloc(sizeof(char) * lenPackName);

	memcpy( *outputStr, tmpStart, sizeof(char) * (lenPackName-1) );
	(*outputStr)[lenPackName-1] = '\0';
}

void copyWithoutChars(char **strGoesHere, const char *strCopyMe, const char *strNotThese){
	if(strGoesHere == NULL || strCopyMe == NULL)
		return;
	
	if(strNotThese == NULL){
		copyString(strGoesHere, strCopyMe);
		return;
	}
		
	size_t lenOut=0;
	size_t lenIn, lenNot, lenCopy;
	for(lenIn=0; strCopyMe[lenIn] != '\0'; ++lenIn){

		for(lenCopy=0; strCopyMe[lenIn + lenCopy] != '\0'; ++lenCopy){
			for(lenNot=0; strNotThese[lenNot] != '\0'; ++lenNot){
				if(	strCopyMe[lenIn + lenCopy] == strNotThese[lenNot] )
					goto copywithoutchars_stop;
			}
		}
		copywithoutchars_stop:
		
		if(lenCopy > 0){
			*strGoesHere = realloc(*strGoesHere, sizeof(char) * (lenOut + lenCopy + 1));
			memcpy( &(*strGoesHere)[lenOut], &strCopyMe[lenIn], sizeof(char) * lenCopy );
			lenOut += lenCopy;
			(*strGoesHere)[lenOut] = '\0';
			lenIn = lenCopy;
		}
	}
}
