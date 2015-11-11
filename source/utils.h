
/*
 *
 *  Copyright (C) 2012 Stuart Bridgens
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
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define SAFE_DELETE(x) { if(x!=NULL) free(x); x = NULL; }
#define EOE(code) eoe(code, __FILE__, __LINE__)

extern char gbuff[512];
extern const char *STR_WARN_FORMAT;
extern const char *STR_ERROR_FORMAT;

#define WARN(...) {\
	snprintf(gbuff, 512, __VA_ARGS__);\
	printf(STR_WARN_FORMAT, gbuff, __FILE__, __LINE__);\
}

#define LOG(...) {\
	snprintf(gbuff, 512, __VA_ARGS__);\
	printf("%s\n", gbuff);\
}

#define ERROR_LOG(...){\
	snprintf(gbuff, 512, __VA_ARGS__);\
	printf(STR_ERROR_FORMAT, gbuff, __FILE__, __LINE__);\
}

#ifdef DEBUG
#	define DBUG_LOG(...) {\
			snprintf(gbuff, 512, __VA_ARGS__);\
			printf("%s [%s : %i]\n", gbuff, __FILE__, __LINE__);\
		}
#	define DBUG_WARN(...) WARN(__VA_ARGS__)
#	ifdef XTRA_LOGGING
#		define XTRA_LOG(...) LOG(__VA_ARGS__)
#	endif
#else
#	define DBUG_LOG(...)
#	define DBUG_WARN(...)
#endif

#ifndef XTRA_LOG
#	define XTRA_LOG(...)
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum defErrCode{
	NOPROB=0,
	PROBLEM,
	ERROR
} errCode;

typedef enum defBool{
	FALSE=0, TRUE
} bool;

typedef struct defListUint{
	unsigned int *arr;
	size_t num;
} sListUint;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*!\brief	Checks if a null pointer, and exits if it is.*/
void memcheck(const void *memptr);

/*!\brief	*/
unsigned int closestPow2(unsigned int num);

/*!\brief	Exit On Error: Prints message and exits the program if it gets an ERROR. Otherwise it passes the other error codes out. */
errCode eoe(errCode in, const char *file, unsigned int line);

/*!\brief	Same as normal malloc, except it checks the memory and exits if it fails. */
void* malloc_chk(size_t memsize);

/*!\brief	Similar to malloc_chk */
void* realloc_chk(void *reallocMe, size_t memsize);

/*!\brief	Similar to malloc_chk */
void* calloc_chk(size_t num, size_t elementSize);

/*!\brief	Add an element to the end of the list. */
void pushListUint(sListUint *pushTo, unsigned int pushMe);

/*!\brief	remove an element from the list and rejoin the 2 halves. */
void snipListUint(sListUint *snipMe, unsigned int idxSnip);

/*!\brief	*/
void cleanupListUint(sListUint *cleanMe);

#endif
