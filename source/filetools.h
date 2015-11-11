#ifndef FILETOOLS_H
#define FILETOOLS_H

#include "strtools.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/*!\brief */
typedef struct defsFileList{
	char **dynarrFiles;
	size_t num;
} sFileList; 

	/*!\brief Used for config file lines */
typedef struct defCfgFileEntry{
	char *strName;
	char *strValue;
} sCfgFileEntry;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	/* !\brief Gets everything before the last / or makes *outputStr = NULL. */
void getBaseDir(char **outputStr, const char *strFullPath);

	/*!\brief	Fills the info inside the file list with a dynamic array of file names. Is not recursive.
	 *!\param	strTargetDir	Path to directory to search
	 *!\param	output		
	 *!\param	ignoreFiles	A series of files to ignore. Can be null.
	 *!\param	strGetOnly	Limits the files to search term. Can be null.
	 *!\return	Non zero if it fails horribly. Returns true even if nothing is found.
	 */
errCode getFiles(
	const char *strTargetDir, sFileList *output, const char *ignoreFiles, const char *strGetOnly
);

	/*!\brief	Gets all the directories nested inside the given base directory.
	 *!param	strBaseDir	The directory to find other nested directories.
	 *!param	dynarrDirs	Allocates a null terminated dynamic array of string which are the relative paths of these directories.
	 */
errCode getDirs(
	const char *strBaseDir, char ***dynarrDirs
);

	/*!\brief 	Generates a null terminated list of entries from a config file.
	 *!\note	A config file has any number of lines that has a 'name' and a 'value' that are separated by a delineator.
	 *!\note	Ignores lines that have no delineator.
	 */
errCode genConfigFileEntries(
	const char *strFile, sCfgFileEntry ***dynarrOut, char delineator
);

	/*!\brief Cleans up the file list, duh!	*/
errCode cleanupFileList(sFileList *files);

	/*!\brief cleanup list of file entries. */
errCode cleanupCfgFileEntries(sCfgFileEntry ***dynarrEntries);

#endif
