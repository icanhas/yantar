/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "local.h"

/*
 *
 * This file does all of the processing necessary to turn a raw grid of points
 * read from the map file into a srfGridMesh_t ready for rendering.
 *
 * The level of detail solution is direction independent, based only on subdivided
 * distance from the true curve.
 *
 * Only a single entry point:
 *
 * srfGridMesh_t *R_SubdividePatchToGrid( int width, int height,
 *                                                              Drawvert points[MAX_PATCH_SIZE*MAX_PATCH_SIZE] ) {
 *
 */


/*
 * LerpDrawVert
 */
static void
LerpDrawVert(Drawvert *a, Drawvert *b, Drawvert *out)
{
	out->xyz[0] = 0.5f * (a->xyz[0] + b->xyz[0]);
	out->xyz[1] = 0.5f * (a->xyz[1] + b->xyz[1]);
	out->xyz[2] = 0.5f * (a->xyz[2] + b->xyz[2]);

	out->st[0] = 0.5f * (a->st[0] + b->st[0]);
	out->st[1] = 0.5f * (a->st[1] + b->st[1]);

	out->lightmap[0] = 0.5f * (a->lightmap[0] + b->lightmap[0]);
	out->lightmap[1] = 0.5f * (a->lightmap[1] + b->lightmap[1]);

	out->color[0]	= (a->color[0] + b->color[0]) >> 1;
	out->color[1]	= (a->color[1] + b->color[1]) >> 1;
	out->color[2]	= (a->color[2] + b->color[2]) >> 1;
	out->color[3]	= (a->color[3] + b->color[3]) >> 1;
}

/*
 * Transpose
 */
static void
Transpose(int width, int height, Drawvert ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE])
{
	int i, j;
	Drawvert temp;

	if(width > height){
		for(i = 0; i < height; i++)
			for(j = i + 1; j < width; j++){
				if(j < height){
					/* swap the value */
					temp = ctrl[j][i];
					ctrl[j][i] = ctrl[i][j];
					ctrl[i][j] = temp;
				}else{
					/* just copy */
					ctrl[j][i] = ctrl[i][j];
				}
			}
	}else{
		for(i = 0; i < width; i++)
			for(j = i + 1; j < height; j++){
				if(j < width){
					/* swap the value */
					temp = ctrl[i][j];
					ctrl[i][j] = ctrl[j][i];
					ctrl[j][i] = temp;
				}else{
					/* just copy */
					ctrl[i][j] = ctrl[j][i];
				}
			}
	}

}


/*
 * MakeMeshNormals
 *
 * Handles all the complicated wrapping and degenerate cases
 */
static void
MakeMeshNormals(int width, int height, Drawvert ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE])
{
	int i, j, k, dist;
	Vec3	normal;
	Vec3	sum;
	int	count = 0;
	Vec3	base;
	Vec3	delta;
	int	x, y;
	Drawvert      *dv;
	Vec3	around[8], temp;
	qbool good[8];
	qbool wrapWidth, wrapHeight;
	float len;
	static int neighbors[8][2] = {
		{0,1}, {1,1}, {1,0}, {1,-1}, {0,-1}, {-1,-1}, {-1,0}, {-1,1}
	};

	wrapWidth = qfalse;
	for(i = 0; i < height; i++){
		subv3(ctrl[i][0].xyz, ctrl[i][width-1].xyz, delta);
		len = lensqrv3(delta);
		if(len > 1.0){
			break;
		}
	}
	if(i == height){
		wrapWidth = qtrue;
	}

	wrapHeight = qfalse;
	for(i = 0; i < width; i++){
		subv3(ctrl[0][i].xyz, ctrl[height-1][i].xyz, delta);
		len = lensqrv3(delta);
		if(len > 1.0){
			break;
		}
	}
	if(i == width){
		wrapHeight = qtrue;
	}


	for(i = 0; i < width; i++)
		for(j = 0; j < height; j++){
			count = 0;
			dv = &ctrl[j][i];
			copyv3(dv->xyz, base);
			for(k = 0; k < 8; k++){
				clearv3(around[k]);
				good[k] = qfalse;

				for(dist = 1; dist <= 3; dist++){
					x = i + neighbors[k][0] * dist;
					y = j + neighbors[k][1] * dist;
					if(wrapWidth){
						if(x < 0){
							x = width - 1 + x;
						}else if(x >= width){
							x = 1 + x - width;
						}
					}
					if(wrapHeight){
						if(y < 0){
							y = height - 1 + y;
						}else if(y >= height){
							y = 1 + y - height;
						}
					}

					if(x < 0 || x >= width || y < 0 || y >= height){
						break;	/* edge of patch */
					}
					subv3(ctrl[y][x].xyz, base, temp);
					if(norm2v3(temp, temp) == 0){
						continue;	/* degenerate edge, get more dist */
					}else{
						good[k] = qtrue;
						copyv3(temp, around[k]);
						break;	/* good edge */
					}
				}
			}

			clearv3(sum);
			for(k = 0; k < 8; k++){
				if(!good[k] || !good[(k+1)&7]){
					continue;	/* didn't get two points */
				}
				crossv3(around[(k+1)&7], around[k], normal);
				if(norm2v3(normal, normal) == 0){
					continue;
				}
				addv3(normal, sum, sum);
				count++;
			}
			/* if ( count == 0 ) {
			 * printf("bad normal\n");
			 * } */
			norm2v3(sum, dv->normal);
		}
}


/*
 * InvertCtrl
 */
static void
InvertCtrl(int width, int height, Drawvert ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE])
{
	int i, j;
	Drawvert temp;

	for(i = 0; i < height; i++)
		for(j = 0; j < width/2; j++){
			temp = ctrl[i][j];
			ctrl[i][j] = ctrl[i][width-1-j];
			ctrl[i][width-1-j] = temp;
		}
}


/*
 * InvertErrorTable
 */
static void
InvertErrorTable(float errorTable[2][MAX_GRID_SIZE], int width, int height)
{
	int i;
	float copy[2][MAX_GRID_SIZE];

	Q_Memcpy(copy, errorTable, sizeof(copy));

	for(i = 0; i < width; i++)
		errorTable[1][i] = copy[0][i];	/* [width-1-i]; */

	for(i = 0; i < height; i++)
		errorTable[0][i] = copy[1][height-1-i];

}

/*
 * PutPointsOnCurve
 */
static void
PutPointsOnCurve(Drawvert ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE],
		 int width, int height)
{
	int i, j;
	Drawvert prev, next;

	for(i = 0; i < width; i++)
		for(j = 1; j < height; j += 2){
			LerpDrawVert(&ctrl[j][i], &ctrl[j+1][i], &prev);
			LerpDrawVert(&ctrl[j][i], &ctrl[j-1][i], &next);
			LerpDrawVert(&prev, &next, &ctrl[j][i]);
		}


	for(j = 0; j < height; j++)
		for(i = 1; i < width; i += 2){
			LerpDrawVert(&ctrl[j][i], &ctrl[j][i+1], &prev);
			LerpDrawVert(&ctrl[j][i], &ctrl[j][i-1], &next);
			LerpDrawVert(&prev, &next, &ctrl[j][i]);
		}
}

/*
 * R_CreateSurfaceGridMesh
 */
srfGridMesh_t *
R_CreateSurfaceGridMesh(int width, int height,
			Drawvert ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE], float errorTable[2][MAX_GRID_SIZE])
{
	int i, j, size;
	Drawvert *vert;
	Vec3 tmpVec;
	srfGridMesh_t *grid;

	/* copy the results out to a grid */
	size = (width * height - 1) * sizeof(Drawvert) + sizeof(*grid);

#ifdef PATCH_STITCHING
	grid = /*ri.hunkalloc*/ ri.Malloc(size);
	Q_Memset(grid, 0, size);

	grid->widthLodError = /*ri.hunkalloc*/ ri.Malloc(width * 4);
	Q_Memcpy(grid->widthLodError, errorTable[0], width * 4);

	grid->heightLodError = /*ri.hunkalloc*/ ri.Malloc(height * 4);
	Q_Memcpy(grid->heightLodError, errorTable[1], height * 4);
#else
	grid = ri.hunkalloc(size);
	Q_Memset(grid, 0, size);

	grid->widthLodError = ri.hunkalloc(width * 4);
	Q_Memcpy(grid->widthLodError, errorTable[0], width * 4);

	grid->heightLodError = ri.hunkalloc(height * 4);
	Q_Memcpy(grid->heightLodError, errorTable[1], height * 4);
#endif

	grid->width = width;
	grid->height = height;
	grid->surfaceType = SF_GRID;
	ClearBounds(grid->meshBounds[0], grid->meshBounds[1]);
	for(i = 0; i < width; i++)
		for(j = 0; j < height; j++){
			vert = &grid->verts[j*width+i];
			*vert = ctrl[j][i];
			AddPointToBounds(vert->xyz, grid->meshBounds[0], grid->meshBounds[1]);
		}

	/* compute local origin and bounds */
	addv3(grid->meshBounds[0], grid->meshBounds[1], grid->localOrigin);
	scalev3(grid->localOrigin, 0.5f, grid->localOrigin);
	subv3(grid->meshBounds[0], grid->localOrigin, tmpVec);
	grid->meshRadius = lenv3(tmpVec);

	copyv3(grid->localOrigin, grid->lodOrigin);
	grid->lodRadius = grid->meshRadius;
	return grid;
}

/*
 * R_FreeSurfaceGridMesh
 */
void
R_FreeSurfaceGridMesh(srfGridMesh_t *grid)
{
	ri.Free(grid->widthLodError);
	ri.Free(grid->heightLodError);
	ri.Free(grid);
}

/*
 * R_SubdividePatchToGrid
 */
srfGridMesh_t *
R_SubdividePatchToGrid(int width, int height,
		       Drawvert points[MAX_PATCH_SIZE*MAX_PATCH_SIZE])
{
	int i, j, k, l;
	drawVert_t_cleared(prev);
	drawVert_t_cleared(next);
	drawVert_t_cleared(mid);
	float len, maxLen;
	int dir;
	int t;
	Drawvert	ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE];
	float		errorTable[2][MAX_GRID_SIZE];

	for(i = 0; i < width; i++)
		for(j = 0; j < height; j++)
			ctrl[j][i] = points[j*width+i];

	for(dir = 0; dir < 2; dir++){

		for(j = 0; j < MAX_GRID_SIZE; j++)
			errorTable[dir][j] = 0;

		/* horizontal subdivisions */
		for(j = 0; j + 2 < width; j += 2){
			/* check subdivided midpoints against control points */

			/* FIXME: also check midpoints of adjacent patches against the control points
			 * this would basically stitch all patches in the same LOD group together. */

			maxLen = 0;
			for(i = 0; i < height; i++){
				Vec3	midxyz;
				Vec3	midxyz2;
				Vec3	dir;
				Vec3	projected;
				float	d;

				/* calculate the point on the curve */
				for(l = 0; l < 3; l++)
					midxyz[l] = (ctrl[i][j].xyz[l] + ctrl[i][j+1].xyz[l] * 2
						     + ctrl[i][j+2].xyz[l]) * 0.25f;

				/* see how far off the line it is
				 * using dist-from-line will not account for internal
				 * texture warping, but it gives a lot less polygons than
				 * dist-from-midpoint */
				subv3(midxyz, ctrl[i][j].xyz, midxyz);
				subv3(ctrl[i][j+2].xyz, ctrl[i][j].xyz, dir);
				normv3(dir);

				d = dotv3(midxyz, dir);
				scalev3(dir, d, projected);
				subv3(midxyz, projected, midxyz2);
				len = lensqrv3(midxyz2);	/* we will do the sqrt later */
				if(len > maxLen){
					maxLen = len;
				}
			}

			maxLen = sqrt(maxLen);

			/* if all the points are on the lines, remove the entire columns */
			if(maxLen < 0.1f){
				errorTable[dir][j+1] = 999;
				continue;
			}

			/* see if we want to insert subdivided columns */
			if(width + 2 > MAX_GRID_SIZE){
				errorTable[dir][j+1] = 1.0f/maxLen;
				continue;	/* can't subdivide any more */
			}

			if(maxLen <= r_subdivisions->value){
				errorTable[dir][j+1] = 1.0f/maxLen;
				continue;	/* didn't need subdivision */
			}

			errorTable[dir][j+2] = 1.0f/maxLen;

			/* insert two columns and replace the peak */
			width += 2;
			for(i = 0; i < height; i++){
				LerpDrawVert(&ctrl[i][j], &ctrl[i][j+1], &prev);
				LerpDrawVert(&ctrl[i][j+1], &ctrl[i][j+2], &next);
				LerpDrawVert(&prev, &next, &mid);

				for(k = width - 1; k > j + 3; k--)
					ctrl[i][k] = ctrl[i][k-2];
				ctrl[i][j + 1]	= prev;
				ctrl[i][j + 2]	= mid;
				ctrl[i][j + 3]	= next;
			}

			/* back up and recheck this set again, it may need more subdivision */
			j -= 2;

		}

		Transpose(width, height, ctrl);
		t = width;
		width	= height;
		height	= t;
	}


	/* put all the aproximating points on the curve */
	PutPointsOnCurve(ctrl, width, height);

	/* cull out any rows or columns that are colinear */
	for(i = 1; i < width-1; i++){
		if(errorTable[0][i] != 999){
			continue;
		}
		for(j = i+1; j < width; j++){
			for(k = 0; k < height; k++)
				ctrl[k][j-1] = ctrl[k][j];
			errorTable[0][j-1] = errorTable[0][j];
		}
		width--;
	}

	for(i = 1; i < height-1; i++){
		if(errorTable[1][i] != 999){
			continue;
		}
		for(j = i+1; j < height; j++){
			for(k = 0; k < width; k++)
				ctrl[j-1][k] = ctrl[j][k];
			errorTable[1][j-1] = errorTable[1][j];
		}
		height--;
	}

#if 1
	/* flip for longest tristrips as an optimization
	 * the results should be visually identical with or
	 * without this step */
	if(height > width){
		Transpose(width, height, ctrl);
		InvertErrorTable(errorTable, width, height);
		t = width;
		width	= height;
		height	= t;
		InvertCtrl(width, height, ctrl);
	}
#endif

	/* calculate normals */
	MakeMeshNormals(width, height, ctrl);

	return R_CreateSurfaceGridMesh(width, height, ctrl, errorTable);
}

/*
 * R_GridInsertColumn
 */
srfGridMesh_t *
R_GridInsertColumn(srfGridMesh_t *grid, int column, int row, Vec3 point, float loderror)
{
	int i, j;
	int width, height, oldwidth;
	Drawvert	ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE];
	float	errorTable[2][MAX_GRID_SIZE];
	float	lodRadius;
	Vec3	lodOrigin;

	oldwidth = 0;
	width = grid->width + 1;
	if(width > MAX_GRID_SIZE)
		return NULL;
	height = grid->height;
	for(i = 0; i < width; i++){
		if(i == column){
			/* insert new column */
			for(j = 0; j < grid->height; j++){
				LerpDrawVert(&grid->verts[j * grid->width + i-1],
					&grid->verts[j * grid->width + i], &ctrl[j][i]);
				if(j == row)
					copyv3(point, ctrl[j][i].xyz);
			}
			errorTable[0][i] = loderror;
			continue;
		}
		errorTable[0][i] = grid->widthLodError[oldwidth];
		for(j = 0; j < grid->height; j++)
			ctrl[j][i] = grid->verts[j * grid->width + oldwidth];
		oldwidth++;
	}
	for(j = 0; j < grid->height; j++)
		errorTable[1][j] = grid->heightLodError[j];
	/* put all the aproximating points on the curve
	 * PutPointsOnCurve( ctrl, width, height );
	 * calculate normals */
	MakeMeshNormals(width, height, ctrl);

	copyv3(grid->lodOrigin, lodOrigin);
	lodRadius = grid->lodRadius;
	/* free the old grid */
	R_FreeSurfaceGridMesh(grid);
	/* create a new grid */
	grid = R_CreateSurfaceGridMesh(width, height, ctrl, errorTable);
	grid->lodRadius = lodRadius;
	copyv3(lodOrigin, grid->lodOrigin);
	return grid;
}

/*
 * R_GridInsertRow
 */
srfGridMesh_t *
R_GridInsertRow(srfGridMesh_t *grid, int row, int column, Vec3 point, float loderror)
{
	int i, j;
	int width, height, oldheight;
	Drawvert	ctrl[MAX_GRID_SIZE][MAX_GRID_SIZE];
	float	errorTable[2][MAX_GRID_SIZE];
	float	lodRadius;
	Vec3	lodOrigin;

	oldheight	= 0;
	width	= grid->width;
	height	= grid->height + 1;
	if(height > MAX_GRID_SIZE)
		return NULL;
	for(i = 0; i < height; i++){
		if(i == row){
			/* insert new row */
			for(j = 0; j < grid->width; j++){
				LerpDrawVert(&grid->verts[(i-1) * grid->width + j],
					&grid->verts[i * grid->width + j], &ctrl[i][j]);
				if(j == column)
					copyv3(point, ctrl[i][j].xyz);
			}
			errorTable[1][i] = loderror;
			continue;
		}
		errorTable[1][i] = grid->heightLodError[oldheight];
		for(j = 0; j < grid->width; j++)
			ctrl[i][j] = grid->verts[oldheight * grid->width + j];
		oldheight++;
	}
	for(j = 0; j < grid->width; j++)
		errorTable[0][j] = grid->widthLodError[j];
	/* put all the aproximating points on the curve
	 * PutPointsOnCurve( ctrl, width, height );
	 * calculate normals */
	MakeMeshNormals(width, height, ctrl);

	copyv3(grid->lodOrigin, lodOrigin);
	lodRadius = grid->lodRadius;
	/* free the old grid */
	R_FreeSurfaceGridMesh(grid);
	/* create a new grid */
	grid = R_CreateSurfaceGridMesh(width, height, ctrl, errorTable);
	grid->lodRadius = lodRadius;
	copyv3(lodOrigin, grid->lodOrigin);
	return grid;
}
