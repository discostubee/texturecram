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

/*!\file	squarefit.h
 *!\brief	Square fit aims to manage a NONE overlapping list of squares that can be filled or a hole.
 */

#ifndef SQUAREFIT_H
#define SQUAREFIT_H

#include "utils.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct defSquare{
	unsigned int x, y, w, h;
} sSquare;

typedef enum defSquareSide{
	eLeftSide,
	eRightSide,
	eTopSide,
	eBottomSide,
	NUM_SIDES
} eSquareSide;

typedef struct defListSquares{
	sSquare *dynarrSquares; 
	bool *dynarrFills;	
	unsigned int boundryW, boundryH;
	size_t num;
} sListSquares;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*!\brief	Collects clusters of smaller squares into larger squares.
 */
errCode consolidate(sListSquares *squares);

/*!\brief	Adds a square to the list.
 */
errCode addSquare(sListSquares *squares, bool filled, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
errCode addSquarePtr(sListSquares *squares, bool filled, sSquare *addMe);

/*!\brief	
 */
errCode overlap(const sSquare *a, const sSquare *b, sSquare *isec);

/*!\brief	Returns TRUE if the square doesn't overlap any of the squares in the list 
 */
bool anyOverlaps(sListSquares *squares, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
bool anyOverlapsPtr(sListSquares *squares, sSquare *checkme);

/*!\brief	Increases the boundries by the given width and height. It does this by increaseing the dimensions
 * 			of the holes along the left and bottom sides, and creating new holes if a filled square is there.
 */
errCode incBoundries(sListSquares *squares, unsigned int w, unsigned int h);

/*!\brief	Returns the index of the hole that is big enough for a square of the given dimensions. Returns a negative on errors.
 */
int canFitSquare(sListSquares *squares, unsigned int w, unsigned int h);

/*!\brief	Finds the index of all the squares that are along a particular side.
 */
errCode getSideSquares(sListSquares *all, sListUint *outSides, eSquareSide side);

/*!\brief	Returns the index of the square at the corner of the two sides.
 */
int getCornerSquare(sListSquares *all, eSquareSide a, eSquareSide b);

/*!\brief	Cleans and polishes.
 */
void cleanupListSquares(sListSquares *cleanMe);

/*!\brief	Fills the square hole at the idxFillMe with another square. Fills from the top left, and creates 2 new holes with the gap 
 * 			thats left.
 *!\return	PROBLEM if holes is null. PROBLEM if the index is a filled square already. 
 *			idxFillMe is out of range or the hole at idxFillMe is not big enough.
 */
errCode fillSquare(sListSquares *squares, unsigned int idxFillMe, unsigned int w, unsigned int h);

/*!\brief	Copy the square list into another. Cleans up the target if not null.
 */
errCode copySquares(sListSquares *to, const sListSquares *from);

/*!\brief	Joins the squares found in 'from' to the list 'to'. Doesn't check for any overlap currently.
 */
errCode joinSquareLists(sListSquares *to, sListSquares *from);

/*!\brief	Reduce the boundries by shrinking empty squares.
 */
errCode shrinkwrapSquares(sListSquares *wrapme);

/*!\brief	Helpful debugging info printed to the terminal.
 */
void printSquares(const sListSquares *squares);

/*!\brief	Tries to place a rectangle into a hole, and increases the boundry holes (thus the total boundries) up to the max if it can't. 
 *!\return	If is succeeds, it returns the index of hole it fit into. 
 *!			Otherwith, -1 if no hole could be found, when if it tries to increase the boundries.		
 */
int fitNFillSquare(
	sListSquares *squares, 
	unsigned int holeW, unsigned int holeH, 
	unsigned int maxBoundryW, unsigned int maxBoundryH
);


#endif
