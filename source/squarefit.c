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

#include "squarefit.h"

/*** RELIEF ***/

typedef enum {
	eUp, eDown, eLeft, eRight
} eDirection;

typedef struct defRelief{
	unsigned int start, dist;
} sRelief;

typedef struct defListRelief{
	sRelief *arr;
	unsigned int num;
	eDirection dir;
	unsigned int min, max;
} sListRelief;

/*!\brief	Returns false if it would introduce a gap. */
bool addRelief(sListRelief *addTo, sSquare *add){
	unsigned int numAdds=0;

	if(addTo == NULL)
		return FALSE;

	unsigned int s, d;
	switch(addTo->dir){
		case eUp:	s = add->x; d = add->y; break;
		case eDown:	s = add->x; d = add->y + add->h; break;
		case eLeft:	s = add->y; d = add->x; break;
		case eRight:	s = add->y; d = add->x + add->w; break;
		default: return FALSE;
	}

	unsigned int start;
	if(addTo->num == 0 || s < addTo->arr[0].start)
		start = 0;
	else
		for(start=0; start < addTo->num && s < addTo->arr[start].start; ++start);

	if(start == addTo->num){	/** append */
		++addTo->num;
		addTo->arr = realloc_chk(addTo->arr, addTo->num * sizeof(sRelief));

		sRelief *ref = &addTo->arr[ addTo->num-1 ];
		ref->start = s;
		ref->dist = d;
		numAdds = 1;

	}else{	/**  insert */
		int remain;
		int stop = start;		/** where in the original list do we stop with the new relief */
		sRelief *newRelief = NULL;	/** All the new relief created by the square */
		unsigned int end;
		unsigned int prev = addTo->arr[stop].dist;

		switch(addTo->dir){
			case eUp:		
			case eDown:
				remain = (int)add->w;
				end = add->x + add->w;
				break;

			case eLeft:
			case eRight:
				remain = (int)add->h;
				end = add->y + add->h;
				break;
			default: return FALSE;
		}


		if(start==0){
			newRelief = malloc_chk(sizeof(sRelief));
			newRelief[0].start = s;
			newRelief[0].dist = d;
			remain -= (int)addTo->arr[0].start -s;
			prev = d;
			++numAdds;
		}

		unsigned int nextD;
		while(remain > 0){
			nextD = addTo->arr[stop].dist;
			if(prev > d || nextD > d){	/** Don't bother if it's all below the new relief or the same */ 
				if(nextD > d)
					prev = nextD;
				else
					prev = d;

				++numAdds;
				newRelief = realloc_chk(newRelief, numAdds * sizeof(sRelief));
				newRelief[numAdds-1].start = addTo->arr[stop].start;
				newRelief[numAdds-1].dist = prev;
			}
			++stop;

			if(stop < addTo->num){
				remain -= addTo->arr[stop].start;

				if(remain < 0){		/** if our new relief cuts into the end */
					++numAdds;
					newRelief = realloc_chk(newRelief, numAdds * sizeof(sRelief));
					newRelief[numAdds-1].start = end;
					newRelief[numAdds-1].dist = addTo->arr[stop -1].dist;
				}
			}else{
				remain = 0;
			}
		}

		if(numAdds > 0){	/** stitch back together and cleanup */
			sRelief *newTotal = calloc_chk(numAdds +addTo->num -stop +start, sizeof(sRelief));
	
		
			if(start > 0)
				memcpy(newTotal, addTo->arr, start * sizeof(sRelief));

			memcpy(&newTotal[start], newRelief, numAdds * sizeof(sRelief));

			if(stop < addTo->num)
				memcpy(&newTotal[start +numAdds], &addTo->arr[stop], (addTo->num -stop) * sizeof(sRelief));

			SAFE_DELETE(addTo->arr);

			addTo->arr = newTotal;
			addTo->num = numAdds +addTo->num -stop +start;
		}

		SAFE_DELETE(newRelief);
	}
	
	{	/** manage min/max */
		unsigned int i;
		addTo->min = (unsigned int) -1;
		addTo->max = 0;
		for(i = 0; i < addTo->num; ++i){
			if(addTo->arr[i].dist > addTo->max)
				addTo->max = addTo->arr[i].dist;

			if(addTo->arr[i].dist < addTo->min)
				addTo->min = addTo->arr[i].dist;
		}
	}

	return TRUE;
}

void cleanupListRelief(sListRelief *clean){
	SAFE_DELETE(clean->arr);
	clean->num = 0;
}

void printRelief(const sListRelief *printMe){
	if(printMe == NULL)
		return;

	printf("Relief (%i, %i) ", printMe->min, printMe->max);
	switch(printMe->dir){
		case eUp: printf(" up"); break;
		case eDown: printf(" down"); break;
		case eLeft: printf(" left"); break;
		case eRight: printf(" right"); break;
	}

	unsigned int i;
	for(i = 0; i < printMe->num; ++i){
		printf(" (%i, %i)", printMe->arr[i].start, printMe->arr[i].dist);
	}
	printf("\n");
}

/*** SQUARES ***/

/** Works only on empty squares. */
errCode consolidate(sListSquares *squares){
	XTRA_LOG("-consolidate");

	if(squares == NULL){
		printf("<error> consolidate: squares are null\n");
		return ERROR;
	}

	sListUint unfilled; memset(&unfilled, 0, sizeof(unfilled));
	unsigned int i;
	for(i = 0; i < squares->num; ++i){
		if(squares->dynarrFills[i] == FALSE)
			pushListUint(&unfilled, i);
	}

#	ifdef XTRA_LOGGING
		printSquares(squares);
#	endif

	/** gather clusters */
	while(unfilled.num > 1){
		sListUint cluster; memset(&cluster, 0, sizeof(sListUint));
		sListUint branches; memset(&branches, 0, sizeof(sListUint));
		sListSquares addThese; memset(&addThese, 0, sizeof(sListSquares));
		sListUint remThese; memset(&remThese, 0, sizeof(remThese));
		sListRelief reliefU; memset(&reliefU, 0, sizeof(sListRelief));
		sListRelief reliefD; memset(&reliefD, 0, sizeof(sListRelief));
		sListRelief reliefL; memset(&reliefL, 0, sizeof(sListRelief));
		sListRelief reliefR; memset(&reliefR, 0, sizeof(sListRelief));
		unsigned int c;
		sSquare *refO, *refMe, *refYou;

		reliefU.dir = eUp;
		reliefD.dir = eDown;
		reliefL.dir = eLeft;
		reliefR.dir = eRight;
	
		/** Start from the top/left most square. */
		i = 0;
		refO = NULL;
		for(c = 0; c < unfilled.num; ++c){
			refMe = &squares->dynarrSquares[ unfilled.arr[c] ];
			if(refO == NULL || refMe->x + refMe->y < refO->x + refO->y){
				refO = &squares->dynarrSquares[ unfilled.arr[c] ];
				i = c;
			}
		}

		pushListUint(&branches, unfilled.arr[i]);
		pushListUint(&cluster, unfilled.arr[i]);
		snipListUint(&unfilled, i);

		refMe = &squares->dynarrSquares[branches.arr[0]];
		addRelief(&reliefU, refMe);
		addRelief(&reliefD, refMe);
		addRelief(&reliefL, refMe);
		addRelief(&reliefR, refMe);

		while(branches.num > 0){
			refMe = &squares->dynarrSquares[branches.arr[0]];
			for(c = 0; c < unfilled.num; ++c){
				refYou = &squares->dynarrSquares[ unfilled.arr[c] ];
				if(
					(	(refYou->x == refMe->x + refMe->w) 
						&&(	(refYou->y >= refMe->y && refYou->y <= refMe->y + refMe->h)
						     || (refYou->y + refYou->y >= refMe->y && refYou->y + refYou->y <= refYou->y + refYou->h)
					   	     || (refYou->y < refMe->y && refYou->y + refYou->h > refMe->y + refMe->h)
						)
					)||(	(refYou->y == refMe->y + refMe->h) 
						&&(	(refYou->x >= refMe->x && refYou->x <= refMe->x + refMe->w)
						    || (refYou->x + refYou->x >= refMe->x && refYou->x + refYou->x <= refYou->x + refYou->w)
						    || (refYou->x < refMe->x && refYou->x + refYou->w > refMe->x + refMe->w)
						)
					)
				){
					pushListUint(&branches, unfilled.arr[c]);
					pushListUint(&cluster, unfilled.arr[c]);
					snipListUint(&unfilled, c);
					addRelief(&reliefU, refYou);
					addRelief(&reliefD, refYou);
					addRelief(&reliefL, refYou);
					addRelief(&reliefR, refYou);
				}
			}
			snipListUint(&branches, 0);
		}
		cleanupListUint(&branches);

		if(cluster.num < 2)
			goto consolidate_endLoop;

		/** Add and remove squares. */
		{
			sSquare bign;

			bign.x = reliefL.max;
			bign.y = reliefU.min;
			bign.w = reliefR.min - bign.x;
			bign.h = reliefD.min - bign.y;

			/** detect if there are any unfilled squares creating donuts under the consolidated square. */

			/** carry on */
			addThese.boundryW = squares->boundryW;
			addThese.boundryH = squares->boundryH;
			EOE( addSquarePtr(&addThese, FALSE, &bign) );

			sSquare isec;
			for(c = 0; c < cluster.num; ++c){
				refYou = &squares->dynarrSquares[ cluster.arr[c] ];
				EOE( overlap(&bign, refYou, &isec) );

				if(isec.x == 0 || isec.y == 0 || isec.w == 0 || isec.h == 0)
					continue;

				EOE( addSquarePtr(&addThese, FALSE, &isec) );
				pushListUint(&remThese, cluster.arr[c]);

				if(refYou->w != isec.w && refYou->h != isec.h){
					if(refYou->x == isec.x){
						if(refYou->y == isec.y){
							addSquare(&addThese, FALSE, refYou->x,		isec.y + isec.h, refYou->w,		refYou->h - isec.h);
							addSquare(&addThese, FALSE, refYou->x + isec.w,	refYou->y,	 refYou->w - isec.w,	refYou->h);
						}else{
							addSquare(&addThese, FALSE, refYou->x, 		refYou->y,	 refYou->w,		refYou->h - isec.h);
							addSquare(&addThese, FALSE, refYou->x + isec.w, isec.y,		 refYou->w - isec.w,	refYou->h - isec.h);
						}
					}else{
						if(refYou->y == isec.y){
							addSquare(&addThese, FALSE, refYou->x,	refYou->y,		refYou->w - isec.w,	isec.h);
							addSquare(&addThese, FALSE, refYou->x,	refYou->y + isec.h,	refYou->w,		refYou->h - isec.h);
						}else{
							addSquare(&addThese, FALSE, refYou->x,	refYou->y,	refYou->w - isec.w,	isec.h);
							addSquare(&addThese, FALSE, refYou->x,	refYou->y,	refYou->w - isec.w,	isec.h);
						}
					}
				}else if(refYou->w == isec.w){
					if(refYou->y == isec.y){
						addSquare(&addThese, FALSE,	refYou->x,	refYou->y + isec.h,	refYou->w,	refYou->h - isec.h);
					}else{
						addSquare(&addThese, FALSE,	refYou->x,	refYou->y,		refYou->w,	refYou->h - isec.h);
					}
				}else if(refYou->h == isec.h){
					if(refYou->x == isec.x){
						addSquare(&addThese, FALSE,	refYou->x + isec.w,	refYou->y,	refYou->w - isec.w,	isec.h);
					}else{
						addSquare(&addThese, FALSE,	refYou->x,		refYou->y,	refYou->w - isec.w,	isec.h);
					}
				}
							
			}

			/** add it all in a big pot */
			if(addThese.num > 0){
				const unsigned int total = squares->num - remThese.num + addThese.num;
					
				sSquare *newSqu = calloc_chk(total, sizeof(sSquare));
				bool *newFill = calloc_chk(total, sizeof(bool));
				unsigned int numTmp = 0;

				unsigned int r;
				bool rem;
				for(c = 0; c < squares->num; ++c){
					rem = FALSE;
					for(r=0; r < remThese.num && rem == FALSE; ++r)
						if(c != remThese.arr[r])
							rem = TRUE;

					if(rem == FALSE){
						newSqu[ numTmp ] = squares->dynarrSquares[c];
						newFill[ numTmp ] = squares->dynarrFills[c];
						++numTmp;
					}
				}
			
				for(c = 0; c < addThese.num; ++c){
					newSqu[ numTmp ] = addThese.dynarrSquares[c];
					++numTmp;
				}
			
				free(squares->dynarrSquares);
				free(squares->dynarrFills);
				squares->dynarrSquares = newSqu;
				squares->dynarrFills = newFill;
				squares->num = numTmp;
			}
		}
	
	consolidate_endLoop:
		cleanupListSquares(&addThese);
		cleanupListUint(&remThese);
		cleanupListRelief(&reliefU);
		cleanupListRelief(&reliefD);
		cleanupListRelief(&reliefL);
		cleanupListRelief(&reliefR);
		cleanupListUint(&cluster);
	}

	cleanupListUint(&unfilled);

	return NOPROB;
}


errCode addSquare(sListSquares *squares, bool filled, unsigned int x, unsigned int y, unsigned int w, unsigned int h){
	if(squares == NULL){
		printf("<error> addSquare: addTo is NULL.\n");
		return ERROR;
	}
	
#	ifdef DEBUG
		if(w == 0 || h == 0){
			WARN("addSquare: height or width is zero.");
			return PROBLEM;
		}else if(x + w > squares->boundryW || y + h > squares->boundryH){
			WARN("addSquare: Square outside boundries (%i, %i, %i, %i) vs (%i, %i)",
				x, y, w, h,
				squares->boundryW, squares->boundryH
			);
			return PROBLEM;
		}else if(anyOverlaps(squares, x, y, w, h)){
			WARN("addSquare: Not adding square because it would overlap.");
			return PROBLEM;
		}else{
			XTRA_LOG("Square added (%i, %i, %i, %i) [%i, %i]", x, y, w, h, squares->boundryW, squares->boundryH);
		}
#	endif
	
	++squares->num;
	squares->dynarrSquares = realloc_chk(squares->dynarrSquares, squares->num * sizeof(sSquare));
	squares->dynarrFills = realloc_chk(squares->dynarrFills, squares->num * sizeof(bool));
	
	sSquare *tmp = &squares->dynarrSquares[squares->num-1];
	tmp->x = x;
	tmp->y = y;
	tmp->w = w;
	tmp->h = h;
	squares->dynarrFills[squares->num-1] = filled;

	return NOPROB;
}

errCode addSquarePtr(sListSquares *squares, bool filled, sSquare *addMe){
	if(addMe==NULL){
		printf("<error> addSquarePtr: addMe is null.\n");
		return ERROR;
	}
	return addSquare(squares, filled, addMe->x, addMe->y, addMe->w, addMe->h);
}

errCode overlap(const sSquare *a, const sSquare *b, sSquare *isec){
	if(a == NULL || b == NULL || isec == NULL)
		return PROBLEM;

	memset(isec, 0, sizeof(sSquare));

	/** test for the square falling wholing inside another */

	if(a->x >= b->x && a->x <= b->x +b->w){
		isec->x = a->x;
		if(a->x +a->w > b->x +b->w)
			isec->w = b->w -a->x +b->x;
		else
			isec->w = a->w;

	}else if(b->x >= a->x && b->x <= a->x +a->w){
		isec->x = b->x;
		if(b->x +b->w > a->x +a->w)
			isec->w = a->w -b->x +a->x;
		else
			isec->w = b->w;

	}else if(a->x <= b->x && a->x +a->w >= b->y +b->w){
		isec->x = b->x;
		isec->w = b->w;

	}else if(b->x <= a->x && b->x +b->w <= a->x +a->w){
		isec->x = a->x;
		isec->w = a->w;
	}

	if(isec->w == 0){
		if(a->x < b->x && a->x +a->w > b->x +b->w){
			isec->w = a->w;
			isec->x = a->x;
		}else if(b->x < a->x && b->x +b->w > a->x +a->w){
			isec->w = b->w;
			isec->x = b->x;
		}
	}

	if(a->x >= b->x && a->x <= b->x +b->w){
		isec->x = a->x;
		if(a->x +a->w > b->x +b->w)
			isec->w = b->w -a->x +b->x;
		else
			isec->w = a->w;

	}else if(b->x >= a->x && b->x <= a->x +a->w){
		isec->x = b->x;
		if(b->x +b->w > a->x +a->w)
			isec->w = a->w -b->x +a->x;
		else
			isec->w = b->w;

	}else if(a->y <= b->y && a->y +a->h >= b->y +b->h){
		isec->y = b->y;
		isec->h = b->h;

	}else if(b->y <= a->y && b->y +b->h <= a->y +a->h){
		isec->y = a->y;
		isec->h = a->h;
	}

	if(isec->h == 0){
		if(a->y < b->y && a->y +a->h > b->y +b->h){
			isec->h = b->h;
			isec->y = b->y;
		}else if(b->y < a->y && b->y +b->h > a->y +a->h){
			isec->h = a->h;
			isec->y = a->y;
		}
	}

	if(isec->w == 0 || isec->h == 0)
		memset(isec, 0, sizeof(sSquare));

	return NOPROB;
}

bool anyOverlaps(sListSquares *squares, unsigned int x, unsigned int y, unsigned int w, unsigned int h){
	size_t i;
	sSquare *tmp;
	for(i=0; i < squares->num; ++i){
		tmp = &squares->dynarrSquares[i];
		if( (
			    (x >= tmp->x 	&& x < tmp->x +tmp->w) 
			 || (x +w > tmp->x	&& x +w < tmp->x + tmp->w)
		) && (
			    (y >= tmp->y	&& y < tmp->y +tmp->h)
			 || (y +h > tmp->y	&& y +h < tmp->y +tmp->h)	
		) ){
			return TRUE;
		}
	}
	
	return FALSE;
}

bool anyOverlapsPtr(sListSquares *squares, sSquare *checkme){
	if(checkme==NULL){
		printf("<error> anyOverlapsPtr: checkme is null.\n");
		return ERROR;
	}
	return anyOverlaps(squares, checkme->x, checkme->y, checkme->w, checkme->h);
}

errCode incBoundries(sListSquares *squares, unsigned int w, unsigned int h){
	unsigned int origW = squares->boundryW, origH = squares->boundryH;

	squares->boundryW += w;
	squares->boundryH += h;

	if(squares == NULL){
		ERROR_LOG("squares is null.");
		return ERROR;
	}
	
	if(squares->num==0){
		if(w == 0 || h == 0){
			WARN("width and height need to be greater than 0");
			return PROBLEM;
		}

		addSquare(squares, FALSE, 0, 0, w, h);
		
	}else if(squares->num==1){

		if(squares->dynarrFills[0] == TRUE){
			sSquare tmp;

			memcpy(&tmp, &squares->dynarrSquares[0], sizeof(sSquare));

			if(h > 0 && w > 0)
				EOE( addSquare(squares, FALSE, tmp.w, tmp.h, w, h) );
			
			if(w > 0)
				EOE( addSquare(squares, FALSE, tmp.w, 0, w, tmp.h) );
			
			if(h > 0)
				EOE( addSquare(squares, FALSE, 0, tmp.h, tmp.w, h) );

		}else{
			sSquare *tmp = &squares->dynarrSquares[0];

			tmp->w += w;
			tmp->h += h;
		}
	}else{

#if 1
		sSquare addme;
		unsigned int startX = origW, startY = origH;

		if(w > 0 && h > 0){
			int corner = getCornerSquare(squares, eRightSide, eBottomSide);

			if(squares->dynarrFills[corner] == TRUE){
				addme.x = origW;
				addme.y = origH;
				addme.w = w;
				addme.h = h;

				if(anyOverlapsPtr(squares, &addme) == TRUE)
					return PROBLEM;

				EOE( addSquarePtr(squares, FALSE, &addme) );
			}else{
				sSquare *tmp = &squares->dynarrSquares[0];

				tmp = &squares->dynarrSquares[corner];
				startX = origW;
				startY = origH;
				origW -= tmp->w;
				origH -= tmp->h;
				tmp->w += w;
				tmp->h += h;
			}
		}

		if(w > 0){
			addme.x = startX;
			addme.y = 0;
			addme.w = w;
			addme.h = origH;

			if(anyOverlapsPtr(squares, &addme) == TRUE)
				return PROBLEM;

			EOE( addSquarePtr(squares, FALSE, &addme) );
		}

		if(h > 0){
			addme.x = 0;
			addme.y = startY;
			addme.w = origW;
			addme.h = h;
			if(anyOverlapsPtr(squares, &addme) == TRUE)
				return PROBLEM;

			EOE( addSquarePtr(squares, FALSE, &addme) );
		}
#else
		sSquare *side;
		unsigned int i;
		short doHeight;
		sListSquares newSquares	memset(&newSquares, 0, sizeof(newSquares));
		int corner = getCornerSquare(squares, eRightSide, eBottomSide);
		sSquare addme;

		newSquares.boundryW = squares->boundryW;
		newSquares.boundryH = squares->boundryH;
		
		if(corner < 0 || corner >= squares->num)
			return PROBLEM;
		
		for(doHeight = 0; doHeight < 2; ++doHeight){
			if(	(doHeight==0 && w > 0) || (doHeight==1 && h > 0) ){
				sListUint sides;	memset(&sides, 0, sizeof(sListUint));
	
				getSideSquares(squares, &sides, (doHeight==0) ? eRightSide : eBottomSide);
				for(i=0; i < sides.num; ++i){
					if(sides.arr[i] == corner)
						continue;
					
					side = &squares->dynarrSquares[ sides.arr[i] ];	//- Sides should be safe enough to not have to check.
	
					if(squares->dynarrFills[ sides.arr[i] ] == TRUE){
						if(doHeight==0){
							addme.x = origW;
							addme.y = side->y;
							addme.w = w;
							addme.h = side->h;
	
						}else{
							addme.x = side->x;
							addme.y = origH;
							addme.w = side->w;
							addme.h = h;
						}
						
						if(anyOverlapsPtr(squares, &addme) == TRUE){
							WARN("Overlap detected.\n");
							continue;
						}
	
						EOE( addSquarePtr( &newSquares, FALSE, &addme) );
	
					}else{
						if(doHeight==0)
							side->w += w;
						else
							side->h += h;
					}
				}
				cleanupListUint(&sides);
			}
		}
		
		tmp = &squares->dynarrSquares[corner];
		if(squares->dynarrFills[corner]==TRUE){
			if(w > 0){
				addme.x = origW;
				addme.y = tmp->y;
				addme.w = w;
				addme.h = tmp->h + h;
				
				if(anyOverlapsPtr(squares, &addme) == TRUE){
					WARN("Overlap detected.\n");	
				}else{
					EOE( addSquarePtr(&newSquares, FALSE, &addme) );
				}
			}
			
			if(h > 0){
				addme.x = tmp->x;
				addme.y = origH;
				addme.w = tmp->w;
				addme.h = h;
				
				if(anyOverlapsPtr(squares, &addme) == TRUE){
					WARN("Overlap detected.\n");
				}else{
					EOE( addSquarePtr(&newSquares, FALSE, &addme) );
				}
			}
			
		}else{
			tmp->w += w;
			tmp->h += h;
		}
	
		if(newSquares.num > 0)
			joinSquareLists(squares, &newSquares);

		cleanupListSquares(&newSquares);
#endif
	}

	XTRA_LOG("New boundries %i, %i", squares->boundryW, squares->boundryH);

	return NOPROB;
}

int canFitSquare(sListSquares *squares, unsigned int w, unsigned int h){
	if(squares == NULL){
		printf("<error> canFitSquare: squares are null.\n");
		return -1;
	}
	
	if(squares->num == 0){
		printf("canFitSquare: No holes.\n");
		return -1;
	}
	
	sSquare *tmp;
	size_t i;
	for(i=0; i < squares->num; ++i){
		if(squares->dynarrFills[i] == FALSE){
			tmp = &squares->dynarrSquares[i];
			if(w <= tmp->w && h <= tmp->h){
				return i;
			}
		}
	}
	
	return -1;
}

errCode getSideSquares(sListSquares *all, sListUint *outSides, eSquareSide side){
	unsigned int idxSide, i;
	bool most;
	sSquare *sqrA, *sqrB;
	
	cleanupListUint(outSides);
	
	for(idxSide=0; idxSide < all->num; ++idxSide){
		sqrA = &all->dynarrSquares[idxSide];
		
		most = TRUE;
		for(i=0; i < all->num && most == TRUE; ++i){
			if(i==idxSide)
				continue;
			
			sqrB = &all->dynarrSquares[i];
			
			switch(side){
				case eLeftSide:
				case eRightSide:
					if(	(sqrA->y >= sqrB->y && sqrA->y < sqrB->y + sqrB->h)
						|| (sqrA->y + sqrA->h > sqrB->y && sqrA->y + sqrA->h <= sqrB->y + sqrB->h)
					){
						if(	(side == eLeftSide && sqrB->x < sqrA->x) 
							|| (side == eRightSide && sqrB->x > sqrA->x)
						)
							most = FALSE;
					}
				break;
				
				case eTopSide:
				case eBottomSide:
					if(	(sqrA->x >= sqrB->x && sqrA->x < sqrB->x + sqrB->w)
						|| (sqrA->x + sqrA->w > sqrB->x && sqrA->x + sqrA->w <= sqrB->x + sqrB->w)
					){
						if(	(side == eTopSide && sqrB->y < sqrA->y) 
							|| (side == eBottomSide && sqrB->y > sqrA->y)
						)
							most = FALSE;
					}
				break;
				default: break;
			}
		}
		
		if(most == TRUE)
			pushListUint(outSides, idxSide);
	}
	
	return NOPROB;
}

int getCornerSquare(sListSquares *all, eSquareSide a, eSquareSide b){
	bool top2bottom;
	bool left2right;
	
	if(a == eTopSide || b == eTopSide){
		top2bottom=FALSE;
		
	}else if(a == eBottomSide || b == eBottomSide){
		top2bottom=TRUE;
		
	}else{
		printf("<warning> getCornerSquare: Didn't choose top or bottom sides.\n");
		return -1;
	}
	
	if(a == eLeftSide || b == eLeftSide){
		left2right=FALSE;
		
	}else if(a == eRightSide || b == eRightSide){
		left2right=TRUE;
		
	}else{
		printf("<warning> getCornerSquare: Didn't choose left or right side.\n");
		return -1;
	}
	
	sListUint leftOrRightSides;		memset(&leftOrRightSides, 0, sizeof(sListUint));
	getSideSquares(all, &leftOrRightSides, (left2right==TRUE) ? eRightSide : eLeftSide);
	
	sListUint topOrBottomSides;		memset(&topOrBottomSides, 0, sizeof(sListUint));
	getSideSquares(all, &topOrBottomSides, (top2bottom==TRUE) ? eBottomSide : eTopSide);

	int found = -1;
	size_t i, j;
	for(i=0; i < leftOrRightSides.num; ++i){
		for(j=0; j < topOrBottomSides.num; ++j){
			if(leftOrRightSides.arr[i] == topOrBottomSides.arr[j]){
				found = leftOrRightSides.arr[i];
				goto getCornerSquare_endloop;
			}
		}
	}
	getCornerSquare_endloop:
	
	cleanupListUint(&leftOrRightSides);
	cleanupListUint(&topOrBottomSides);
	
	return found;
}

errCode joinSquareLists(sListSquares *to, sListSquares *from){
	if(to == NULL || from == NULL){
		printf("<error> joinSquareLists: 'to' or 'from' are null.\n");
		return ERROR;
	}
	
	if(from->num == 0){
		printf("<warning> joinSquareLists: 'from' has nothing.\n");
		return PROBLEM;
	}
	
	to->dynarrSquares = realloc_chk(to->dynarrSquares, (to->num + from->num) * sizeof(sSquare));
	memcpy(
		&to->dynarrSquares[to->num], 
		from->dynarrSquares, 
		from->num * sizeof(sSquare)
	);
	
	to->dynarrFills = realloc_chk(to->dynarrFills, (to->num + from->num) * sizeof(bool));
	memcpy(
		&to->dynarrFills[to->num],
		from->dynarrFills,
		from->num * sizeof(bool)
	);
	
	to->num += from->num;
	
	return NOPROB;
}

void printSquares(const sListSquares *squares){
	sSquare *ref;
	size_t i;
	printf("Squares %u * %u\n", squares->boundryW, squares->boundryH);
	for(i=0; i < squares->num; ++i){
		ref = &squares->dynarrSquares[i];
		printf("%u: (%u, %u) %u * %u. %s \n",
			(unsigned int)i, ref->x, ref->y, ref->w, ref->h,
			(squares->dynarrFills[i]==TRUE) ? "filled" : "empty"
		);
	}
}

unsigned int getSquareIdxWithId(unsigned int ID){
	return 0;
}

void cleanupListSquares(sListSquares *cleanMe){
	SAFE_DELETE(cleanMe->dynarrSquares);
	SAFE_DELETE(cleanMe->dynarrFills);
	cleanMe->num = 0;
}


errCode fillSquare(sListSquares *squares, unsigned int idxFillMe, unsigned int w, unsigned int h){
	
	if(idxFillMe >= squares->num){
		printf("<error> fillSquare: bad index.\n");
		return ERROR;
	}
	
	if(squares->dynarrFills[idxFillMe] == TRUE){
		printf("<warning> fillSquare: square is already filled\n");
		return PROBLEM;
	}
	
	sSquare *fill = &squares->dynarrSquares[idxFillMe];
	
	if(w > fill->w || h > fill->h){
		printf("<warning> fillSquare: square is too small\n");
		return PROBLEM;
	}
	
	squares->dynarrFills[idxFillMe] = TRUE;
	
	if(w < fill->w){
		unsigned int dif = fill->w -w;
		fill->w = w;
		addSquare(squares, FALSE, fill->x +w, fill->y, dif, fill->h);
		fill = &squares->dynarrSquares[idxFillMe];	//- refresh
	}

	if(h < fill->h){
		unsigned int dif = fill->h -h;
		fill->h = h;
		addSquare(squares, FALSE, fill->x, fill->y +h, fill->w, dif);
		fill = &squares->dynarrSquares[idxFillMe];	//- refresh
	}

	return NOPROB;
}

errCode copySquares(sListSquares *to, const sListSquares *from){
	if(to == NULL || from == NULL)
		return ERROR;
	
	if(from->num ==0)
		return NOPROB;

	to->dynarrSquares = calloc_chk(from->num, sizeof(sSquare));
	to->dynarrFills = calloc_chk(from->num, sizeof(bool));
	to->boundryW = from->boundryW;
	to->boundryH = from->boundryH;
	to->num = from->num;
	memcpy(to->dynarrSquares, from->dynarrSquares, to->num *sizeof(sSquare));
	memcpy(to->dynarrFills, from->dynarrFills, to->num *sizeof(bool));
	return NOPROB;
}

int fitNFillSquare(
	sListSquares *squares, 
	unsigned int holeW, unsigned int holeH, 
	unsigned int maxBoundryW, unsigned int maxBoundryH
){
	
	if(squares == NULL){
		WARN("squares is null");
		return -1;
	}
	
	if(squares->num == 0){
		if(holeW > maxBoundryW || holeH > maxBoundryH){
			WARN("Image too large for max size.");
			return -1;
		}
		
		squares->dynarrSquares = calloc_chk(1, sizeof(sSquare));
		squares->dynarrSquares[0].x=0;
		squares->dynarrSquares[0].y=0;
		squares->dynarrSquares[0].w =squares->boundryW =holeW;
		squares->dynarrSquares[0].h =squares->boundryH =holeH;
		
		squares->dynarrFills = calloc_chk(1, sizeof(bool));
		squares->dynarrFills[0] = TRUE;
		
		++squares->num;
		
		return 0;
	}
	
	int idx = canFitSquare(squares, holeW, holeH);
	
	if(idx < 0){	/** We need to expand the boundries */
#if 1
		if(squares->boundryH >= holeH && squares->boundryW + holeW <= maxBoundryW)
			incBoundries(squares, holeW, 0);
		else if(squares->boundryW >= holeW && squares->boundryH + holeH <= maxBoundryH)
			incBoundries(squares, 0, holeH);
		else if(squares->boundryW + holeW <= maxBoundryW && squares->boundryH + holeH <= maxBoundryH)
			incBoundries(squares, holeW, holeH);
		else
			return -1;
		
		idx = canFitSquare(squares, holeW, holeH);
#else
		size_t b;
		sListUint rightSides, bottomSides;
		sListUint *refSides;
		sSquare *ref, *best=NULL;
		
		eSquareSide curSide = eRightSide;
		while(curSide != NUM_SIDES){	/** Attempt to find a hole on the side with at least 1 dimension large enough. */
		
			if(curSide == eRightSide)
				refSides = &rightSides;
			else
				refSides = &bottomSides;

			memset(refSides, 0, sizeof(sListUint));
			getSideSquares(squares, refSides, curSide);
			
			for(b=0; b < refSides->num; ++b){
				if(squares->dynarrFills[ refSides->arr[b] ] == FALSE){
					ref = &squares->dynarrSquares[ refSides->arr[b] ];
					if(    (curSide == eRightSide && ref->h >= holeH) 
						|| (curSide == eBottomSide && ref->w >= holeW) 
					){
						if(best == NULL){
							best = ref;
							idx = b;
							
						}else if( (ref->w + ref->h) < (best->w + best->h) ){	/** The least increase wins */
							best = ref;
						}
					}
				}
			}

			if(curSide==eRightSide)
				curSide=eBottomSide;
			else
				curSide=NUM_SIDES;

		}
		
		if(best != NULL){
			int incW = holeW - best->w;
			int incH = holeH - best->h;
			
			if(incW < 0)
				incW = 0;
			
			if(incH < 0)
				incH = 0;
			
			EOE( incBoundries(squares, (unsigned int)incW, (unsigned int)incH) );
			
		}else{	/** No hole found with at least 1 dimension large enough. */
			int corner = getCornerSquare(squares, eRightSide, eBottomSide);
			
			if(corner >= 0 && squares->dynarrFills[corner]==FALSE){
				int w = holeW - squares->dynarrSquares[corner].w;
				int h = holeH - squares->dynarrSquares[corner].h;

				if(w < 0)
					w =0;

				if(h < 0)
					h =0;

				EOE( incBoundries(squares, (unsigned int)w, (unsigned int)h) );
				
			}else if(holeH <= squares->boundryH && squares->boundryW + holeW <= maxBoundryW){
				EOE( incBoundries(squares, holeW, 0) );
				
			}else if(holeW <= squares->boundryW && squares->boundryH + holeH <= maxBoundryH){
				EOE( incBoundries(squares, 0, holeH) );
				
			}else if(  squares->boundryW + holeW <= maxBoundryW
				&& squares->boundryH + holeH <= maxBoundryH
			){
				EOE( incBoundries(squares, holeW, holeH) );
			}	
		}
		
		idx = canFitSquare(squares, holeW, holeH);
			
		cleanupListUint(&rightSides);
		cleanupListUint(&bottomSides);
#endif
	}
	
	if(idx >= 0){
		EOE( fillSquare(squares, idx, holeW, holeH) );
	}

	return idx;
}

errCode shrinkwrapSquares(sListSquares *wrapme){
	return NOPROB;
}

