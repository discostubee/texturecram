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
 */

#include "filetools.h"


#if defined __APPLE__
	#include <CoreFoundation/CoreFoundation.h>
	#include <CoreServices/CoreServices.h>
#elif defined __linux
	#include <glib.h>
	#include <gio/gio.h>
	#include <glib/gstdio.h>
#endif

void getBaseDir(char **outputStr, const char *strFullPath){
	if(outputStr==NULL || strFullPath==NULL)
		return;

	int lenFullPath = (int)strlen(strFullPath);
	if(*outputStr!=NULL){
		free(outputStr);
		*outputStr=NULL;
	}

	while(lenFullPath>0 && strFullPath[lenFullPath]!='/' && strFullPath[lenFullPath]!='\\')
		--lenFullPath;

	if(lenFullPath<=0)
		return;

	if(strFullPath[lenFullPath]=='.')	//- If we only have 1 level and it's the containing dir.
		return;

	*outputStr = (char*)calloc(lenFullPath+2, sizeof(char));

	memcpy(*outputStr, strFullPath, (lenFullPath+1)*sizeof(char));
	(*outputStr)[lenFullPath+1]='\0';
}

#ifdef __APPLE__

errCode getFiles(const char *strTargetDir, sFileList *output, const char *ignoreFiles, const char *strGetOnly){
	errCode rtn = NOPROB;

	if(output == NULL)
		return PROBLEM;

	output->num = 0;
	output->dynarrFiles = NULL;

	if(strTargetDir == NULL)
		return PROBLEM;

	const size_t lenTargetDir = strlen(strTargetDir);

	CFURLRef dirURL = CFURLCreateFromFileSystemRepresentation(
		NULL, (const unsigned char *)strTargetDir, lenTargetDir, true
	);

	SInt32 error;

	CFArrayRef files = (CFArrayRef)CFURLCreatePropertyFromResource(NULL, dirURL, kCFURLFileDirectoryContents, &error); 

	if(error != 0 || files == NULL){
		rtn = PROBLEM;
		goto GETFILES_END;
	}

	char buff[256];

	CFIndex numFiles = CFArrayGetCount(files);
	while(numFiles > 0){
		--numFiles;

		CFURLRef tmp = (CFURLRef)CFArrayGetValueAtIndex(files, numFiles); /** There should be no need to free or retain this (no multithread) */
		if(true == CFURLHasDirectoryPath(tmp))
			continue;

		if(false == CFURLGetFileSystemRepresentation(tmp, false, (unsigned char*)buff, 256))
			continue;

		if(ignoreFiles != NULL && TRUE == hasWildStr(buff, ignoreFiles))
			continue;

		if(strGetOnly != NULL && FALSE == hasWildStr(buff, strGetOnly))
			continue;

		++output->num;
		output->dynarrFiles = (char**)realloc(output->dynarrFiles, output->num * sizeof(char*));
		
		char** strmem =  &output->dynarrFiles[ output->num-1 ];
		*strmem = NULL;
		copyString(strmem, buff);
	}

	CFRelease(files);

	sortStrings(output->dynarrFiles, output->num);

GETFILES_END:
	CFRelease(dirURL);

	return rtn;
}

errCode getDirs(const char *strBaseDir, char ***dynarrDirs){
	errCode rtn = NOPROB;

	if(strBaseDir==NULL)
		return PROBLEM;

	if(dynarrDirs==NULL)
		return PROBLEM;

	*dynarrDirs = NULL;

	const size_t lenDir = strlen(strBaseDir);

	CFURLRef dirURL = CFURLCreateFromFileSystemRepresentation(
		NULL, (const unsigned char *)strBaseDir, lenDir, true
	);

	SInt32 error;

	CFArrayRef files = (CFArrayRef)CFURLCreatePropertyFromResource(
		NULL, dirURL, kCFURLFileDirectoryContents, &error
	); 

	if(error != 0 || files == NULL){
		rtn = PROBLEM;
		goto GETDIRS_END;
	}

	char buff[256];
	size_t numDirs = 0;

	CFIndex numFiles = CFArrayGetCount(files);
	while(numFiles > 0){
		--numFiles;

		CFURLRef url = (CFURLRef)CFArrayGetValueAtIndex(files, numFiles); /** There should be no need to free or retain this (no multithread) */
		if(	true == CFURLHasDirectoryPath(url)
			&& true == CFURLGetFileSystemRepresentation(url, false, (unsigned char*)buff, 256)
		){
			++numDirs;
			*dynarrDirs = realloc(*dynarrDirs, (numDirs + 1) * sizeof(char*));
			(*dynarrDirs)[ numDirs - 1 ] = NULL;
			copyString( &((*dynarrDirs)[ numDirs - 1 ]), buff);
			(*dynarrDirs)[ numDirs ] = NULL;
		}
	}

	CFRelease(files);


GETDIRS_END:
	CFRelease(dirURL);

	return rtn;

}


void printAppleErrors(SInt32 code){
	switch(code){
	case kCFURLUnknownError: printf("unkown error\n"); break;
	case kCFURLUnknownSchemeError: printf("unknown scheme\n"); break;
	case kCFURLResourceNotFoundError: printf("resource not found\n"); break;
	case kCFURLResourceAccessViolationError: printf("resource access\n"); break;
	case kCFURLRemoteHostUnavailableError: printf("remote host\n"); break;
	case kCFURLImproperArgumentsError: printf("improper argument\n"); break;
	case kCFURLUnknownPropertyKeyError: printf("property key\n"); break;
	case kCFURLPropertyKeyUnavailableError: printf("key unavailable\n"); break;
	case kCFURLTimeoutError: printf("timeout\n"); break;
	default: printf("unknown code\n"); break;
	}
}

#elif defined __linux

static errCode getFSEntries(const char *strTargetDir, sFileList *output, const char *ignoreFiles, const char *strGetOnly, bool getDirs){
	GError *errors=NULL;
	gchar const **encodingList=NULL; /** Do NOT free */
	gchar const *nativeEncoding=NULL;	/** Do NOT free */
	gsize charsConverted=0;
	gsize lengthNative=0;
	GDir *handDir=NULL;	/**use glib to free */
	size_t lenPlatformFName=0;
	gchar const *refName = NULL;	/** Do NOT free */
	GStatBuf stats;
	gchar *platformFName=NULL;	/** free me */

	const size_t lenTargetDir = strlen(strTargetDir);

	if(strTargetDir == NULL)
		return PROBLEM;

	g_get_filename_charsets(&encodingList);

	if(encodingList==NULL || encodingList[0]==NULL)
		return PROBLEM;

	g_get_charset(&nativeEncoding);

	if(nativeEncoding==NULL)
		return PROBLEM;

	platformFName = g_convert(
		(const gchar*)strTargetDir, (const gssize)lenTargetDir,
		encodingList[0], nativeEncoding,
		&charsConverted, &lengthNative, &errors
	);

	if(platformFName==NULL)
		return PROBLEM;
	
	handDir = g_dir_open(platformFName, 0, &errors);
	
	do{
		char *nativeFName=NULL;

		refName = g_dir_read_name(handDir);	/** Do not free */

		if(refName==NULL)
			break;

		{
			bool goodEntry = FALSE;
			gchar *fullPath = g_build_filename(platformFName, refName, NULL);		
			
			goodEntry = (g_file_test(fullPath, G_FILE_TEST_IS_DIR) == getDirs);

			g_free(fullPath);

			if(goodEntry == FALSE)
				continue;
		}

		lenPlatformFName = strlen(refName);

		nativeFName = (char*)g_convert(	/** nativeFName should be freed by the output file list */
			refName, lenPlatformFName,
			nativeEncoding, encodingList[0],
			&charsConverted, &lengthNative, &errors
		);

		if(ignoreFiles != NULL && hasWildStr(nativeFName, ignoreFiles)==TRUE){	/** Skip ignore file */
			SAFE_DELETE(nativeFName);
			continue;
		}

		if(strGetOnly != NULL && hasWildStr(nativeFName, strGetOnly)==FALSE){
			SAFE_DELETE(nativeFName);
			continue;
		}

		++output->num;
		output->dynarrFiles = (char**)realloc(output->dynarrFiles, sizeof(char*) * output->num);
		output->dynarrFiles[output->num-1] = nativeFName;

	}while(refName!=NULL);

	g_dir_close(handDir);
	g_free(platformFName);

	sortStrings(output->dynarrFiles, output->num);

	return NOPROB;
}


errCode getFiles(const char *strTargetDir, sFileList *output, const char *ignoreFiles, const char *strGetOnly){
	return getFSEntries(strTargetDir, output, ignoreFiles, strGetOnly, FALSE);
}

errCode getDirs(const char *strBaseDir, char ***dynarrDirs){
	if(dynarrDirs==NULL)
		return PROBLEM;

	sFileList dirs;		memset(&dirs, 0, sizeof(sFileList));
	if(getFSEntries(strBaseDir, &dirs, NULL, NULL, TRUE)==PROBLEM){
		cleanupFileList(&dirs);
		return PROBLEM;
	}
	
	*dynarrDirs = dirs.dynarrFiles;
	*dynarrDirs = realloc(*dynarrDirs, sizeof(char*) * (dirs.num + 1));
	(*dynarrDirs)[dirs.num] = NULL;

	return NOPROB;
}


#endif

errCode genConfigFileEntries(const char *strFile, sCfgFileEntry ***dynarrOut, char delineator){
	char buff[512];
	char *ptrDelim, *ptrTrim;
	FILE *f=fopen(strFile, "r");
	unsigned int numEntries = 0;
	sCfgFileEntry *ref;
	unsigned int sizeName, sizeValue;

	if(dynarrOut == NULL)
		return PROBLEM;

	SAFE_DELETE(*dynarrOut);

	if(f == NULL)
		return PROBLEM;

	while(fgets(buff, 512, f) != NULL && feof(f) == 0){
		ptrDelim = strrchr(buff, (int)delineator);
		if(ptrDelim != NULL){
			ref = malloc_chk(sizeof(sCfgFileEntry));
			memset(ref, 0, sizeof(sCfgFileEntry));

			sizeName = ptrDelim - (char*)buff;

			if(sizeName < 512)
			{
				ref->strName = malloc_chk(sizeName + sizeof(char));
				memcpy(ref->strName, buff, sizeName);
				ref->strName[sizeName] = '\0';
				
				ptrTrim = strrchr(&ptrDelim[1], (int)'\n');
				if(ptrTrim != NULL)
					sizeValue = (long)ptrTrim - (long)&ptrDelim[1];
				else
					sizeValue = strlen(&ptrDelim[1]) * sizeof(char);
				ref->strValue = malloc_chk(sizeValue + sizeof(char));
				memcpy(ref->strValue, &ptrDelim[1], sizeValue);
				ref->strValue[sizeValue] = '\0';
			}

			++numEntries;
			*dynarrOut = realloc_chk(*dynarrOut, (numEntries + 1) * sizeof(sCfgFileEntry*));
			(*dynarrOut)[numEntries - 1] = ref;
			(*dynarrOut)[numEntries] = NULL;
		}
	}
	return NOPROB;
}

errCode cleanupCfgFileEntries(sCfgFileEntry ***pEntries){
	if(pEntries == NULL || *pEntries == NULL)
		return NOPROB;

	sCfgFileEntry **itr;
	for(itr = *pEntries; *itr != NULL; ++itr){
		free((*itr)->strName);
		free((*itr)->strValue);
		free(*itr);
	}
	return NOPROB;
}

errCode cleanupFileList(sFileList *files){
	if(files == NULL)
		return NOPROB;

	if(files->dynarrFiles != NULL){
		while(files->num > 0){
			--files->num;
			SAFE_DELETE(files->dynarrFiles[files->num]);
		}
		SAFE_DELETE(files->dynarrFiles);
	}

	return NOPROB;
}

