/* model loading and caching */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

#include "local.h"

#define LL(x) x=LittleLong(x)

static qbool R_LoadMD3(model_t *mod, int lod, void *buffer, const char *name);
static qbool R_LoadMD4(model_t *mod, void *buffer, const char *name);

/*
 * R_RegisterMD3
 */
Handle
R_RegisterMD3(const char *name, model_t *mod)
{
	union {
		unsigned	*u;
		void		*v;
	} buf;
	int	lod;
	int	ident;
	qbool loaded = qfalse;
	int numLoaded;
	char filename[MAX_QPATH], namebuf[MAX_QPATH+20];
	char *fext, defex[] = "md3";

	numLoaded = 0;

	strcpy(filename, name);

	fext = strchr(filename, '.');
	if(!fext)
		fext = defex;
	else{
		*fext = '\0';
		fext++;
	}

	for(lod = MD3_MAX_LODS - 1; lod >= 0; lod--){
		if(lod)
			Q_sprintf(namebuf, sizeof(namebuf), "%s_%d.%s", filename, lod, fext);
		else
			Q_sprintf(namebuf, sizeof(namebuf), "%s.%s", filename, fext);

		ri.fsreadfile(namebuf, &buf.v);
		if(!buf.u)
			continue;

		ident = LittleLong(*(unsigned*)buf.u);
		if(ident == MD4_IDENT)
			loaded = R_LoadMD4(mod, buf.u, name);
		else{
			if(ident == MD3_IDENT)
				loaded = R_LoadMD3(mod, lod, buf.u, name);
			else
				ri.Printf(PRINT_WARNING,"R_RegisterMD3: unknown fileid for %s\n", name);
		}

		ri.fsfreefile(buf.v);

		if(loaded){
			mod->numLods++;
			numLoaded++;
		}else
			break;
	}

	if(numLoaded){
		/* duplicate into higher lod spots that weren't
		 * loaded, in case the user changes r_lodbias on the fly */
		for(lod--; lod >= 0; lod--){
			mod->numLods++;
			mod->md3[lod] = mod->md3[lod + 1];
		}

		return mod->index;
	}

#ifdef _DEBUG
	ri.Printf(PRINT_WARNING,"R_RegisterMD3: couldn't load %s\n", name);
#endif

	mod->type = MOD_BAD;
	return 0;
}

/*
 * R_RegisterIQM
 */
Handle
R_RegisterIQM(const char *name, model_t *mod)
{
	union {
		unsigned	*u;
		void		*v;
	} buf;
	qbool loaded = qfalse;
	int filesize;

	filesize = ri.fsreadfile(name, (void**)&buf.v);
	if(!buf.u){
		mod->type = MOD_BAD;
		return 0;
	}

	loaded = R_LoadIQM(mod, buf.u, filesize, name);

	ri.fsfreefile (buf.v);

	if(!loaded){
		ri.Printf(PRINT_WARNING,"R_RegisterIQM: couldn't load iqm file %s\n", name);
		mod->type = MOD_BAD;
		return 0;
	}

	return mod->index;
}


typedef struct {
	char *ext;
	Handle (*ModelLoader)(const char *, model_t *);
} modelExtToLoaderMap_t;

/* Note that the ordering indicates the order of preference used
 * when there are multiple models of different formats available */
static modelExtToLoaderMap_t modelLoaders[] =
{
	{ "iqm", R_RegisterIQM },
	{ "md4", R_RegisterMD3 },
	{ "md3", R_RegisterMD3 }
};

static int numModelLoaders = ARRAY_LEN(modelLoaders);

/* =============================================================================== */

/*
** R_GetModelByHandle
*/
model_t *
R_GetModelByHandle(Handle index)
{
	model_t *mod;

	/* out of range gets the defualt model */
	if(index < 1 || index >= tr.numModels){
		return tr.models[0];
	}

	mod = tr.models[index];

	return mod;
}

/* =============================================================================== */

/*
** R_AllocModel
*/
model_t *
R_AllocModel(void)
{
	model_t *mod;

	if(tr.numModels == MAX_MOD_KNOWN){
		return NULL;
	}

	mod = ri.hunkalloc(sizeof(*tr.models[tr.numModels]), Hlow);
	mod->index = tr.numModels;
	tr.models[tr.numModels] = mod;
	tr.numModels++;

	return mod;
}

/*
 * RE_RegisterModel
 *
 * Loads in a model for the given name
 *
 * Zero will be returned if the model fails to load.
 * An entry will be retained for failed models as an
 * optimization to prevent disk rescanning if they are
 * asked for again.
 */
Handle
RE_RegisterModel(const char *name)
{
	model_t *mod;
	Handle		hModel;
	qbool		orgNameFailed = qfalse;
	int	orgLoader = -1;
	int	i;
	char	localName[ MAX_QPATH ];
	const char      *ext;
	char	altName[ MAX_QPATH ];

	if(!name || !name[0]){
		ri.Printf(PRINT_ALL, "RE_RegisterModel: NULL name\n");
		return 0;
	}

	if(strlen(name) >= MAX_QPATH){
		ri.Printf(PRINT_ALL, "Model name exceeds MAX_QPATH\n");
		return 0;
	}

	/*
	 * search the currently loaded models
	 *  */
	for(hModel = 1; hModel < tr.numModels; hModel++){
		mod = tr.models[hModel];
		if(!strcmp(mod->name, name)){
			if(mod->type == MOD_BAD){
				return 0;
			}
			return hModel;
		}
	}

	/* allocate a new model_t */

	if((mod = R_AllocModel()) == NULL){
		ri.Printf(PRINT_WARNING, "RE_RegisterModel: R_AllocModel() failed for '%s'\n", name);
		return 0;
	}

	/* only set the name after the model has been successfully loaded */
	Q_strncpyz(mod->name, name, sizeof(mod->name));


	/* make sure the render thread is stopped */
	R_SyncRenderThread();

	mod->type = MOD_BAD;
	mod->numLods = 0;

	/*
	 * load the files
	 *  */
	Q_strncpyz(localName, name, MAX_QPATH);

	ext = Q_getext(localName);

	if(*ext){
		/* Look for the correct loader and use it */
		for(i = 0; i < numModelLoaders; i++)
			if(!Q_stricmp(ext, modelLoaders[ i ].ext)){
				/* Load */
				hModel = modelLoaders[ i ].ModelLoader(localName, mod);
				break;
			}

		/* A loader was found */
		if(i < numModelLoaders){
			if(!hModel){
				/* Loader failed, most likely because the file isn't there;
				 * try again without the extension */
				orgNameFailed = qtrue;
				orgLoader = i;
				Q_stripext(name, localName, MAX_QPATH);
			}else{
				/* Something loaded */
				return mod->index;
			}
		}
	}

	/* Try and find a suitable match using all
	 * the model formats supported */
	for(i = 0; i < numModelLoaders; i++){
		if(i == orgLoader)
			continue;

		Q_sprintf(altName, sizeof(altName), "%s.%s", localName, modelLoaders[ i ].ext);

		/* Load */
		hModel = modelLoaders[ i ].ModelLoader(altName, mod);

		if(hModel){
			if(orgNameFailed){
				ri.Printf(PRINT_DEVELOPER, "WARNING: %s not present, using %s instead\n",
					name, altName);
			}

			break;
		}
	}

	return hModel;
}

/*
 * R_LoadMD3
 */
static qbool
R_LoadMD3(model_t *mod, int lod, void *buffer, const char *mod_name)
{
	int i, j;
	MD3header *pinmodel;
	MD3frame *frame;
	MD3surf *surf;
	MD3shader *shader;
	MD3tri *tri;
	md3St_t *st;
	MD3xyznorm *xyz;
	MD3tag	*tag;
	int	version;
	int	size;

	pinmodel = (MD3header*)buffer;

	version = LittleLong (pinmodel->version);
	if(version != MD3_VERSION){
		ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has wrong version (%i should be %i)\n",
			mod_name, version, MD3_VERSION);
		return qfalse;
	}

	mod->type = MOD_MESH;
	size = LittleLong(pinmodel->ofsEnd);
	mod->dataSize	+= size;
	mod->md3[lod]	= ri.hunkalloc(size, Hlow);

	Q_Memcpy (mod->md3[lod], buffer, LittleLong(pinmodel->ofsEnd));

	LL(mod->md3[lod]->ident);
	LL(mod->md3[lod]->version);
	LL(mod->md3[lod]->numFrames);
	LL(mod->md3[lod]->numTags);
	LL(mod->md3[lod]->numSurfaces);
	LL(mod->md3[lod]->ofsFrames);
	LL(mod->md3[lod]->ofsTags);
	LL(mod->md3[lod]->ofsSurfaces);
	LL(mod->md3[lod]->ofsEnd);

	if(mod->md3[lod]->numFrames < 1){
		ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has no frames\n", mod_name);
		return qfalse;
	}

	/* swap all the frames */
	frame = (MD3frame*)((byte*)mod->md3[lod] + mod->md3[lod]->ofsFrames);
	for(i = 0; i < mod->md3[lod]->numFrames; i++, frame++){
		frame->radius = LittleFloat(frame->radius);
		for(j = 0; j < 3; j++){
			frame->bounds[0][j]	= LittleFloat(frame->bounds[0][j]);
			frame->bounds[1][j]	= LittleFloat(frame->bounds[1][j]);
			frame->localOrigin[j]	= LittleFloat(frame->localOrigin[j]);
		}
	}

	/* swap all the tags */
	tag = (MD3tag*)((byte*)mod->md3[lod] + mod->md3[lod]->ofsTags);
	for(i = 0; i < mod->md3[lod]->numTags * mod->md3[lod]->numFrames; i++, tag++)
		for(j = 0; j < 3; j++){
			tag->origin[j]	= LittleFloat(tag->origin[j]);
			tag->axis[0][j] = LittleFloat(tag->axis[0][j]);
			tag->axis[1][j] = LittleFloat(tag->axis[1][j]);
			tag->axis[2][j] = LittleFloat(tag->axis[2][j]);
		}

	/* swap all the surfaces */
	surf = (MD3surf*)((byte*)mod->md3[lod] + mod->md3[lod]->ofsSurfaces);
	for(i = 0; i < mod->md3[lod]->numSurfaces; i++){

		LL(surf->ident);
		LL(surf->flags);
		LL(surf->numFrames);
		LL(surf->numShaders);
		LL(surf->numTriangles);
		LL(surf->ofsTriangles);
		LL(surf->numVerts);
		LL(surf->ofsShaders);
		LL(surf->ofsSt);
		LL(surf->ofsXyzNormals);
		LL(surf->ofsEnd);

		if(surf->numVerts > SHADER_MAX_VERTEXES){
			ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has more than %i verts on a surface (%i).\n",
				mod_name, SHADER_MAX_VERTEXES, surf->numVerts);
			return qfalse;
		}
		if(surf->numTriangles*3 > SHADER_MAX_INDEXES){
			ri.Printf(PRINT_WARNING,
				"R_LoadMD3: %s has more than %i triangles on a surface (%i).\n",
				mod_name, SHADER_MAX_INDEXES / 3,
				surf->numTriangles);
			return qfalse;
		}

		/* change to surface identifier */
		surf->ident = SF_MD3;

		/* lowercase the surface name so skin compares are faster */
		Q_strlwr(surf->name);

		/* strip off a trailing _1 or _2
		 * this is a crutch for q3data being a mess */
		j = strlen(surf->name);
		if(j > 2 && surf->name[j-2] == '_'){
			surf->name[j-2] = 0;
		}

		/* register the shaders */
		shader = (MD3shader*)((byte*)surf + surf->ofsShaders);
		for(j = 0; j < surf->numShaders; j++, shader++){
			material_t *sh;

			sh = R_FindShader(shader->name, LIGHTMAP_NONE, qtrue);
			if(sh->defaultShader){
				shader->shaderIndex = 0;
			}else{
				shader->shaderIndex = sh->index;
			}
		}

		/* swap all the triangles */
		tri = (MD3tri*)((byte*)surf + surf->ofsTriangles);
		for(j = 0; j < surf->numTriangles; j++, tri++){
			LL(tri->indexes[0]);
			LL(tri->indexes[1]);
			LL(tri->indexes[2]);
		}

		/* swap all the ST */
		st = (md3St_t*)((byte*)surf + surf->ofsSt);
		for(j = 0; j < surf->numVerts; j++, st++){
			st->st[0]	= LittleFloat(st->st[0]);
			st->st[1]	= LittleFloat(st->st[1]);
		}

		/* swap all the XyzNormals */
		xyz = (MD3xyznorm*)((byte*)surf + surf->ofsXyzNormals);
		for(j = 0; j < surf->numVerts * surf->numFrames; j++, xyz++){
			xyz->xyz[0]	= LittleShort(xyz->xyz[0]);
			xyz->xyz[1]	= LittleShort(xyz->xyz[1]);
			xyz->xyz[2]	= LittleShort(xyz->xyz[2]);

			xyz->normal = LittleShort(xyz->normal);
		}


		/* find the next surface */
		surf = (MD3surf*)((byte*)surf + surf->ofsEnd);
	}

	return qtrue;
}

/*
 * R_LoadMD4
 */
static qbool
R_LoadMD4(model_t *mod, void *buffer, const char *mod_name)
{
	int i, j, k, lodindex;
	md4Header_t *pinmodel, *md4;
	md4Frame_t *frame;
	md4LOD_t *lod;
	md4Surface_t *surf;
	md4Triangle_t *tri;
	md4Vertex_t *v;
	int	version;
	int	size;
	material_t *sh;
	int frameSize;

	pinmodel = (md4Header_t*)buffer;

	version = LittleLong (pinmodel->version);
	if(version != MD4_VERSION){
		ri.Printf(PRINT_WARNING, "R_LoadMD4: %s has wrong version (%i should be %i)\n",
			mod_name, version, MD4_VERSION);
		return qfalse;
	}

	mod->type = MOD_MD4;
	size = LittleLong(pinmodel->ofsEnd);
	mod->dataSize	+= size;
	mod->modelData	= md4 = ri.hunkalloc(size, Hlow);

	Q_Memcpy(md4, buffer, size);

	LL(md4->ident);
	LL(md4->version);
	LL(md4->numFrames);
	LL(md4->numBones);
	LL(md4->numLODs);
	LL(md4->ofsFrames);
	LL(md4->ofsLODs);
	md4->ofsEnd = size;

	if(md4->numFrames < 1){
		ri.Printf(PRINT_WARNING, "R_LoadMD4: %s has no frames\n", mod_name);
		return qfalse;
	}

	/* we don't need to swap tags in the renderer, they aren't used */

	/* swap all the frames */
	frameSize = (size_t)(&((md4Frame_t*)0)->bones[ md4->numBones ]);
	for(i = 0; i < md4->numFrames; i++){
		frame = (md4Frame_t*)((byte*)md4 + md4->ofsFrames + i * frameSize);
		frame->radius = LittleFloat(frame->radius);
		for(j = 0; j < 3; j++){
			frame->bounds[0][j]	= LittleFloat(frame->bounds[0][j]);
			frame->bounds[1][j]	= LittleFloat(frame->bounds[1][j]);
			frame->localOrigin[j]	= LittleFloat(frame->localOrigin[j]);
		}
		for(j = 0; j < md4->numBones * sizeof(md4Bone_t) / 4; j++)
			((float*)frame->bones)[j] = LittleFloat(((float*)frame->bones)[j]);
	}

	/* swap all the LOD's */
	lod = (md4LOD_t*)((byte*)md4 + md4->ofsLODs);
	for(lodindex = 0; lodindex < md4->numLODs; lodindex++){

		/* swap all the surfaces */
		surf = (md4Surface_t*)((byte*)lod + lod->ofsSurfaces);
		for(i = 0; i < lod->numSurfaces; i++){
			LL(surf->ident);
			LL(surf->numTriangles);
			LL(surf->ofsTriangles);
			LL(surf->numVerts);
			LL(surf->ofsVerts);
			LL(surf->ofsEnd);

			if(surf->numVerts > SHADER_MAX_VERTEXES){
				ri.Printf(PRINT_WARNING,
					"R_LoadMD4: %s has more than %i verts on a surface (%i).\n",
					mod_name, SHADER_MAX_VERTEXES,
					surf->numVerts);
				return qfalse;
			}
			if(surf->numTriangles*3 > SHADER_MAX_INDEXES){
				ri.Printf(PRINT_WARNING,
					"R_LoadMD4: %s has more than %i triangles on a surface (%i).\n",
					mod_name, SHADER_MAX_INDEXES / 3,
					surf->numTriangles);
				return qfalse;
			}

			/* change to surface identifier */
			surf->ident = SF_MD4;

			/* lowercase the surface name so skin compares are faster */
			Q_strlwr(surf->name);

			/* register the shaders */
			sh = R_FindShader(surf->shader, LIGHTMAP_NONE, qtrue);
			if(sh->defaultShader){
				surf->shaderIndex = 0;
			}else{
				surf->shaderIndex = sh->index;
			}

			/* swap all the triangles */
			tri = (md4Triangle_t*)((byte*)surf + surf->ofsTriangles);
			for(j = 0; j < surf->numTriangles; j++, tri++){
				LL(tri->indexes[0]);
				LL(tri->indexes[1]);
				LL(tri->indexes[2]);
			}

			/* swap all the vertexes
			 * FIXME
			 * This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
			 * in for reference.
			 * v = (md4Vertex_t *) ( (byte *)surf + surf->ofsVerts + 12); */
			v = (md4Vertex_t*)((byte*)surf + surf->ofsVerts);
			for(j = 0; j < surf->numVerts; j++){
				v->normal[0]	= LittleFloat(v->normal[0]);
				v->normal[1]	= LittleFloat(v->normal[1]);
				v->normal[2]	= LittleFloat(v->normal[2]);

				v->texCoords[0] = LittleFloat(v->texCoords[0]);
				v->texCoords[1] = LittleFloat(v->texCoords[1]);

				v->numWeights = LittleLong(v->numWeights);

				for(k = 0; k < v->numWeights; k++){
					v->weights[k].boneIndex		= LittleLong(v->weights[k].boneIndex);
					v->weights[k].boneWeight	= LittleFloat(
						v->weights[k].boneWeight);
					v->weights[k].offset[0] = LittleFloat(v->weights[k].offset[0]);
					v->weights[k].offset[1] = LittleFloat(v->weights[k].offset[1]);
					v->weights[k].offset[2] = LittleFloat(v->weights[k].offset[2]);
				}
				/* FIXME
				 * This makes TFC's skeletons work.  Shouldn't be necessary anymore, but left
				 * in for reference.
				 * v = (md4Vertex_t *)( ( byte * )&v->weights[v->numWeights] + 12 ); */
				v = (md4Vertex_t*)(( byte* )&v->weights[v->numWeights]);
			}

			/* find the next surface */
			surf = (md4Surface_t*)((byte*)surf + surf->ofsEnd);
		}

		/* find the next LOD */
		lod = (md4LOD_t*)((byte*)lod + lod->ofsEnd);
	}

	return qtrue;
}



/* ============================================================================= */

/*
** RE_BeginRegistration
*/
void
RE_BeginRegistration(Glconfig *glconfigOut)
{

	R_Init();

	*glconfigOut = glConfig;

	R_SyncRenderThread();

	tr.viewCluster = -1;	/* force markleafs to regenerate */
	R_ClearFlares();
	RE_ClearScene();

	tr.registered = qtrue;

	/* NOTE: this sucks, for some reason the first stretch pic is never drawn
	 * without this we'd see a white flash on a level load because the very
	 * first time the level shot would not be drawn */
/*	RE_StretchPic(0, 0, 0, 0, 0, 0, 1, 1, 0); */
}

/* ============================================================================= */

/*
 * R_ModelInit
 */
void
R_ModelInit(void)
{
	model_t *mod;

	/* leave a space for NULL model */
	tr.numModels = 0;

	mod = R_AllocModel();
	mod->type = MOD_BAD;
}


/*
 * R_Modellist_f
 */
void
R_Modellist_f(void)
{
	int i, j;
	model_t *mod;
	int	total;
	int	lods;

	total = 0;
	for(i = 1; i < tr.numModels; i++){
		mod	= tr.models[i];
		lods	= 1;
		for(j = 1; j < MD3_MAX_LODS; j++)
			if(mod->md3[j] && mod->md3[j] != mod->md3[j-1]){
				lods++;
			}
		ri.Printf(PRINT_ALL, "%8i : (%i) %s\n",mod->dataSize, lods, mod->name);
		total += mod->dataSize;
	}
	ri.Printf(PRINT_ALL, "%8i : Total models\n", total);

#if     0	/* not working right with new hunk */
	if(tr.world){
		ri.Printf(PRINT_ALL, "\n%8i : %s\n", tr.world->dataSize, tr.world->name);
	}
#endif
}


/* ============================================================================= */


/*
 * R_GetTag
 */
static MD3tag *
R_GetTag(MD3header *mod, int frame, const char *tagName)
{
	MD3tag *tag;
	int i;

	if(frame >= mod->numFrames){
		/* it is possible to have a bad frame while changing models, so don't error */
		frame = mod->numFrames - 1;
	}

	tag = (MD3tag*)((byte*)mod + mod->ofsTags) + frame * mod->numTags;
	for(i = 0; i < mod->numTags; i++, tag++)
		if(!strcmp(tag->name, tagName)){
			return tag;	/* found it */
		}

	return NULL;
}

/*
 * R_LerpTag
 */
int
R_LerpTag(Orient *tag, Handle handle, int startFrame, int endFrame,
	  float frac, const char *tagName)
{
	MD3tag	*start, *end;
	int i;
	float frontLerp, backLerp;
	model_t *model;

	model = R_GetModelByHandle(handle);
	if(!model->md3[0]){
		if(model->type == MOD_IQM){
			return R_IQMLerpTag(tag, model->modelData,
				startFrame, endFrame,
				frac, tagName);
		}else{

			clearaxis(tag->axis);
			clearv3(tag->origin);
			return qfalse;

		}
	}else{
		start	= R_GetTag(model->md3[0], startFrame, tagName);
		end	= R_GetTag(model->md3[0], endFrame, tagName);
		if(!start || !end){
			clearaxis(tag->axis);
			clearv3(tag->origin);
			return qfalse;
		}
	}

	frontLerp	= frac;
	backLerp	= 1.0f - frac;

	for(i = 0; i < 3; i++){
		tag->origin[i]	= start->origin[i] * backLerp +  end->origin[i] * frontLerp;
		tag->axis[0][i] = start->axis[0][i] * backLerp +  end->axis[0][i] * frontLerp;
		tag->axis[1][i] = start->axis[1][i] * backLerp +  end->axis[1][i] * frontLerp;
		tag->axis[2][i] = start->axis[2][i] * backLerp +  end->axis[2][i] * frontLerp;
	}
	normv3(tag->axis[0]);
	normv3(tag->axis[1]);
	normv3(tag->axis[2]);
	return qtrue;
}


/*
 * R_ModelBounds
 */
void
R_ModelBounds(Handle handle, Vec3 mins, Vec3 maxs)
{
	model_t *model;

	model = R_GetModelByHandle(handle);

	if(model->type == MOD_BRUSH){
		copyv3(model->bmodel->bounds[0], mins);
		copyv3(model->bmodel->bounds[1], maxs);

		return;
	}else if(model->type == MOD_MESH){
		MD3header *header;
		MD3frame *frame;

		header	= model->md3[0];
		frame	= (MD3frame*)((byte*)header + header->ofsFrames);

		copyv3(frame->bounds[0], mins);
		copyv3(frame->bounds[1], maxs);

		return;
	}else if(model->type == MOD_MD4){
		md4Header_t *header;
		md4Frame_t *frame;

		header	= (md4Header_t*)model->modelData;
		frame	= (md4Frame_t*)((byte*)header + header->ofsFrames);

		copyv3(frame->bounds[0], mins);
		copyv3(frame->bounds[1], maxs);

		return;
	}else if(model->type == MOD_IQM){
		iqmData_t *iqmData;

		iqmData = model->modelData;

		if(iqmData->bounds){
			copyv3(iqmData->bounds, mins);
			copyv3(iqmData->bounds + 3, maxs);
			return;
		}
	}

	clearv3(mins);
	clearv3(maxs);
}
