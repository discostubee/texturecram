#include "utils.h"

#include <stdlib.h>
#include <memory.h>

char gbuff[512];

const char *STR_WARN_FORMAT = "<warning> %s [%s : %i]\n";
const char *STR_ERROR_FORMAT = "<fatal error> %s [%s : %i]\n";

static const char refstrMemFault[] = "Out of memory\n";
void memcheck(const void *memptr){
	if(memptr == NULL){
		printf(refstrMemFault);
		exit(1);
	}
}

unsigned int closestPow2(unsigned int num){
	unsigned a, p=0;
	do{
		a = 1 << ++p;
	}while(a <= num);
	return a;
}

errCode eoe(errCode in, const char *file, unsigned int line){
	if(in == ERROR){
		printf("<FAIL> Got error code at '%s' at line %i.\n", file, line);
		exit(1);
	}
	
	return in;
}


void* malloc_chk(size_t memsize){
	void* tmp = malloc(memsize);
	memcheck(tmp);
	return tmp;
}

void* realloc_chk(void *reallocMe, size_t memsize){
	void* tmp = realloc(reallocMe, memsize);
	memcheck(tmp);
	return tmp;
}

void* calloc_chk(size_t num, size_t elementSize){
	void* tmp = calloc(num, elementSize);
	memcheck(tmp);
	return tmp;
}

void pushListUint(sListUint *pushTo, unsigned int pushMe){
	if(pushTo == NULL)
		return;
	
	++pushTo->num;
	pushTo->arr = realloc(pushTo->arr, pushTo->num * sizeof(unsigned int));
	pushTo->arr[ pushTo->num-1 ] = pushMe;
}

void snipListUint(sListUint *snipMe, unsigned int idxSnip){
	if(snipMe == NULL)
		return;

	if(idxSnip >= snipMe->num)
		return;

	if(snipMe->num == 1){
		SAFE_DELETE(snipMe->arr);
		snipMe->num = 0;
		return;
	}

	unsigned int *arrOrig = snipMe->arr;
	snipMe->arr = calloc_chk(snipMe->num -1, sizeof(unsigned int));

	if(idxSnip > 0)
		memcpy(snipMe->arr, arrOrig, idxSnip * sizeof(unsigned int));

	if(idxSnip < snipMe->num -1)
		memcpy(&snipMe->arr[idxSnip], &arrOrig[idxSnip +1], (snipMe->num -idxSnip -1) * sizeof(unsigned int));

	--snipMe->num;

	free(arrOrig);
}

void cleanupListUint(sListUint *cleanMe){
	if(cleanMe == NULL)
		return;
	
	SAFE_DELETE(cleanMe->arr);
	memset(cleanMe, 0, sizeof(sListUint));
}
