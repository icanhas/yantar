/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
/* model loading and caching */

#include "local.h"

#define LL(x) x=LittleLong(x)

static qbool R_LoadMD3(model_t *mod, int lod, void *buffer, int bufferSize, const char *modName);
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
	int	size;
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

		size = ri.FS_ReadFile(namebuf, &buf.v);
		if(!buf.u)
			continue;

		ident = LittleLong(*(unsigned*)buf.u);
		if(ident == MD4_IDENT)
			loaded = R_LoadMD4(mod, buf.u, name);
		else{
			if(ident == MD3_IDENT)
				loaded = R_LoadMD3(mod, lod, buf.u, size, name);
			else
				ri.Printf(PRINT_WARNING,"R_RegisterMD3: unknown fileid for %s\n", name);
		}

		ri.FS_FreeFile(buf.v);

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
			mod->mdv[lod] = mod->mdv[lod + 1];
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

	filesize = ri.FS_ReadFile(name, (void**)&buf.v);
	if(!buf.u){
		mod->type = MOD_BAD;
		return 0;
	}

	loaded = R_LoadIQM(mod, buf.u, filesize, name);

	ri.FS_FreeFile (buf.v);

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

	mod = ri.Hunk_Alloc(sizeof(*tr.models[tr.numModels]), h_low);
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
R_LoadMD3(model_t * mod, int lod, void *buffer, int bufferSize, const char *modName)
{
	int f, i, j, k;

	MD3header *md3Model;
	MD3frame *md3Frame;
	MD3surf *md3Surf;
	MD3shader *md3Shader;
	MD3tri *md3Tri;
	md3St_t *md3st;
	MD3xyznorm *md3xyz;
	MD3tag *md3Tag;

	mdvModel_t	*mdvModel;
	mdvFrame_t	*frame;
	mdvSurface_t *surf;	/* , *surface; */
	int *shaderIndex;
	srfTriangle_t	*tri;
	mdvVertex_t	*v;
	mdvSt_t *st;
	mdvTag_t *tag;
	mdvTagName_t *tagName;

	int	version;
	int	size;

	UNUSED(bufferSize);
	md3Model = (MD3header*)buffer;

	version = LittleLong(md3Model->version);
	if(version != MD3_VERSION){
		ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has wrong version (%i should be %i)\n", modName,
			version,
			MD3_VERSION);
		return qfalse;
	}

	mod->type = MOD_MESH;
	size = LittleLong(md3Model->ofsEnd);
	mod->dataSize += size;
	mdvModel = mod->mdv[lod] = ri.Hunk_Alloc(sizeof(mdvModel_t), h_low);

/*  Q_Memcpy(mod->md3[lod], buffer, LittleLong(md3Model->ofsEnd)); */

	LL(md3Model->ident);
	LL(md3Model->version);
	LL(md3Model->numFrames);
	LL(md3Model->numTags);
	LL(md3Model->numSurfaces);
	LL(md3Model->ofsFrames);
	LL(md3Model->ofsTags);
	LL(md3Model->ofsSurfaces);
	LL(md3Model->ofsEnd);

	if(md3Model->numFrames < 1){
		ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has no frames\n", modName);
		return qfalse;
	}

	/* swap all the frames */
	mdvModel->numFrames = md3Model->numFrames;
	mdvModel->frames = frame = ri.Hunk_Alloc(sizeof(*frame) * md3Model->numFrames, h_low);

	md3Frame = (MD3frame*)((byte*)md3Model + md3Model->ofsFrames);
	for(i = 0; i < md3Model->numFrames; i++, frame++, md3Frame++){
		frame->radius = LittleFloat(md3Frame->radius);
		for(j = 0; j < 3; j++){
			frame->bounds[0][j]	= LittleFloat(md3Frame->bounds[0][j]);
			frame->bounds[1][j]	= LittleFloat(md3Frame->bounds[1][j]);
			frame->localOrigin[j]	= LittleFloat(md3Frame->localOrigin[j]);
		}
	}

	/* swap all the tags */
	mdvModel->numTags = md3Model->numTags;
	mdvModel->tags = tag = ri.Hunk_Alloc(sizeof(*tag) * (md3Model->numTags * md3Model->numFrames), h_low);

	md3Tag = (MD3tag*)((byte*)md3Model + md3Model->ofsTags);
	for(i = 0; i < md3Model->numTags * md3Model->numFrames; i++, tag++, md3Tag++)
		for(j = 0; j < 3; j++){
			tag->origin[j]	= LittleFloat(md3Tag->origin[j]);
			tag->axis[0][j] = LittleFloat(md3Tag->axis[0][j]);
			tag->axis[1][j] = LittleFloat(md3Tag->axis[1][j]);
			tag->axis[2][j] = LittleFloat(md3Tag->axis[2][j]);
		}


	mdvModel->tagNames = tagName = ri.Hunk_Alloc(sizeof(*tagName) * (md3Model->numTags), h_low);

	md3Tag = (MD3tag*)((byte*)md3Model + md3Model->ofsTags);
	for(i = 0; i < md3Model->numTags; i++, tagName++, md3Tag++)
		Q_strncpyz(tagName->name, md3Tag->name, sizeof(tagName->name));

	/* swap all the surfaces */
	mdvModel->numSurfaces = md3Model->numSurfaces;
	mdvModel->surfaces = surf = ri.Hunk_Alloc(sizeof(*surf) * md3Model->numSurfaces, h_low);

	md3Surf = (MD3surf*)((byte*)md3Model + md3Model->ofsSurfaces);
	for(i = 0; i < md3Model->numSurfaces; i++){
		LL(md3Surf->ident);
		LL(md3Surf->flags);
		LL(md3Surf->numFrames);
		LL(md3Surf->numShaders);
		LL(md3Surf->numTriangles);
		LL(md3Surf->ofsTriangles);
		LL(md3Surf->numVerts);
		LL(md3Surf->ofsShaders);
		LL(md3Surf->ofsSt);
		LL(md3Surf->ofsXyzNormals);
		LL(md3Surf->ofsEnd);

		if(md3Surf->numVerts > SHADER_MAX_VERTEXES){
			ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has more than %i verts on a surface (%i)",
				modName, SHADER_MAX_VERTEXES, md3Surf->numVerts);
			return qfalse;
		}
		if(md3Surf->numTriangles * 3 > SHADER_MAX_INDEXES){
			ri.Printf(PRINT_WARNING, "R_LoadMD3: %s has more than %i triangles on a surface (%i)",
				modName, SHADER_MAX_INDEXES / 3, md3Surf->numTriangles);
			return qfalse;
		}

		/* change to surface identifier */
		surf->surfaceType = SF_MDV;

		/* give pointer to model for Tess_SurfaceMDX */
		surf->model = mdvModel;

		/* copy surface name */
		Q_strncpyz(surf->name, md3Surf->name, sizeof(surf->name));

		/* lowercase the surface name so skin compares are faster */
		Q_strlwr(surf->name);

		/* strip off a trailing _1 or _2
		 * this is a crutch for q3data being a mess */
		j = strlen(surf->name);
		if(j > 2 && surf->name[j - 2] == '_'){
			surf->name[j - 2] = 0;
		}

		/* register the shaders */
		surf->numShaderIndexes = md3Surf->numShaders;
		surf->shaderIndexes = shaderIndex = ri.Hunk_Alloc(sizeof(*shaderIndex) * md3Surf->numShaders,
			h_low);

		md3Shader = (MD3shader*)((byte*)md3Surf + md3Surf->ofsShaders);
		for(j = 0; j < md3Surf->numShaders; j++, shaderIndex++, md3Shader++){
			material_t *sh;

			sh = R_FindShader(md3Shader->name, LIGHTMAP_NONE, qtrue);
			if(sh->defaultShader){
				*shaderIndex = 0;
			}else{
				*shaderIndex = sh->index;
			}
		}

		/* swap all the triangles */
		surf->numTriangles = md3Surf->numTriangles;
		surf->triangles = tri = ri.Hunk_Alloc(sizeof(*tri) * md3Surf->numTriangles, h_low);

		md3Tri = (MD3tri*)((byte*)md3Surf + md3Surf->ofsTriangles);
		for(j = 0; j < md3Surf->numTriangles; j++, tri++, md3Tri++){
			tri->indexes[0] = LittleLong(md3Tri->indexes[0]);
			tri->indexes[1] = LittleLong(md3Tri->indexes[1]);
			tri->indexes[2] = LittleLong(md3Tri->indexes[2]);
		}

		R_CalcSurfaceTriangleNeighbors(surf->numTriangles, surf->triangles);

		/* swap all the XyzNormals */
		surf->numVerts = md3Surf->numVerts;
		surf->verts = v = ri.Hunk_Alloc(sizeof(*v) * (md3Surf->numVerts * md3Surf->numFrames), h_low);

		md3xyz = (MD3xyznorm*)((byte*)md3Surf + md3Surf->ofsXyzNormals);
		for(j = 0; j < md3Surf->numVerts * md3Surf->numFrames; j++, md3xyz++, v++){
			unsigned lat, lng;
			unsigned short normal;

			v->xyz[0]	= LittleShort(md3xyz->xyz[0]) * MD3_XYZ_SCALE;
			v->xyz[1]	= LittleShort(md3xyz->xyz[1]) * MD3_XYZ_SCALE;
			v->xyz[2]	= LittleShort(md3xyz->xyz[2]) * MD3_XYZ_SCALE;

			normal = LittleShort(md3xyz->normal);

			lat	= (normal >> 8) & 0xff;
			lng	= (normal & 0xff);
			lat	*= (FUNCTABLE_SIZE/256);
			lng	*= (FUNCTABLE_SIZE/256);

			/* decode X as cos( lat ) * sin( long )
			 * decode Y as sin( lat ) * sin( long )
			 * decode Z as cos( long ) */

			v->normal[0] =
				tr.sinTable[(lat+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK] * tr.sinTable[lng];
			v->normal[1]	= tr.sinTable[lat] * tr.sinTable[lng];
			v->normal[2]	= tr.sinTable[(lng+(FUNCTABLE_SIZE/4))&FUNCTABLE_MASK];
		}

		/* swap all the ST */
		surf->st = st = ri.Hunk_Alloc(sizeof(*st) * md3Surf->numVerts, h_low);

		md3st = (md3St_t*)((byte*)md3Surf + md3Surf->ofsSt);
		for(j = 0; j < md3Surf->numVerts; j++, md3st++, st++){
			st->st[0]	= LittleFloat(md3st->st[0]);
			st->st[1]	= LittleFloat(md3st->st[1]);
		}

		/* calc tangent spaces */
		{
			const float *v0, *v1, *v2;
			const float *t0, *t1, *t2;
			Vec3	tangent;
			Vec3	bitangent;
			Vec3	normal;

			for(j = 0, v = surf->verts; j < (surf->numVerts * mdvModel->numFrames); j++, v++){
				clearv3(v->tangent);
				clearv3(v->bitangent);
				if(r_recalcMD3Normals->integer)
					clearv3(v->normal);
			}

			for(f = 0; f < mdvModel->numFrames; f++){
				for(j = 0, tri = surf->triangles; j < surf->numTriangles; j++, tri++){
					v0	= surf->verts[surf->numVerts * f + tri->indexes[0]].xyz;
					v1	= surf->verts[surf->numVerts * f + tri->indexes[1]].xyz;
					v2	= surf->verts[surf->numVerts * f + tri->indexes[2]].xyz;

					t0	= surf->st[tri->indexes[0]].st;
					t1	= surf->st[tri->indexes[1]].st;
					t2	= surf->st[tri->indexes[2]].st;

					if(!r_recalcMD3Normals->integer)
						copyv3(v->normal, normal);

					#if 1
					R_CalcTangentSpace(tangent, bitangent, normal, v0, v1, v2, t0, t1, t2);
					#else
					R_CalcNormalForTriangle(normal, v0, v1, v2);
					R_CalcTangentsForTriangle(tangent, bitangent, v0, v1, v2, t0, t1, t2);
					#endif

					for(k = 0; k < 3; k++){
						float *v;

						v = surf->verts[surf->numVerts * f + tri->indexes[k]].tangent;
						addv3(v, tangent, v);

						v =
							surf->verts[surf->numVerts * f +
								    tri->indexes[k]].bitangent;
						addv3(v, bitangent, v);

						if(r_recalcMD3Normals->integer){
							v =
								surf->verts[surf->numVerts * f +
									    tri->indexes[k]].normal;
							addv3(v, normal, v);
						}
					}
				}
			}

			for(j = 0, v = surf->verts; j < (surf->numVerts * mdvModel->numFrames); j++, v++){
				normv3(v->tangent);
				normv3(v->bitangent);
				normv3(v->normal);
			}
		}

		/* find the next surface */
		md3Surf = (MD3surf*)((byte*)md3Surf + md3Surf->ofsEnd);
		surf++;
	}

	{
		srfVBOMDVMesh_t *vboSurf;

		mdvModel->numVBOSurfaces = mdvModel->numSurfaces;
		mdvModel->vboSurfaces = ri.Hunk_Alloc(sizeof(*mdvModel->vboSurfaces) * mdvModel->numSurfaces,
			h_low);

		vboSurf = mdvModel->vboSurfaces;
		surf = mdvModel->surfaces;
		for(i = 0; i < mdvModel->numSurfaces; i++, vboSurf++, surf++){
			Vec3	*verts;
			Vec3	*normals;
			Vec3	*tangents;
			Vec3	*bitangents;
			Vec2	*texcoords;

			byte	*data;
			int	dataSize;

			int	ofs_xyz, ofs_normal, ofs_st, ofs_tangent, ofs_bitangent;

			dataSize = 0;

			ofs_xyz		= dataSize;
			dataSize	+= surf->numVerts * mdvModel->numFrames * sizeof(*verts);

			ofs_normal	= dataSize;
			dataSize	+= surf->numVerts * mdvModel->numFrames * sizeof(*normals);

			ofs_tangent = dataSize;
			dataSize += surf->numVerts * mdvModel->numFrames * sizeof(*tangents);

			ofs_bitangent = dataSize;
			dataSize += surf->numVerts * mdvModel->numFrames * sizeof(*bitangents);

			ofs_st		= dataSize;
			dataSize	+= surf->numVerts * sizeof(*texcoords);

			data = ri.Malloc(dataSize);

			verts		=      (void*)(data + ofs_xyz);
			normals		=    (void*)(data + ofs_normal);
			tangents	=   (void*)(data + ofs_tangent);
			bitangents	= (void*)(data + ofs_bitangent);
			texcoords	=  (void*)(data + ofs_st);

			v = surf->verts;
			for(j = 0; j < surf->numVerts * mdvModel->numFrames; j++, v++){
				copyv3(v->xyz,       verts[j]);
				copyv3(v->normal,    normals[j]);
				copyv3(v->tangent,   tangents[j]);
				copyv3(v->bitangent, bitangents[j]);
			}

			st = surf->st;
			for(j = 0; j < surf->numVerts; j++, st++){
				texcoords[j][0] = st->st[0];
				texcoords[j][1] = st->st[1];
			}

			vboSurf->surfaceType = SF_VBO_MDVMESH;
			vboSurf->mdvModel	= mdvModel;
			vboSurf->mdvSurface	= surf;
			vboSurf->numIndexes	= surf->numTriangles * 3;
			vboSurf->numVerts	= surf->numVerts;
			vboSurf->vbo = R_CreateVBO(va("staticMD3Mesh_VBO '%s'",
					surf->name), data, dataSize, VBO_USAGE_STATIC);

			vboSurf->vbo->ofs_xyz = ofs_xyz;
			vboSurf->vbo->ofs_normal	= ofs_normal;
			vboSurf->vbo->ofs_tangent	= ofs_tangent;
			vboSurf->vbo->ofs_bitangent	= ofs_bitangent;
			vboSurf->vbo->ofs_st = ofs_st;

			vboSurf->vbo->stride_xyz = sizeof(*verts);
			vboSurf->vbo->stride_normal	= sizeof(*normals);
			vboSurf->vbo->stride_tangent	= sizeof(*tangents);
			vboSurf->vbo->stride_bitangent	= sizeof(*bitangents);
			vboSurf->vbo->stride_st		= sizeof(*st);

			vboSurf->vbo->size_xyz = sizeof(*verts) * surf->numVerts;
			vboSurf->vbo->size_normal = sizeof(*normals) * surf->numVerts;

			ri.Free(data);

			vboSurf->ibo = R_CreateIBO2(va("staticMD3Mesh_IBO %s",
					surf->name), surf->numTriangles, surf->triangles, VBO_USAGE_STATIC);
		}
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
	mod->modelData	= md4 = ri.Hunk_Alloc(size, h_low);

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

	tr.visIndex = 0;
	memset(tr.visClusters, -2, sizeof(tr.visClusters));	/* force markleafs to regenerate */

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
			if(mod->mdv[j] && mod->mdv[j] != mod->mdv[j-1]){
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
static mdvTag_t *
R_GetTag(mdvModel_t *mod, int frame, const char *_tagName)
{
	int i;
	mdvTag_t *tag;
	mdvTagName_t *tagName;

	if(frame >= mod->numFrames){
		/* it is possible to have a bad frame while changing models, so don't error */
		frame = mod->numFrames - 1;
	}

	tag = mod->tags + frame * mod->numTags;
	tagName = mod->tagNames;
	for(i = 0; i < mod->numTags; i++, tag++, tagName++)
		if(!strcmp(tagName->name, _tagName)){
			return tag;
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
	mdvTag_t	*start, *end;
	int i;
	float frontLerp, backLerp;
	model_t *model;

	model = R_GetModelByHandle(handle);
	if(!model->mdv[0]){
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
		start	= R_GetTag(model->mdv[0], startFrame, tagName);
		end	= R_GetTag(model->mdv[0], endFrame, tagName);
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
		mdvModel_t *header;
		mdvFrame_t *frame;

		header	= model->mdv[0];
		frame	= header->frames;

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
