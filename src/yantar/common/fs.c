/* Handle-based filesystem */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */
 
#include "shared.h"
#include "common.h"
#include "unzip.h"

/*
 * The Virtual Filesystem
 * FIXME: documentation is not code.
 *
 * All of Yantar's data access is through a hierarchical file system, but the contents of
 * the file system can be transparently merged from several sources.
 *
 * A "qpath" is a reference to game file data.  MAX_ZPATH is 256 characters, which must include
 * a terminating zero. "..", "\\", and ":" are explicitly illegal in qpaths to prevent any
 * references outside the quake directory system.
 *
 * The "base path" is the path to the directory holding all the game directories and usually
 * the executable.  It defaults to ".", but can be overridden with a "+set fs_basepath c:\quake3"
 * command line to allow code debugging in a different directory.  Basepath cannot
 * be modified at all after startup.  Any files that are created (demos, screenshots,
 * etc) will be created reletive to the base path, so base path should usually be writable.
 *
 * The "home path" is the path used for all write access. On win32 systems we have "base path"
 * == "home path", but on *nix systems the base installation is usually readonly, and
 * "home path" points to ~/.q3a or similar
 *
 * The user can also install custom mods and content in "home path", so it should be searched
 * along with "home path" and "cd path" for game content.
 *
 *
 * The "base game" is the directory under the paths where data comes from by default, and
 * can be either "baseq3" or "demoq3".
 *
 * The "current game" may be the same as the base game, or it may be the name of another
 * directory under the paths that should be searched for files before looking in the base game.
 * This is the basis for addons.
 *
 * Clients automatically set the game directory after receiving a gamestate from a server,
 * so only servers need to worry about +set fs_game.
 *
 * No other directories outside of the base game and current game will ever be referenced by
 * filesystem functions.
 *
 * To save disk space and speed loading, directory trees can be collapsed into zip files.
 * The files use a ".pk3" extension to prevent users from unzipping them accidentally, but
 * otherwise the are simply normal uncompressed zip files.  A game directory can have multiple
 * zip files of the form "pak0.pk3", "pak1.pk3", etc.  Zip files are searched in decending order
 * from the highest number to the lowest, and will always take precedence over the filesystem.
 * This allows a pk3 distributed as a patch to override all existing data.
 *
 * File search order: when FS_FOpenFileRead gets called it will go through the fs_searchpaths
 * structure and stop on the first successful hit. fs_searchpaths is built with successive
 * calls to FS_AddGameDirectory
 *
 * Additionally, we search in several subdirectories:
 * current game is the current mode
 * base game is a variable to allow mods based on other mods
 * (such as baseq3 + missionpack content combination in a mod for instance)
 * BASEGAME is the hardcoded base game ("baseq3")
 *
 * e.g. the qpath "sound/newstuff/test.wav" would be searched for in the following places:
 *
 * home path + current game's zip files
 * home path + current game's directory
 * base path + current game's zip files
 * base path + current game's directory
 * cd path + current game's zip files
 * cd path + current game's directory
 *
 * home path + base game's zip file
 * home path + base game's directory
 * base path + base game's zip file
 * base path + base game's directory
 * cd path + base game's zip file
 * cd path + base game's directory
 *
 * home path + BASEGAME's zip file
 * home path + BASEGAME's directory
 * base path + BASEGAME's zip file
 * base path + BASEGAME's directory
 * cd path + BASEGAME's zip file
 * cd path + BASEGAME's directory
 *
 * server download, to be written to home path + current game's directory
 *
 *
 * The filesystem can be safely shutdown and reinitialized with different
 * basedir / cddir / game combinations, but all other subsystems that rely on it
 * (sound, video) must also be forced to restart.
 *
 * Because the same files are loaded by both the clip model (CM_) and renderer (TR_)
 * subsystems, a simple single-file caching scheme is used.  The CM_ subsystems will
 * load the file with a request to cache.  Only one file will be kept cached at a time,
 * so any models that are going to be referenced by both subsystems should alternate
 * between the CM_ load function and the ref load function.
 *
 * TODO: A qpath that starts with a leading slash will always refer to the base game, even if another
 * game is currently active.  This allows character models, skins, and sounds to be downloaded
 * to a common directory no matter which game is active.
 *
 * How to prevent downloading zip files?
 * Pass pk3 file names in systeminfo, and download before FS_Restart()?
 *
 * Aborting a download disconnects the client from the server.
 *
 * How to mark files as downloadable?  Commercial add-ons won't be downloadable.
 *
 * Non-commercial downloads will want to download the entire zip file.
 * the game would have to be reset to actually read the zip in
 *
 * Auto-update information
 *
 * Path separators
 *
 * Casing
 *
 * separate server gamedir and client gamedir, so if the user starts
 * a local game after having connected to a network game, it won't stick
 * with the network game.
 *
 * todo:
 *
 * downloading (outside fs?)
 * game directory passing and restarting
 */

/* 
 * every time a new demo pk3 file is built, this checksum must be updated.
 * the easiest way to get it is to just run the game and see what it spits out 
 */
#define DEMO_PAK0_CHECKSUM 2985612116u
static const unsigned int	pak_checksums[] = 
{
	1566731103u,
	298122907u,
	412165236u,
	2991495316u,
	1197932710u,
	4087071573u,
	3709064859u,
	908855077u,
	977125798u
};

static const unsigned int	missionpak_checksums[] =
{
	2430342401u,
	511014160u,
	2662638993u,
	1438664554u
};

#define MAX_ZPATH		256
#define MAX_SEARCH_PATHS	4096
#define MAX_FILEHASH_SIZE	1024

typedef struct fileInPack_t fileInPack_t;
typedef struct pack_t pack_t;
typedef struct directory_t directory_t;
typedef struct searchpath_t searchpath_t;
typedef union qfile_gut qfile_gut;
typedef struct qfile_ut qfile_ut;
typedef struct fileHandleData_t fileHandleData_t;

struct fileInPack_t {
	char			*name;	/* name of the file */
	unsigned long		pos;	/* file info position in zip */
	unsigned long		len;	/* uncompress file size */
	fileInPack_t		*next;	/* next file in the hash */
};

struct pack_t {
	char		pakPathname[MAX_OSPATH];	/* c:\quake3\baseq3 */
	char		pakFilename[MAX_OSPATH];	/* c:\quake3\baseq3\pak0.pk3 */
	char		pakBasename[MAX_OSPATH];	/* pak0 */
	char		pakGamename[MAX_OSPATH];	/* baseq3 */
	unzFile		handle;				/* handle to zip file */
	int		checksum;			/* regular checksum */
	int		pure_checksum;			/* checksum for pure */
	int		numfiles;			/* number of files in pk3 */
	int		referenced;			/* referenced file flags */
	int		hashSize;			/* (power of 2) */
	fileInPack_t	**hashTable;
	fileInPack_t	* buildBuffer;			/* buffer with the filenames etc. */
};

struct directory_t {
	char	path[MAX_OSPATH];	/* c:\quake3 */
	char	fullpath[MAX_OSPATH];	/* c:\quake3\baseq3 */
	char	gamedir[MAX_OSPATH];	/* baseq3 */
};

struct searchpath_t {
	searchpath_t	*next;
	pack_t		*pack;	/* only one of pack / dir will be non NULL */
	directory_t	*dir;
};

union qfile_gut {
	FILE	* o;
	unzFile z;
};

struct qfile_ut {
	qfile_gut	file;
	qbool		unique;
};

struct fileHandleData_t {
	qfile_ut	handleFiles;
	qbool		handleSync;
	int		baseOffset;
	int		fileSize;
	int		zipFilePos;
	qbool		zipFile;
	qbool		streamed;
	char		name[MAX_ZPATH];
};

static char fs_gamedir[MAX_OSPATH];	/* this will be a single file name with no separators */
static cvar_t	*fs_debug;
static cvar_t	*fs_homepath;

#ifdef MACOS_X
/* Also search the .app bundle for .pk3 files */
static cvar_t *fs_apppath;
#endif

static cvar_t *fs_basepath;
static cvar_t *fs_basegame;
static cvar_t	*fs_gamedirvar;
static searchpath_t *fs_searchpaths;
static int	fs_readCount;		/* total bytes read */
static int	fs_loadCount;		/* total files read */
static int	fs_loadStack;		/* total files in memory */
static int	fs_packFiles = 0;	/* total number of files in packs */

static int	fs_checksumFeed;

static fileHandleData_t fsh[MAX_FILE_HANDLES];

/* whether we did a reorder on the current search path when joining the server */
static qbool fs_reordered;

/* never load anything from pk3 files that are not present at the server when pure */
static int	fs_numServerPaks = 0;
static int	fs_serverPaks[MAX_SEARCH_PATHS];	/* checksums */
static char *fs_serverPakNames[MAX_SEARCH_PATHS];	/* pk3 names */

/* 
 * only used for autodownload, to make sure the client has at least
 * all the pk3 files that are referenced at the server side 
 */
static int	fs_numServerReferencedPaks;
static int	fs_serverReferencedPaks[MAX_SEARCH_PATHS];	/* checksums */
static char *fs_serverReferencedPakNames[MAX_SEARCH_PATHS];	/* pk3 names */

/* last valid game folder used */
char	lastValidBase[MAX_OSPATH];
char	lastValidGame[MAX_OSPATH];

#ifdef FS_MISSING
FILE *missingFiles = nil;
#endif

/* C99 defines __func__ */
#ifndef __func__
#define __func__ "(unknown)"
#endif

qbool
FS_Initialized(void)
{
	return (fs_searchpaths != NULL);
}

qbool
FS_PakIsPure(pack_t *pack)
{
	int i;

	if(fs_numServerPaks){
		for(i = 0; i < fs_numServerPaks; i++)
			/* FIXME: also use hashed file names
			 * NOTE TTimo: a pk3 with same checksum but different name would be validated too
			 *   I don't see this as allowing for any exploit, it would only happen if the client does manips of its file names 'not a bug' */
			if(pack->checksum == fs_serverPaks[i])
				return qtrue;	/* on the aproved list */
		return qfalse;			/* not on the pure server pak list */
	}
	return qtrue;
}

/* return load stack */
int
FS_LoadStack(void)
{
	return fs_loadStack;
}

/* return a hash value for the filename */
static long
FS_HashFileName(const char *fname, int hashSize)
{
	int i;
	long hash;
	char letter;

	hash = 0;
	i = 0;
	while(fname[i] != '\0'){
		letter = tolower(fname[i]);
		if(letter =='.') break;			/* don't include extension */
		if(letter =='\\') letter = '/';
		if(letter == PATH_SEP) letter = '/';
		hash+=(long)(letter)*(i+119);
		i++;
	}
	hash = (hash ^ (hash >> 10) ^ (hash >> 20));
	hash &= (hashSize-1);
	return hash;
}

static fileHandle_t
FS_HandleForFile(void)
{
	int i;

	for(i = 1; i < MAX_FILE_HANDLES; i++)
		if(fsh[i].handleFiles.file.o == NULL)
			return i;
	Com_Errorf(ERR_DROP, "FS_HandleForFile: none free");
	return 0;
}

static FILE*
FS_FileForHandle(fileHandle_t f)
{
	if(f < 1 || f > MAX_FILE_HANDLES)
		Com_Errorf(ERR_DROP, "FS_FileForHandle: out of range");
	if(fsh[f].zipFile == qtrue)
		Com_Errorf(ERR_DROP,
			"FS_FileForHandle: can't get FILE on zip file");
	if(!fsh[f].handleFiles.file.o)
		Com_Errorf(ERR_DROP, "FS_FileForHandle: NULL");

	return fsh[f].handleFiles.file.o;
}

/* forces flush on files we're writing to. */
void
FS_ForceFlush(fileHandle_t f)
{
	FILE *file;

	file = FS_FileForHandle(f);
	setvbuf(file, NULL, _IONBF, 0);
}

long
FS_fplength(FILE *h)
{
	long	pos;
	long	end;

	pos = ftell(h);
	fseek(h, 0, SEEK_END);
	end = ftell(h);
	fseek(h, pos, SEEK_SET);

	return end;
}

/*
 * If this is called on a non-unique FILE (from a pak file),
 * it will return the size of the pak file, not the expected
 * size of the file.
 */
long
FS_filelength(fileHandle_t f)
{
	FILE *h;

	h = FS_FileForHandle(f);

	if(h == NULL)
		return -1;
	else
		return FS_fplength(h);
}

/* Fix things up differently for win/unix/mac */
static void
FS_ReplaceSeparators(char *path)
{
	char *s;
	qbool lastCharWasSep = qfalse;

	for(s = path; *s; s++){
		if(*s == '/' || *s == '\\'){
			if(!lastCharWasSep){
				*s = PATH_SEP;
				lastCharWasSep = qtrue;
			}else
				memmove (s, s + 1, strlen (s));
		}else
			lastCharWasSep = qfalse;
	}
}

/* Qpath may have either forward or backwards slashes */
char*
FS_BuildOSPath(const char *base, const char *game, const char *qpath)
{
	char temp[MAX_OSPATH];
	static char	ospath[2][MAX_OSPATH];
	static int	toggle;

	toggle ^= 1;	/* flip-flop to allow two returns without clash */

	if(!game || !game[0])
		game = fs_gamedir;

	Q_sprintf(temp, sizeof(temp), "/%s/%s", game, qpath);
	FS_ReplaceSeparators(temp);
	Q_sprintf(ospath[toggle], sizeof(ospath[0]), "%s%s", base, temp);

	return ospath[toggle];
}

/* Creates any directories needed to store the given filename */
qbool
FS_CreatePath(char *OSPath)
{
	char	*ofs;
	char	path[MAX_OSPATH];

	/* make absolutely sure that it can't back up the path
	 * FIXME: is c: allowed??? */
	if(strstr(OSPath, "..") || strstr(OSPath, "::")){
		Com_Printf("WARNING: refusing to create relative path \"%s\"\n",
			OSPath);
		return qtrue;
	}

	Q_strncpyz(path, OSPath, sizeof(path));
	FS_ReplaceSeparators(path);

	/* Skip creation of the root directory as it will always be there */
	ofs = strchr(path, PATH_SEP);
	ofs++;

	for(; ofs != NULL && *ofs; ofs++)
		if(*ofs == PATH_SEP){
			/* create the directory */
			*ofs = 0;
			if(!Sys_Mkdir (path))
				Com_Errorf(ERR_FATAL,
					"FS_CreatePath: failed to create path \"%s\"",
					path);
			*ofs = PATH_SEP;
		}

	return qfalse;
}

/* ERR_FATAL if trying to manipulate a file with the platform library extension */
static void
FS_CheckFilenameIsNotExecutable(const char *filename,
				const char *function)
{
	/* Check if the filename ends with the library extension */
	if(Q_cmpext(filename, DLL_EXT))
		Com_Errorf(
			ERR_FATAL, "%s: Not allowed to manipulate '%s' due "
				   "to %s extension", function, filename,
			DLL_EXT);
}

void
FS_Remove(const char *osPath)
{
	FS_CheckFilenameIsNotExecutable(osPath, __func__);

	remove(osPath);
}

void
FS_HomeRemove(const char *homePath)
{
	FS_CheckFilenameIsNotExecutable(homePath, __func__);

	remove(FS_BuildOSPath(fs_homepath->string,
			fs_gamedir, homePath));
}

/* Tests if path and file exists */
qbool
FS_FileInPathExists(const char *testpath)
{
	FILE *filep;

	filep = fopen(testpath, "rb");

	if(filep){
		fclose(filep);
		return qtrue;
	}

	return qfalse;
}

/*
 * Tests if the file exists in the current gamedir, this DOES NOT
 * search the paths.  This is to determine if opening a file to write
 * (which always goes into the current gamedir) will cause any overwrites.
 * NOTE TTimo: this goes with FS_FOpenFileWrite for opening the file afterwards
 */
qbool
FS_FileExists(const char *file)
{
	return FS_FileInPathExists(FS_BuildOSPath(fs_homepath->string,
			fs_gamedir, file));
}

qbool
FS_SV_FileExists(const char *file)
{
	char *testpath;

	testpath = FS_BuildOSPath(fs_homepath->string, file, "");
	testpath[strlen(testpath)-1] = '\0';

	return FS_FileInPathExists(testpath);
}

fileHandle_t
FS_SV_FOpenFileWrite(const char *filename)
{
	char *ospath;
	fileHandle_t f;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	ospath = FS_BuildOSPath(fs_homepath->string, filename, "");
	ospath[strlen(ospath)-1] = '\0';

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	if(fs_debug->integer)
		Com_Printf("FS_SV_FOpenFileWrite: %s\n", ospath);

	FS_CheckFilenameIsNotExecutable(ospath, __func__);

	if(FS_CreatePath(ospath))
		return 0;

	Com_DPrintf("writing to: %s\n", ospath);
	fsh[f].handleFiles.file.o = fopen(ospath, "wb");

	Q_strncpyz(fsh[f].name, filename, sizeof(fsh[f].name));

	fsh[f].handleSync = qfalse;
	if(!fsh[f].handleFiles.file.o)
		f = 0;
	return f;
}

/*
 * Search for a file somewhere below the home path then base path
 * in that order
 *
 * if uniqueFILE is true, then a new FILE will be fopened even if the file
 * is found in an already open pak file. If uniqueFILE is false, you must call
 * FS_FCloseFile instead of fclose, otherwise the pak FILE would be improperly closed
 * It is generally safe to always set uniqueFILE to true, because the majority of
 * file IO goes through FS_ReadFile, which Does The Right Thing already.
 */
long
FS_SV_FOpenFileRead(const char *filename, fileHandle_t *fp)
{
	char *ospath;
	fileHandle_t f = 0;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz(fsh[f].name, filename, sizeof(fsh[f].name));

	/* don't let sound stutter */
	S_ClearSoundBuffer();

	/* search homepath */
	ospath = FS_BuildOSPath(fs_homepath->string, filename, "");
	/* remove trailing slash */
	ospath[strlen(ospath)-1] = '\0';

	if(fs_debug->integer)
		Com_Printf("FS_SV_FOpenFileRead (fs_homepath): %s\n", ospath);

	fsh[f].handleFiles.file.o = fopen(ospath, "rb");
	fsh[f].handleSync = qfalse;
	if(!fsh[f].handleFiles.file.o){
		/* If fs_homepath == fs_basepath, don't bother */
		if(Q_stricmp(fs_homepath->string,fs_basepath->string)){
			/* search basepath */
			ospath = FS_BuildOSPath(fs_basepath->string, filename,
				"");
			ospath[strlen(ospath)-1] = '\0';

			if(fs_debug->integer)
				Com_Printf(
					"FS_SV_FOpenFileRead (fs_basepath): %s\n",
					ospath);

			fsh[f].handleFiles.file.o = fopen(ospath, "rb");
			fsh[f].handleSync = qfalse;
		}

		if(!fsh[f].handleFiles.file.o)
			f = 0;
	}

	*fp = f;
	if(f)
		return FS_filelength(f);

	return -1;
}

void
FS_SV_Rename(const char *from, const char *to)
{
	char *from_ospath, *to_ospath;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	/* don't let sound stutter */
	S_ClearSoundBuffer();

	from_ospath = FS_BuildOSPath(fs_homepath->string, from, "");
	to_ospath = FS_BuildOSPath(fs_homepath->string, to, "");
	from_ospath[strlen(from_ospath)-1] = '\0';
	to_ospath[strlen(to_ospath)-1] = '\0';

	if(fs_debug->integer)
		Com_Printf("FS_SV_Rename: %s --> %s\n", from_ospath, to_ospath);

	FS_CheckFilenameIsNotExecutable(to_ospath, __func__);

	rename(from_ospath, to_ospath);
}

void
FS_Rename(const char *from, const char *to)
{
	char *from_ospath, *to_ospath;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	/* don't let sound stutter */
	S_ClearSoundBuffer();

	from_ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, from);
	to_ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, to);

	if(fs_debug->integer)
		Com_Printf("FS_Rename: %s --> %s\n", from_ospath, to_ospath);

	FS_CheckFilenameIsNotExecutable(to_ospath, __func__);

	rename(from_ospath, to_ospath);
}

/*
 * If the FILE pointer is an open pak file, leave it open.
 *
 * For some reason, other dll's can't just call fclose()
 * on files returned by FS_FOpenFile...
 */
void
FS_FCloseFile(fileHandle_t f)
{
	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(fsh[f].zipFile == qtrue){
		unzCloseCurrentFile(fsh[f].handleFiles.file.z);
		if(fsh[f].handleFiles.unique)
			unzClose(fsh[f].handleFiles.file.z);
		Q_Memset(&fsh[f], 0, sizeof(fsh[f]));
		return;
	}

	/* we didn't find it as a pak, so close it as a unique file */
	if(fsh[f].handleFiles.file.o)
		fclose (fsh[f].handleFiles.file.o);
	Q_Memset(&fsh[f], 0, sizeof(fsh[f]));
}

fileHandle_t
FS_FOpenFileWrite(const char *filename)
{
	char *ospath;
	fileHandle_t f;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, filename);

	if(fs_debug->integer)
		Com_Printf("FS_FOpenFileWrite: %s\n", ospath);

	FS_CheckFilenameIsNotExecutable(ospath, __func__);

	if(FS_CreatePath(ospath))
		return 0;

	/* enabling the following line causes a recursive function call loop
	 * when running with +set logfile 1 +set developer 1
	 * Com_DPrintf( "writing to: %s\n", ospath ); */
	fsh[f].handleFiles.file.o = fopen(ospath, "wb");

	Q_strncpyz(fsh[f].name, filename, sizeof(fsh[f].name));

	fsh[f].handleSync = qfalse;
	if(!fsh[f].handleFiles.file.o)
		f = 0;
	return f;
}

fileHandle_t
FS_FOpenFileAppend(const char *filename)
{
	char *ospath;
	fileHandle_t f;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz(fsh[f].name, filename, sizeof(fsh[f].name));

	/* don't let sound stutter */
	S_ClearSoundBuffer();

	ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, filename);

	if(fs_debug->integer)
		Com_Printf("FS_FOpenFileAppend: %s\n", ospath);

	FS_CheckFilenameIsNotExecutable(ospath, __func__);

	if(FS_CreatePath(ospath))
		return 0;

	fsh[f].handleFiles.file.o = fopen(ospath, "ab");
	fsh[f].handleSync = qfalse;
	if(!fsh[f].handleFiles.file.o)
		f = 0;
	return f;
}

fileHandle_t
FS_FCreateOpenPipeFile(const char *filename)
{
	char	*ospath;
	FILE	*fifo;
	fileHandle_t f;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	f = FS_HandleForFile();
	fsh[f].zipFile = qfalse;

	Q_strncpyz(fsh[f].name, filename, sizeof(fsh[f].name));

	/* don't let sound stutter */
	S_ClearSoundBuffer();

	ospath = FS_BuildOSPath(fs_homepath->string, fs_gamedir, filename);

	if(fs_debug->integer)
		Com_Printf("FS_FCreateOpenPipeFile: %s\n", ospath);

	FS_CheckFilenameIsNotExecutable(ospath, __func__);

	fifo = Sys_Mkfifo(ospath);
	if(fifo){
		fsh[f].handleFiles.file.o = fifo;
		fsh[f].handleSync = qfalse;
	}else{
		Com_Printf(
			S_COLOR_YELLOW
			"WARNING: Could not create new com_pipefile at %s. "
			"com_pipefile will not be used.\n",
			ospath);
		f = 0;
	}

	return f;
}

/* Ignore case and seprator char distinctions */
qbool
FS_FilenameCompare(const char *s1, const char *s2)
{
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if(c1 >= 'a' && c1 <= 'z')
			c1 -= ('a' - 'A');
		if(c2 >= 'a' && c2 <= 'z')
			c2 -= ('a' - 'A');

		if(c1 == '\\' || c1 == ':')
			c1 = '/';
		if(c2 == '\\' || c2 == ':')
			c2 = '/';

		if(c1 != c2)
			return qtrue;	/* strings not equal */
	} while(c1);

	return qfalse;	/* strings are equal */
}

/* Return qtrue if ext matches file extension filename */
qbool
FS_IsExt(const char *filename, const char *ext, int namelen)
{
	int extlen;

	extlen = strlen(ext);

	if(extlen > namelen)
		return qfalse;

	filename += namelen - extlen;

	return !Q_stricmp(filename, ext);
}

qbool
FS_IsDemoExt(const char *filename, int namelen)
{
	char	*ext_test;
	int	index, protocol;

	ext_test = strrchr(filename, '.');
	if(ext_test &&
	   !Q_stricmpn(ext_test + 1, DEMOEXT, ARRAY_LEN(DEMOEXT) - 1)){
		protocol = atoi(ext_test + ARRAY_LEN(DEMOEXT));

		if(protocol == com_protocol->integer)
			return qtrue;


		for(index = 0; demo_protocols[index]; index++)
			if(demo_protocols[index] == protocol)
				return qtrue;
	}

	return qfalse;
}

/*
 * Tries opening file "filename" in searchpath "search"
 * Returns filesize and an open FILE pointer.
 */
extern qbool com_fullyInitialized;

long
FS_FOpenFileReadDir(const char *filename, searchpath_t *search,
		    fileHandle_t *file, qbool uniqueFILE,
		    qbool unpure)
{
	long hash;
	pack_t *pak;
	fileInPack_t	*pakFile;
	directory_t	*dir;
	char	*netpath;
	FILE    *filep;
	int	len;

	if(filename == NULL)
		Com_Errorf(ERR_FATAL,
			"FS_FOpenFileRead: NULL 'filename' parameter passed");

	/* qpaths are not supposed to have a leading slash */
	if(filename[0] == '/' || filename[0] == '\\')
		filename++;

	/* 
	 * Make absolutely sure that it can't back up the path.
	 * The searchpaths do guarantee that something will always
	 * be prepended, so we don't need to worry about "c:" or "//limbo" 
	 */
	if(strstr(filename, "..") || strstr(filename, "::")){
		if(file == NULL)
			return qfalse;

		*file = 0;
		return -1;
	}

	/* make sure the q3key file is only readable by the quake3.exe at initialization
	 * any other time the key should only be accessed in memory using the provided functions */
	if(com_fullyInitialized && strstr(filename, "q3key")){
		if(file == NULL)
			return qfalse;

		*file = 0;
		return -1;
	}

	if(file == NULL){
		/* just wants to see if file is there */

		/* is the element a pak file? */
		if(search->pack){
			hash = FS_HashFileName(filename, search->pack->hashSize);

			if(search->pack->hashTable[hash]){
				/* look through all the pak file elements */
				pak = search->pack;
				pakFile = pak->hashTable[hash];

				do {
					/* case and separator insensitive comparisons */
					if(!FS_FilenameCompare(pakFile->name,
						   filename)){
						/* found it! */
						if(pakFile->len)
							return pakFile->len;
						else
							/* It's not nice, but legacy code depends
							 * on positive value if file exists no matter
							 * what size */
							return 1;
					}

					pakFile = pakFile->next;
				} while(pakFile != NULL);
			}
		}else if(search->dir){
			dir = search->dir;

			netpath = FS_BuildOSPath(dir->path, dir->gamedir,
				filename);
			filep = fopen (netpath, "rb");

			if(filep){
				len = FS_fplength(filep);
				fclose(filep);

				if(len)
					return len;
				else
					return 1;
			}
		}

		return 0;
	}

	*file = FS_HandleForFile();
	fsh[*file].handleFiles.unique = uniqueFILE;

	/* is the element a pak file? */
	if(search->pack){
		hash = FS_HashFileName(filename, search->pack->hashSize);

		if(search->pack->hashTable[hash]){
			/* disregard if it doesn't match one of the allowed pure pak files */
			if(!unpure && !FS_PakIsPure(search->pack)){
				*file = 0;
				return -1;
			}

			/* look through all the pak file elements */
			pak = search->pack;
			pakFile = pak->hashTable[hash];

			do {
				/* case and separator insensitive comparisons */
				if(!FS_FilenameCompare(pakFile->name,
					   filename)){
					/* found it! */

					/* 
					 * mark the pak as having been referenced and mark specifics on cgame and ui
					 * shaders, txt, arena files  by themselves do not count as a reference as
					 * these are loaded from all pk3s
					 * from every pk3 file.. 
					 */
					len = strlen(filename);

					if(!(pak->referenced & FS_GENERAL_REF)){
						if(!FS_IsExt(filename, ".shader", len)
						   && !FS_IsExt(filename, ".txt", len)
						   && !FS_IsExt(filename, ".cfg", len)
						   && !FS_IsExt(filename, ".config", len)
						   && !FS_IsExt(filename, ".bot", len)
						   && !FS_IsExt(filename, ".arena", len)
						   && !FS_IsExt(filename, ".menu", len)
						   && Q_stricmp(filename,  "game.qvm") != 0
						   && !strstr(filename, "levelshots"))
						then{
							pak->referenced |= FS_GENERAL_REF;
						}
					}

					if(strstr(filename, "cgame.qvm"))
						pak->referenced |= FS_CGAME_REF;
					if(strstr(filename, "ui.qvm"))
						pak->referenced |= FS_UI_REF;

					if(uniqueFILE){
						/* open a new file on the pakfile */
						fsh[*file].handleFiles.file.z =
							unzOpen(pak->pakFilename);

						if(fsh[*file].handleFiles.file.z
						   == NULL)
							Com_Errorf(
								ERR_FATAL,
								"Couldn't open %s",
								pak->pakFilename);
					}else
						fsh[*file].handleFiles.file.z =
							pak->handle;

					Q_strncpyz(fsh[*file].name, filename,
						sizeof(fsh[*file].name));
					fsh[*file].zipFile = qtrue;

					/* set the file position in the zip file (also sets the current file info) */
					unzSetOffset(
						fsh[*file].handleFiles.file.z,
						pakFile->pos);

					/* open the file in the zip */
					unzOpenCurrentFile(
						fsh[*file].handleFiles.file.z);
					fsh[*file].zipFilePos = pakFile->pos;

					if(fs_debug->integer)
						Com_Printf(
							"FS_FOpenFileRead: %s (found in '%s')\n",
							filename,
							pak->pakFilename);

					return pakFile->len;
				}

				pakFile = pakFile->next;
			} while(pakFile != NULL);
		}
	}else if(search->dir){
		/* check a file in the directory tree */

		/* 
		 * if we are running restricted, the only files we
		 * will allow to come from the directory are .cfg files 
		 */
		len = strlen(filename);
		/* FIXME TTimo I'm not sure about the fs_numServerPaks test
		 * if you are using FS_ReadFile to find out if a file exists,
		 *   this test can make the search fail although the file is in the directory
		 * I had the problem on https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=8
		 * turned out I used FS_FileExists instead 
		 */
		if(!unpure && fs_numServerPaks)
			if(!FS_IsExt(filename, ".cfg", len) &&	/* for config files */
			   !FS_IsExt(filename, ".menu", len) &&	/* menu files */
			   !FS_IsExt(filename, ".game", len) &&	/* menu files */
			   !FS_IsExt(filename, ".dat", len) &&	/* for journal files */
			   !FS_IsDemoExt(filename, len))	/* demos */
			then{
				*file = 0;
				return -1;
			}

		dir = search->dir;

		netpath = FS_BuildOSPath(dir->path, dir->gamedir, filename);
		filep	= fopen(netpath, "rb");

		if(filep == NULL){
			*file = 0;
			return -1;
		}

		Q_strncpyz(fsh[*file].name, filename, sizeof(fsh[*file].name));
		fsh[*file].zipFile = qfalse;

		if(fs_debug->integer)
			Com_Printf("FS_FOpenFileRead: %s (found in '%s/%s')\n",
				filename,
				dir->path,
				dir->gamedir);

		fsh[*file].handleFiles.file.o = filep;
		return FS_fplength(filep);
	}

	return -1;
}

/*
 * Finds the file in the search path.
 * Returns filesize and an open FILE pointer.
 * Used for streaming data out of either a
 * separate file or a ZIP file.
 */
long
FS_FOpenFileRead(const char *filename, fileHandle_t *file, qbool uniqueFILE)
{
	searchpath_t *search;
	long len;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	for(search = fs_searchpaths; search; search = search->next){
		len = FS_FOpenFileReadDir(filename, search, file, uniqueFILE,
			qfalse);

		if(file == NULL){
			if(len > 0)
				return len;
		}else if(len >= 0 && *file)
			return len;

	}

#ifdef FS_MISSING
	if(missingFiles)
		fprintf(missingFiles, "%s\n", filename);
#endif

	if(file)
		*file = 0;

	return -1;
}

/*
 * Find a suitable VM file in search path order.
 *
 * In each searchpath try:
 * - open DLL file if DLL loading enabled
 * - open QVM file
 *
 * Enable search for DLL by setting enableDll to FSVM_ENABLEDLL
 *
 * write found DLL or QVM to "found" and return VMI_NATIVE if DLL, VMI_COMPILED if QVM
 * Return the searchpath in "startSearch".
 */
vmInterpret_t
FS_FindVM(void **startSearch, char *found, int foundlen, const char *name,
	  int enableDll)
{
	searchpath_t *search, *lastSearch;
	directory_t	*dir;
	pack_t		*pack;
	char dllName[MAX_OSPATH], qvmName[MAX_OSPATH];
	char		*netpath;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(enableDll)
		Q_sprintf(dllName, sizeof(dllName), "%s-" ARCH_STRING DLL_EXT,
			name);

	Q_sprintf(qvmName, sizeof(qvmName), Pvmfiles "/%s.qvm", name);

	lastSearch = *startSearch;
	if(*startSearch == NULL)
		search = fs_searchpaths;
	else
		search = lastSearch->next;

	while(search){
		if(search->dir && !fs_numServerPaks){
			dir = search->dir;

			if(enableDll){
				netpath = FS_BuildOSPath(dir->path, dir->gamedir,
					dllName);

				if(FS_FileInPathExists(netpath)){
					Q_strncpyz(found, netpath, foundlen);
					*startSearch = search;

					return VMI_NATIVE;
				}
			}

			if(FS_FOpenFileReadDir(qvmName, search, NULL, qfalse,
				   qfalse) > 0){
				*startSearch = search;
				return VMI_COMPILED;
			}
		}else if(search->pack){
			pack = search->pack;

			if(lastSearch && lastSearch->pack)
				/* make sure we only try loading one VM file per game dir
				 * i.e. if VM from pak7.pk3 fails we won't try one from pak6.pk3 */

				if(!FS_FilenameCompare(lastSearch->pack->
					   pakPathname, pack->pakPathname)){
					search = search->next;
					continue;
				}

			if(FS_FOpenFileReadDir(qvmName, search, NULL, qfalse,
				   qfalse) > 0){
				*startSearch = search;

				return VMI_COMPILED;
			}
		}

		search = search->next;
	}

	return -1;
}

/* Properly handles partial reads */
int
FS_Read2(void *buffer, int len, fileHandle_t f)
{
	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(!f)
		return 0;
	if(fsh[f].streamed){
		int r;
		fsh[f].streamed = qfalse;
		r = FS_Read(buffer, len, f);
		fsh[f].streamed = qtrue;
		return r;
	}else
		return FS_Read(buffer, len, f);
}

int
FS_Read(void *buffer, int len, fileHandle_t f)
{
	int	block, remaining;
	int	read;
	byte *buf;
	int	tries;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(!f)
		return 0;

	buf = (byte*)buffer;
	fs_readCount += len;

	if(fsh[f].zipFile == qfalse){
		remaining = len;
		tries = 0;
		while(remaining){
			block	= remaining;
			read	= fread (buf, 1, block,
				fsh[f].handleFiles.file.o);
			if(read == 0){
				/* 
				 * we might have been trying to read from a CD, which
				 * sometimes returns a 0 read on windows 
				 */
				if(!tries)
					tries = 1;
				else
					return len-remaining;	/* Com_Errorf (ERR_FATAL, "FS_Read: 0 bytes read"); */
			}

			if(read == -1)
				Com_Errorf (ERR_FATAL, "FS_Read: -1 bytes read");

			remaining -= read;
			buf += read;
		}
		return len;
	}else
		return unzReadCurrentFile(fsh[f].handleFiles.file.z, buffer, len);
}

/* Properly handles partial writes */
int
FS_Write(const void *buffer, int len, fileHandle_t h)
{
	int	block, remaining;
	int	written;
	byte *buf;
	int	tries;
	FILE *f;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(!h)
		return 0;

	f = FS_FileForHandle(h);
	buf = (byte*)buffer;

	remaining = len;
	tries = 0;
	while(remaining){
		block	= remaining;
		written = fwrite (buf, 1, block, f);
		if(written == 0){
			if(!tries)
				tries = 1;
			else{
				Com_Printf("FS_Write: 0 bytes written\n");
				return 0;
			}
		}

		if(written == -1){
			Com_Printf("FS_Write: -1 bytes written\n");
			return 0;
		}

		remaining -= written;
		buf += written;
	}
	if(fsh[h].handleSync)
		fflush(f);
	return len;
}

void QDECL
FS_Printf(fileHandle_t h, const char *fmt, ...)
{
	va_list argptr;
	char	msg[MAXPRINTMSG];

	va_start (argptr,fmt);
	Q_vsnprintf (msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	FS_Write(msg, strlen(msg), h);
}

#define PK3_SEEK_BUFFER_SIZE 65536

/* seek on a file (doesn't work for zip files! */
int
FS_Seek(fileHandle_t f, long offset, int origin)
{
	int _origin;

	if(!fs_searchpaths){
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");
		return -1;
	}

	if(fsh[f].streamed){
		fsh[f].streamed = qfalse;
		FS_Seek(f, offset, origin);
		fsh[f].streamed = qtrue;
	}

	if(fsh[f].zipFile == qtrue){
		/* FIXME: this is incomplete and really, really
		 * crappy (but better than what was here before) */
		byte	buffer[PK3_SEEK_BUFFER_SIZE];
		int	remainder = offset;

		if(offset < 0 || origin == FS_SEEK_END){
			Com_Errorf(
				ERR_FATAL,
				"Negative offsets and FS_SEEK_END not implemented "
				"for FS_Seek on pk3 file contents");
			return -1;
		}

		switch(origin){
		case FS_SEEK_SET:
			unzSetOffset(fsh[f].handleFiles.file.z,
				fsh[f].zipFilePos);
			unzOpenCurrentFile(fsh[f].handleFiles.file.z);
		/* fallthrough */

		case FS_SEEK_CUR:
			while(remainder > PK3_SEEK_BUFFER_SIZE){
				FS_Read(buffer, PK3_SEEK_BUFFER_SIZE, f);
				remainder -= PK3_SEEK_BUFFER_SIZE;
			}
			FS_Read(buffer, remainder, f);
			return offset;
			break;

		default:
			Com_Errorf(ERR_FATAL, "Bad origin in FS_Seek");
			return -1;
			break;
		}
	}else{
		FILE *file;
		file = FS_FileForHandle(f);
		switch(origin){
		case FS_SEEK_CUR:
			_origin = SEEK_CUR;
			break;
		case FS_SEEK_END:
			_origin = SEEK_END;
			break;
		case FS_SEEK_SET:
			_origin = SEEK_SET;
			break;
		default:
			_origin = SEEK_CUR;
			Com_Errorf(ERR_FATAL, "Bad origin in FS_Seek");
			break;
		}

		return fseek(file, offset, _origin);
	}
}

/*
 * Convenience functions for entire files
 */

int
FS_FileIsInPAK(const char *filename, int *pChecksum)
{
	searchpath_t *search;
	pack_t	*pak;
	fileInPack_t    *pakFile;
	long	hash = 0;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(!filename)
		Com_Errorf(ERR_FATAL,
			"FS_FOpenFileRead: NULL 'filename' parameter passed");

	/* qpaths are not supposed to have a leading slash */
	if(filename[0] == '/' || filename[0] == '\\')
		filename++;

	/* make absolutely sure that it can't back up the path.
	 * The searchpaths do guarantee that something will always
	 * be prepended, so we don't need to worry about "c:" or "//limbo" */
	if(strstr(filename, "..") || strstr(filename, "::"))
		return -1;

	/*
	 * search through the path, one element at a time
	 *  */

	for(search = fs_searchpaths; search; search = search->next){
		/*  */
		if(search->pack)
			hash = FS_HashFileName(filename, search->pack->hashSize);
		/* is the element a pak file? */
		if(search->pack && search->pack->hashTable[hash]){
			/* disregard if it doesn't match one of the allowed pure pak files */
			if(!FS_PakIsPure(search->pack))
				continue;

			/* look through all the pak file elements */
			pak = search->pack;
			pakFile = pak->hashTable[hash];
			do {
				/* case and separator insensitive comparisons */
				if(!FS_FilenameCompare(pakFile->name,
					   filename)){
					if(pChecksum)
						*pChecksum = pak->pure_checksum;
					return 1;
				}
				pakFile = pakFile->next;
			} while(pakFile != NULL);
		}
	}
	return -1;
}

/*
 * Filename are relative to the quake search path
 * a null buffer will just return the file length without loading
 * If searchPath is non-NULL search only in that specific search path
 *
 * returns the length of the file
 * a null buffer will just return the file length without loading
 * as a quick check for existance. -1 length == not present
 * A 0 byte will always be appended at the end, so string ops are safe.
 * the buffer should be considered read-only, because it may be cached
 * for other uses.
 */
long
FS_ReadFileDir(const char *qpath, void *searchPath, qbool unpure,
	       void **buffer)
{
	fileHandle_t	h;
	searchpath_t	*search;
	byte	* buf;
	qbool isConfig;
	long	len;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(!qpath || !qpath[0])
		Com_Errorf(ERR_FATAL, "FS_ReadFile with empty name");

	buf = NULL;	/* quiet compiler warning */

	/* if this is a .cfg file and we are playing back a journal, read
	 * it from the journal file */
	if(strstr(qpath, ".cfg")){
		isConfig = qtrue;
		if(com_journal && com_journal->integer == 2){
			int r;

			Com_DPrintf("Loading %s from journal file.\n", qpath);
			r = FS_Read(&len, sizeof(len), com_journalDataFile);
			if(r != sizeof(len)){
				if(buffer != NULL) *buffer = NULL;
				return -1;
			}
			/* if the file didn't exist when the journal was created */
			if(!len){
				if(buffer == NULL)
					return 1;	/* hack for old journal files */
				*buffer = NULL;
				return -1;
			}
			if(buffer == NULL)
				return len;

			buf = Hunk_AllocateTempMemory(len+1);
			*buffer = buf;

			r = FS_Read(buf, len, com_journalDataFile);
			if(r != len)
				Com_Errorf(ERR_FATAL,
					"Read from journalDataFile failed");

			fs_loadCount++;
			fs_loadStack++;

			/* guarantee that it will have a trailing 0 for string operations */
			buf[len] = 0;

			return len;
		}
	}else
		isConfig = qfalse;

	search = searchPath;

	if(search == NULL)
		/* look for it in the filesystem or pack files */
		len = FS_FOpenFileRead(qpath, &h, qfalse);
	else
		/* look for it in a specific search path only */
		len = FS_FOpenFileReadDir(qpath, search, &h, qfalse, unpure);

	if(h == 0){
		if(buffer)
			*buffer = NULL;
		/* if we are journalling and it is a config file, write a zero to the journal file */
		if(isConfig && com_journal && com_journal->integer == 1){
			Com_DPrintf("Writing zero for %s to journal file.\n",
				qpath);
			len = 0;
			FS_Write(&len, sizeof(len), com_journalDataFile);
			FS_Flush(com_journalDataFile);
		}
		return -1;
	}

	if(!buffer){
		if(isConfig && com_journal && com_journal->integer == 1){
			Com_DPrintf("Writing len for %s to journal file.\n",
				qpath);
			FS_Write(&len, sizeof(len), com_journalDataFile);
			FS_Flush(com_journalDataFile);
		}
		FS_FCloseFile(h);
		return len;
	}

	fs_loadCount++;
	fs_loadStack++;

	buf = Hunk_AllocateTempMemory(len+1);
	*buffer = buf;

	FS_Read (buf, len, h);

	/* guarantee that it will have a trailing 0 for string operations */
	buf[len] = 0;
	FS_FCloseFile(h);

	/* if we are journalling and it is a config file, write it to the journal file */
	if(isConfig && com_journal && com_journal->integer == 1){
		Com_DPrintf("Writing %s to journal file.\n", qpath);
		FS_Write(&len, sizeof(len), com_journalDataFile);
		FS_Write(buf, len, com_journalDataFile);
		FS_Flush(com_journalDataFile);
	}
	return len;
}

/*
 * Filename are relative to the quake search path
 * a null buffer will just return the file length without loading
 */
long
FS_ReadFile(const char *qpath, void **buffer)
{
	return FS_ReadFileDir(qpath, NULL, qfalse, buffer);
}

/* frees the memory returned by FS_ReadFile */
void
FS_FreeFile(void *buffer)
{
	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");
	if(!buffer)
		Com_Errorf(ERR_FATAL, "FS_FreeFile( NULL )");
	fs_loadStack--;

	Hunk_FreeTempMemory(buffer);

	/* if all of our temp files are free, clear all of our space */
	if(fs_loadStack == 0)
		Hunk_ClearTempMemory();
}

/* writes a complete file, creating any subdirectories needed */
/* Filenames are relative to the quake search path */
void
FS_WriteFile(const char *qpath, const void *buffer, int size)
{
	fileHandle_t f;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(!qpath || !buffer)
		Com_Errorf(ERR_FATAL, "FS_WriteFile: NULL parameter");

	f = FS_FOpenFileWrite(qpath);
	if(!f){
		Com_Printf("Failed to open %s\n", qpath);
		return;
	}

	FS_Write(buffer, size, f);

	FS_FCloseFile(f);
}


/*
 * pk3 file loading
 */

/*
 * Creates a new pak_t in the search chain for the contents
 * of a zip file.
 */
static pack_t *
FS_LoadZipFile(const char *zipfile, const char *basename)
{
	fileInPack_t *buildBuffer;
	pack_t	*pack;
	unzFile uf;
	int	err;
	unz_global_info gi;
	char	filename_inzip[MAX_ZPATH];
	unz_file_info file_info;
	int	i, len;
	long	hash;
	int	fs_numHeaderLongs;
	int *fs_headerLongs;
	char *namePtr;

	fs_numHeaderLongs = 0;

	uf = unzOpen(zipfile);
	err = unzGetGlobalInfo (uf,&gi);

	if(err != UNZ_OK)
		return NULL;

	len = 0;
	unzGoToFirstFile(uf);
	for(i = 0; i < gi.number_entry; i++){
		err = unzGetCurrentFileInfo(uf, &file_info, filename_inzip,
				sizeof(filename_inzip), NULL, 0, NULL,
				0);
		if(err != UNZ_OK)
			break;
		len += strlen(filename_inzip) + 1;
		unzGoToNextFile(uf);
	}

	buildBuffer = Z_Malloc((gi.number_entry * sizeof(fileInPack_t)) + len);
	namePtr = ((char*)buildBuffer) + gi.number_entry * sizeof(fileInPack_t);
	fs_headerLongs = Z_Malloc((gi.number_entry + 1) * sizeof(int));
	fs_headerLongs[ fs_numHeaderLongs++ ] = LittleLong(fs_checksumFeed);

	/* 
	 * get the hash table size from the number of files in the zip
	 * because lots of custom pk3 files have less than 32 or 64 files 
	 */
	for(i = 1; i <= MAX_FILEHASH_SIZE; i <<= 1)
		if(i > gi.number_entry)
			break;

	pack = Z_Malloc(sizeof(pack_t) + i * sizeof(fileInPack_t *));
	pack->hashSize	= i;
	pack->hashTable = (fileInPack_t**)(((char*)pack) + sizeof(pack_t));
	for(i = 0; i < pack->hashSize; i++)
		pack->hashTable[i] = NULL;

	Q_strncpyz(pack->pakFilename, zipfile, sizeof(pack->pakFilename));
	Q_strncpyz(pack->pakBasename, basename, sizeof(pack->pakBasename));

	/* strip .pk3 if needed */
	if(strlen(pack->pakBasename) > 4 &&
	   !Q_stricmp(pack->pakBasename + strlen(pack->pakBasename) - 4, ".pk3"))
		pack->pakBasename[strlen(pack->pakBasename) - 4] = 0;

	pack->handle = uf;
	pack->numfiles = gi.number_entry;
	unzGoToFirstFile(uf);

	for(i = 0; i < gi.number_entry; i++){
		err =
			unzGetCurrentFileInfo(uf, &file_info, filename_inzip,
				sizeof(filename_inzip), NULL, 0, NULL,
				0);
		if(err != UNZ_OK)
			break;
		if(file_info.uncompressed_size > 0)
			fs_headerLongs[fs_numHeaderLongs++] = LittleLong(
				file_info.crc);
		Q_strlwr(filename_inzip);
		hash = FS_HashFileName(filename_inzip, pack->hashSize);
		buildBuffer[i].name = namePtr;
		strcpy(buildBuffer[i].name, filename_inzip);
		namePtr += strlen(filename_inzip) + 1;
		/* store the file position in the zip */
		buildBuffer[i].pos = unzGetOffset(uf);
		buildBuffer[i].len = file_info.uncompressed_size;
		buildBuffer[i].next = pack->hashTable[hash];
		pack->hashTable[hash] = &buildBuffer[i];
		unzGoToNextFile(uf);
	}

	pack->checksum = Q_BlockChecksum(
			&fs_headerLongs[1], sizeof(*fs_headerLongs) *
			(fs_numHeaderLongs - 1));
	pack->pure_checksum =
		Q_BlockChecksum(fs_headerLongs,
			sizeof(*fs_headerLongs) * fs_numHeaderLongs);
	pack->checksum = LittleLong(pack->checksum);
	pack->pure_checksum = LittleLong(pack->pure_checksum);

	Z_Free(fs_headerLongs);

	pack->buildBuffer = buildBuffer;
	return pack;
}

/* Frees a pak structure and releases all associated resources */
static void
FS_FreePak(pack_t *thepak)
{
	unzClose(thepak->handle);
	Z_Free(thepak->buildBuffer);
	Z_Free(thepak);
}

/* Compares whether the given pak file matches a referenced checksum */
qbool
FS_CompareZipChecksum(const char *zipfile)
{
	pack_t	*thepak;
	int	index, checksum;

	thepak = FS_LoadZipFile(zipfile, "");

	if(!thepak)
		return qfalse;

	checksum = thepak->checksum;
	FS_FreePak(thepak);

	for(index = 0; index < fs_numServerReferencedPaks; index++)
		if(checksum == fs_serverReferencedPaks[index])
			return qtrue;

	return qfalse;
}


/*
 * Directory scanning routines
 */

#define MAX_FOUND_FILES 0x1000

static int
FS_ReturnPath(const char *zname, char *zpath, int *depth)
{
	int len, at, newdep;

	newdep = 0;
	zpath[0] = 0;
	len = 0;
	at = 0;

	while(zname[at] != 0){
		if(zname[at]=='/' || zname[at]=='\\'){
			len = at;
			newdep++;
		}
		at++;
	}
	strcpy(zpath, zname);
	zpath[len] = 0;
	*depth = newdep;
	return len;
}

static int
FS_AddFileToList(char *name, char *list[MAX_FOUND_FILES], int nfiles)
{
	int i;

	if(nfiles == MAX_FOUND_FILES - 1)
		return nfiles;
	for(i = 0; i < nfiles; i++)
		if(!Q_stricmp(name, list[i]))
			return nfiles;	/* allready in list */
	list[nfiles] = Copystr(name);
	nfiles++;

	return nfiles;
}

/*
 * Returns a uniqued list of files that match the given criteria
 * from all search paths
 */
char**
FS_ListFilteredFiles(const char *path, const char *extension, char *filter,
		     int *numfiles,
		     qbool allowNonPureFilesOnDisk)
{
	int nfiles;
	char **listCopy;
	char	*list[MAX_FOUND_FILES];
	searchpath_t *search;
	int	i;
	int	pathLength;
	int	extensionLength;
	int	length, pathDepth, temp;
	pack_t	*pak;
	fileInPack_t    *buildBuffer;
	char	zpath[MAX_ZPATH];

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");

	if(!path){
		*numfiles = 0;
		return NULL;
	}
	if(!extension)
		extension = "";

	pathLength = strlen(path);
	if(path[pathLength-1] == '\\' || path[pathLength-1] == '/')
		pathLength--;
	extensionLength = strlen(extension);
	nfiles = 0;
	FS_ReturnPath(path, zpath, &pathDepth);

	/*
	 * search through the path, one element at a time, adding to list
	 */
	for(search = fs_searchpaths; search; search = search->next){
		/* is the element a pak file? */
		if(search->pack){
			if(!FS_PakIsPure(search->pack)){
				/* 
				 * If we are pure, don't search for files on paks that
				 * aren't on the pure list 
				 */
				continue;
			}

			/* look through all the pak file elements */
			pak = search->pack;
			buildBuffer = pak->buildBuffer;
			for(i = 0; i < pak->numfiles; i++){
				char *name;
				int zpathLen, depth;

				/* check for directory match */
				name = buildBuffer[i].name;
				if(filter){
					/* case insensitive */
					if(!Q_FilterPath(filter, name, qfalse))
						continue;
					/* unique the match */
					nfiles = FS_AddFileToList(name, list,
						nfiles);
				}else{

					zpathLen = FS_ReturnPath(name, zpath,
						&depth);

					if((depth-pathDepth)>2 || pathLength >
					   zpathLen ||
					   Q_stricmpn(name, path, pathLength))
						continue;

					/* check for extension match */
					length = strlen(name);
					if(length < extensionLength)
						continue;

					if(Q_stricmp(name + length -
						   extensionLength, extension))
						continue;
					/* unique the match */

					temp = pathLength;
					if(pathLength)
						temp++;		/* include the '/' */
					nfiles = FS_AddFileToList(name + temp,
							list,
							nfiles);
				}
			}
		}else if(search->dir){	/* scan for files in the filesystem */
			char	*netpath;
			int	numSysFiles;
			char **sysFiles;
			char	*name;

			/* don't scan directories for files if we are pure or restricted */
			if(fs_numServerPaks && !allowNonPureFilesOnDisk)
				continue;
			else{
				netpath = FS_BuildOSPath(
					search->dir->path, search->dir->gamedir,
					path);
				sysFiles =
					Sys_ListFiles(netpath, extension, filter,
						&numSysFiles,
						qfalse);
				for(i = 0; i < numSysFiles; i++){
					/* unique the match */
					name = sysFiles[i];
					nfiles = FS_AddFileToList(name, list,
						nfiles);
				}
				Sys_FreeFileList(sysFiles);
			}
		}
	}

	/* return a copy of the list */
	*numfiles = nfiles;

	if(!nfiles)
		return NULL;

	listCopy = Z_Malloc((nfiles + 1) * sizeof(*listCopy));
	for(i = 0; i < nfiles; i++)
		listCopy[i] = list[i];
	listCopy[i] = NULL;

	return listCopy;
}

/* 
 * directory should not have either a leading or trailing /
 * if extension is "/", only subdirectories will be returned
 * the returned files will not include any directories or / 
 */
char**
FS_ListFiles(const char *path, const char *extension, int *numfiles)
{
	return FS_ListFilteredFiles(path, extension, NULL, numfiles, qfalse);
}

void
FS_FreeFileList(char **list)
{
	int i;

	if(!fs_searchpaths)
		Com_Errorf(ERR_FATAL,
			"Filesystem call made without initialization");
	if(!list)
		return;
	for(i = 0; list[i]; i++)
		Z_Free(list[i]);
	Z_Free(list);
}

int
FS_GetFileList(const char *path, const char *extension, char *listbuf,
	       int bufsize)
{
	int nFiles, i, nTotal, nLen;
	char **pFiles = NULL;

	*listbuf = 0;
	nFiles = 0;
	nTotal = 0;

	if(Q_stricmp(path, "$modlist") == 0)
		return FS_GetModList(listbuf, bufsize);

	pFiles = FS_ListFiles(path, extension, &nFiles);

	for(i =0; i < nFiles; i++){
		nLen = strlen(pFiles[i]) + 1;
		if(nTotal + nLen + 1 < bufsize){
			strcpy(listbuf, pFiles[i]);
			listbuf += nLen;
			nTotal += nLen;
		}else{
			nFiles = i;
			break;
		}
	}
	FS_FreeFileList(pFiles);
	return nFiles;
}

/*
 * Naive implementation. Concatenates three lists into a
 * new list, and frees the old lists from the heap.
 *
 * FIXME TTimo those two should move to common.c next to Sys_ListFiles
 */
static unsigned int
Sys_CountFileList(char **list)
{
	int i = 0;

	if(list)
		while(*list){
			list++;
			i++;
		}
	return i;
}

static char**
Sys_ConcatenateFileLists(char **list0, char **list1)
{
	int totalLength = 0;
	char ** cat = NULL, **dst, **src;

	totalLength += Sys_CountFileList(list0);
	totalLength += Sys_CountFileList(list1);

	/* Create new list. */
	dst = cat = Z_Malloc((totalLength + 1) * sizeof(char*));

	/* Copy over lists. */
	if(list0)
		for(src = list0; *src; src++, dst++)
			*dst = *src;
	if(list1)
		for(src = list1; *src; src++, dst++)
			*dst = *src;

	/* Terminate the list */
	*dst = NULL;

	/* Free our old lists.
	 * NOTE: not freeing their content, it's been merged in dst and still being used */
	if(list0) Z_Free(list0);
	if(list1) Z_Free(list1);

	return cat;
}

/*
 * Returns a list of mod directory names
 * A mod directory is a peer to baseq3 with a pk3 in it
 * The directories are searched in base path, cd path and home path
 */
int
FS_GetModList(char *listbuf, int bufsize)
{
	int nMods, i, j, nTotal, nLen, nPaks, nPotential, nDescLen;
	char **pFiles = NULL;
	char	**pPaks = NULL;
	char    *name, *path;
	char	descPath[MAX_OSPATH];
	fileHandle_t descHandle;

	int dummy;
	char **pFiles0	= NULL;
	char **pFiles1	= NULL;
	qbool bDrop	= qfalse;

	*listbuf = 0;
	nMods = nTotal = 0;

	pFiles0 = Sys_ListFiles(fs_homepath->string, NULL, NULL, &dummy, qtrue);
	pFiles1 = Sys_ListFiles(fs_basepath->string, NULL, NULL, &dummy, qtrue);
	/* we searched for mods in the three paths
	 * it is likely that we have duplicate names now, which we will cleanup below */
	pFiles = Sys_ConcatenateFileLists(pFiles0, pFiles1);
	nPotential = Sys_CountFileList(pFiles);

	for(i = 0; i < nPotential; i++){
		name = pFiles[i];
		/* NOTE: cleaner would involve more changes
		 * ignore duplicate mod directories */
		if(i!=0){
			bDrop = qfalse;
			for(j=0; j<i; j++)
				if(Q_stricmp(pFiles[j],name)==0){
					/* this one can be dropped */
					bDrop = qtrue;
					break;
				}
		}
		if(bDrop)
			continue;
		/* we drop "baseq3" "." and ".." */
		if(Q_stricmp(name,
			   com_basegame->string) && Q_stricmpn(name, ".", 1)){
			/* now we need to find some .pk3 files to validate the mod
			 * NOTE TTimo: (actually I'm not sure why .. what if it's a mod under developement with no .pk3?)
			 * we didn't keep the information when we merged the directory names, as to what OS Path it was found under
			 *   so it could be in base path, cd path or home path
			 *   we will try each three of them here (yes, it's a bit messy) */
			path = FS_BuildOSPath(fs_basepath->string, name, "");
			nPaks	= 0;
			pPaks	= Sys_ListFiles(path, ".pk3", NULL, &nPaks,
				qfalse);
			Sys_FreeFileList(pPaks);	/* we only use Sys_ListFiles to check wether .pk3 files are present */

			/* try on home path */
			if(nPaks <= 0){
				path =
					FS_BuildOSPath(fs_homepath->string, name,
						"");
				nPaks	= 0;
				pPaks	=
					Sys_ListFiles(path, ".pk3", NULL, &nPaks,
						qfalse);
				Sys_FreeFileList(pPaks);
			}

			if(nPaks > 0){
				nLen = strlen(name) + 1;
				/* nLen is the length of the mod path
				 * we need to see if there is a description available */
				descPath[0] = '\0';
				strcpy(descPath, name);
				strcat(descPath, "/description.txt");
				nDescLen = FS_SV_FOpenFileRead(descPath,
					&descHandle);
				if(nDescLen > 0 && descHandle){
					FILE *file;
					file = FS_FileForHandle(descHandle);
					Q_Memset(descPath, 0, sizeof(descPath));
					nDescLen = fread(descPath, 1, 48, file);
					if(nDescLen >= 0)
						descPath[nDescLen] = '\0';
					FS_FCloseFile(descHandle);
				}else
					strcpy(descPath, name);
				nDescLen = strlen(descPath) + 1;

				if(nTotal + nLen + 1 + nDescLen + 1 < bufsize){
					strcpy(listbuf, name);
					listbuf += nLen;
					strcpy(listbuf, descPath);
					listbuf += nDescLen;
					nTotal	+= nLen + nDescLen;
					nMods++;
				}else
					break;
			}
		}
	}
	Sys_FreeFileList(pFiles);

	return nMods;
}

void
FS_Dir_f(void)
{
	char *path;
	char	*extension;
	char **dirnames;
	int	ndirs;
	int	i;

	if(Cmd_Argc() < 2 || Cmd_Argc() > 3){
		Com_Printf("usage: dir <directory> [extension]\n");
		return;
	}

	if(Cmd_Argc() == 2){
		path = Cmd_Argv(1);
		extension = "";
	}else{
		path = Cmd_Argv(1);
		extension = Cmd_Argv(2);
	}

	Com_Printf("Directory of %s %s\n", path, extension);
	Com_Printf("---------------\n");

	dirnames = FS_ListFiles(path, extension, &ndirs);

	for(i = 0; i < ndirs; i++)
		Com_Printf("%s\n", dirnames[i]);
	FS_FreeFileList(dirnames);
}

void
FS_ConvertPath(char *s)
{
	while(*s){
		if(*s == '\\' || *s == ':')
			*s = '/';
		s++;
	}
}

/* Ignore case and seprator char distinctions */
int
FS_PathCmp(const char *s1, const char *s2)
{
	int c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if(c1 >= 'a' && c1 <= 'z')
			c1 -= ('a' - 'A');
		if(c2 >= 'a' && c2 <= 'z')
			c2 -= ('a' - 'A');

		if(c1 == '\\' || c1 == ':')
			c1 = '/';
		if(c2 == '\\' || c2 == ':')
			c2 = '/';

		if(c1 < c2)
			return -1;	/* strings not equal */
		if(c1 > c2)
			return 1;
	} while(c1);

	return 0;	/* strings are equal */
}

void
FS_SortFileList(char **filelist, int numfiles)
{
	int i, j, k, numsortedfiles;
	char **sortedlist;

	sortedlist = Z_Malloc((numfiles + 1) * sizeof(*sortedlist));
	sortedlist[0]	= NULL;
	numsortedfiles	= 0;
	for(i = 0; i < numfiles; i++){
		for(j = 0; j < numsortedfiles; j++)
			if(FS_PathCmp(filelist[i], sortedlist[j]) < 0)
				break;
		for(k = numsortedfiles; k > j; k--)
			sortedlist[k] = sortedlist[k-1];
		sortedlist[j] = filelist[i];
		numsortedfiles++;
	}
	Q_Memcpy(filelist, sortedlist, numfiles * sizeof(*filelist));
	Z_Free(sortedlist);
}

void
FS_NewDir_f(void)
{
	char	*filter;
	char **dirnames;
	int	ndirs;
	int	i;

	if(Cmd_Argc() < 2){
		Com_Printf("usage: fdir <filter>\n");
		Com_Printf("example: fdir *q3dm*.bsp\n");
		return;
	}

	filter = Cmd_Argv(1);

	Com_Printf("---------------\n");

	dirnames = FS_ListFilteredFiles("", "", filter, &ndirs, qfalse);

	FS_SortFileList(dirnames, ndirs);

	for(i = 0; i < ndirs; i++){
		FS_ConvertPath(dirnames[i]);
		Com_Printf("%s\n", dirnames[i]);
	}
	Com_Printf("%d files listed\n", ndirs);
	FS_FreeFileList(dirnames);
}

void
FS_Path_f(void)
{
	searchpath_t *s;
	int i;

	Com_Printf ("Current search path:\n");
	for(s = fs_searchpaths; s; s = s->next){
		if(s->pack){
			Com_Printf ("%s (%i files)\n", s->pack->pakFilename,
				s->pack->numfiles);
			if(fs_numServerPaks){
				if(!FS_PakIsPure(s->pack))
					Com_Printf("    not on the pure list\n");
				else
					Com_Printf("    on the pure list\n");
			}
		}else
			Com_Printf ("%s%c%s\n", s->dir->path, PATH_SEP,
				s->dir->gamedir);
	}


	Com_Printf("\n");
	for(i = 1; i < MAX_FILE_HANDLES; i++)
		if(fsh[i].handleFiles.file.o)
			Com_Printf("handle %i: %s\n", i, fsh[i].name);
}

void
FS_TouchFile_f(void)
{
	fileHandle_t f;

	if(Cmd_Argc() != 2){
		Com_Printf("Usage: touchFile <file>\n");
		return;
	}

	FS_FOpenFileRead(Cmd_Argv(1), &f, qfalse);
	if(f)
		FS_FCloseFile(f);
}

qbool
FS_Which(const char *filename, void *searchPath)
{
	searchpath_t *search = searchPath;

	if(FS_FOpenFileReadDir(filename, search, NULL, qfalse, qfalse) > 0){
		if(search->pack){
			Com_Printf("File \"%s\" found in \"%s\"\n", filename,
				search->pack->pakFilename);
			return qtrue;
		}else if(search->dir){
			Com_Printf("File \"%s\" found at \"%s\"\n", filename,
				search->dir->fullpath);
			return qtrue;
		}
	}

	return qfalse;
}

void
FS_Which_f(void)
{
	searchpath_t *search;
	char *filename;

	filename = Cmd_Argv(1);

	if(!filename[0]){
		Com_Printf("Usage: which <file>\n");
		return;
	}

	/* qpaths are not supposed to have a leading slash */
	if(filename[0] == '/' || filename[0] == '\\')
		filename++;

	/* just wants to see if file is there */
	for(search = fs_searchpaths; search; search = search->next)
		if(FS_Which(filename, search))
			return;

	Com_Printf("File not found: \"%s\"\n", filename);
	return;
}


static int QDECL
paksort(const void *a, const void *b)
{
	char *aa, *bb;

	aa = *(char**)a;
	bb = *(char**)b;

	return FS_PathCmp(aa, bb);
}

/*
 * Sets fs_gamedir, adds the directory to the head of the
 * path, then loads the zip headers
 */
void
FS_AddGameDirectory(const char *path, const char *dir)
{
	int	i;	/* index into pakfiles */
	int	j;	/* index into pakdirs */
	int	nfiles, ndirs;
	char	**pakfiles, **pakdirs;
	char	*pakfile, curpath[MAX_OSPATH + 1];
	enum {Pakfile, Pakdir} paktype;
	pack_t *pak;
	searchpath_t *sp;

	for(sp = fs_searchpaths; sp != NULL; sp = sp->next)
		if(sp->dir && Q_stricmp(sp->dir->gamedir, dir)
		   && Q_stricmp(sp->dir->path, path)
		   ) return;	/* ignore; we've already got this one */

	Q_strncpyz(fs_gamedir, dir, sizeof(fs_gamedir));
	/*
	 * find all pakfiles in this directory, and get _all_ dirs to later
	 * filter down to pakdirs
	 */
	Q_strncpyz(curpath, FS_BuildOSPath(path, dir, ""), sizeof(curpath));
	curpath[strlen(curpath) - 1] = '\0';	/* strip trailing slash */
	pakfiles = Sys_ListFiles(curpath, ".pk3", NULL, &nfiles, qfalse);
	pakdirs = Sys_ListFiles(curpath, "/", NULL, &ndirs, qfalse);
	qsort(pakfiles, nfiles, sizeof(char *), paksort);
	qsort(pakdirs, ndirs, sizeof(char *), paksort);

	for(i = 0, j = 0; ((i + j) < (nfiles + ndirs)); ){
		if(i >= nfiles)
			paktype = Pakdir;	/* out of files; must be a pakdir */
		else if(j >= ndirs)
			paktype = Pakfile;	/* vice versa */
		else{
			/* could be either; let's see... */
			if(paksort(&pakfiles[i], &pakdirs[j]) < 0)
				paktype = Pakdir;
			else
				paktype = Pakfile;
		}

		/* add pakfile or add pakdir */
		if(paktype == Pakfile){
			pakfile = FS_BuildOSPath(path, dir, pakfiles[i]);
			if((pak = FS_LoadZipFile(pakfile, pakfiles[i])) == 0)
				continue;
			Q_strncpyz(pak->pakPathname, curpath,
				sizeof(pak->pakPathname));
			/* store the game name for downloading */
			Q_strncpyz(pak->pakGamename, dir,
				sizeof(pak->pakGamename));

			fs_packFiles += pak->numfiles;

			sp = Z_Malloc(sizeof(*sp));
			sp->pack = pak;
			sp->next = fs_searchpaths;
			fs_searchpaths = sp;

			++i;
		}else{
			int len;
			directory_t *p;

			len = strlen(pakdirs[j]);
			/* filter out anything that doesn't end with .dir3 */
			if(!FS_IsExt(pakdirs[j], ".dir3", len)){
				++j;
				continue;
			}
			pakfile = FS_BuildOSPath(path, dir, pakdirs[j]);
			/* FIXME: increase fs_packFiles */
			/* add to the search path */
			sp = Z_Malloc(sizeof(*sp));
			sp->dir = Z_Malloc(sizeof(*sp->dir));
			p = sp->dir;
			Q_strncpyz(p->path, curpath, sizeof(p->path));
			Q_strncpyz(p->fullpath, pakfile, sizeof(p->fullpath));
			Q_strncpyz(p->gamedir, pakdirs[j], sizeof(p->gamedir));

			++j;
		}
	}
	Sys_FreeFileList(pakfiles);
	Sys_FreeFileList(pakdirs);

	/* add the directory to the search path */
	sp = Z_Malloc(sizeof(*sp));
	sp->dir = Z_Malloc(sizeof(*sp->dir));
	Q_strncpyz(sp->dir->path, path, sizeof(sp->dir->path));
	Q_strncpyz(sp->dir->fullpath, curpath, sizeof(sp->dir->fullpath));
	Q_strncpyz(sp->dir->gamedir, dir, sizeof(sp->dir->gamedir));
	sp->next = fs_searchpaths;
	fs_searchpaths = sp;
}

qbool
FS_idPak(char *pak, char *base, int numPaks)
{
	int i;

	for(i = 0; i < NUM_ID_PAKS; i++)
		if(!FS_FilenameCompare(pak, va("%s/pak%d", base, i)))
			break;
	if(i < numPaks)
		return qtrue;
	return qfalse;
}

/*
 * Check whether the string contains stuff like "../" to prevent directory traversal bugs
 * and return qtrue if it does.
 */

qbool
FS_CheckDirTraversal(const char *checkdir)
{
	if(strstr(checkdir, "../") || strstr(checkdir, "..\\"))
		return qtrue;

	return qfalse;
}

/*
 * ----------------
 * dlstring == qtrue
 *
 * Returns a list of pak files that we should download from the server. They all get stored
 * in the current gamedir and an FS_Restart will be fired up after we download them all.
 *
 * The string is the format:
 *
 * @remotename@localname [repeat]
 *
 * static int		fs_numServerReferencedPaks;
 * static int		fs_serverReferencedPaks[MAX_SEARCH_PATHS];
 * static char		*fs_serverReferencedPakNames[MAX_SEARCH_PATHS];
 *
 * ----------------
 * dlstring == qfalse
 *
 * we are not interested in a download string format, we want something human-readable
 * (this is used for diagnostics while connecting to a pure server)
 *
 */
qbool
FS_ComparePaks(char *neededpaks, int len, qbool dlstring)
{
	searchpath_t *sp;
	qbool havepak;
	char *origpos = neededpaks;
	int i;

	if(!fs_numServerReferencedPaks)
		return qfalse;	/* Server didn't send any pack information along */

	*neededpaks = 0;

	for(i = 0; i < fs_numServerReferencedPaks; i++){
		/* Ok, see if we have this pak file */
		havepak = qfalse;

		/* never autodownload any of the id paks */
		if(FS_idPak(fs_serverReferencedPakNames[i], BASEGAME,
			   NUM_ID_PAKS)
#ifndef STANDALONE
		   || FS_idPak(fs_serverReferencedPakNames[i], BASETA,
			   NUM_TA_PAKS)
#endif
		   )
			continue;

		/* Make sure the server cannot make us write to non-quake3 directories. */
		if(FS_CheckDirTraversal(fs_serverReferencedPakNames[i])){
			Com_Printf("WARNING: Invalid download name %s\n",
				fs_serverReferencedPakNames[i]);
			continue;
		}

		for(sp = fs_searchpaths; sp; sp = sp->next)
			if(sp->pack && sp->pack->checksum ==
			   fs_serverReferencedPaks[i]){
				havepak = qtrue;	/* This is it! */
				break;
			}

		if(!havepak && fs_serverReferencedPakNames[i] &&
		   *fs_serverReferencedPakNames[i]){
			/* Don't got it */

			if(dlstring){
				/* We need this to make sure we won't hit the end of the buffer or the server could
				 * overwrite non-pk3 files on clients by writing so much crap into neededpaks that
				 * Q_strcat cuts off the .pk3 extension. */

				origpos += strlen(origpos);

				/* Remote name */
				Q_strcat(neededpaks, len, "@");
				Q_strcat(neededpaks, len,
					fs_serverReferencedPakNames[i]);
				Q_strcat(neededpaks, len, ".pk3");

				/* Local name */
				Q_strcat(neededpaks, len, "@");
				/* Do we have one with the same name? */
				if(FS_SV_FileExists(va("%s.pk3",
						   fs_serverReferencedPakNames[
							   i]))){
					char st[MAX_ZPATH];
					/* We already have one called this, we need to download it to another name
					 * Make something up with the checksum in it */
					Q_sprintf(
						st, sizeof(st), "%s.%08x.pk3",
						fs_serverReferencedPakNames[i],
						fs_serverReferencedPaks[i]);
					Q_strcat(neededpaks, len, st);
				}else{
					Q_strcat(neededpaks, len,
						fs_serverReferencedPakNames[i]);
					Q_strcat(neededpaks, len, ".pk3");
				}

				/* Find out whether it might have overflowed the buffer and don't add this file to the
				 * list if that is the case. */
				if(strlen(origpos) + (origpos - neededpaks) >=
				   len - 1){
					*origpos = '\0';
					break;
				}
			}else{
				Q_strcat(neededpaks, len,
					fs_serverReferencedPakNames[i]);
				Q_strcat(neededpaks, len, ".pk3");
				/* Do we have one with the same name? */
				if(FS_SV_FileExists(va("%s.pk3",
						   fs_serverReferencedPakNames[
							   i])))
					Q_strcat(
						neededpaks, len,
						" (local file exists with wrong checksum)");
				Q_strcat(neededpaks, len, "\n");
			}
		}
	}
	if(*neededpaks)
		return qtrue;
	return qfalse;	/* We have them all */
}

/* frees all resources */
void
FS_Shutdown(qbool closemfp)
{
	searchpath_t *p, *next;
	int i;

	for(i = 0; i < MAX_FILE_HANDLES; i++)
		if(fsh[i].fileSize)
			FS_FCloseFile(i);

	/* free everything */
	for(p = fs_searchpaths; p; p = next){
		next = p->next;

		if(p->pack)
			FS_FreePak(p->pack);
		if(p->dir)
			Z_Free(p->dir);

		Z_Free(p);
	}

	/* any FS_ calls will now be an error until reinitialized */
	fs_searchpaths = NULL;

	Cmd_RemoveCommand("path");
	Cmd_RemoveCommand("dir");
	Cmd_RemoveCommand("fdir");
	Cmd_RemoveCommand("touchFile");
	Cmd_RemoveCommand("which");

#ifdef FS_MISSING
	if(closemfp)
		fclose(missingFiles);

#endif
}

/*
 * NOTE TTimo: the reordering that happens here is not reflected in the cvars (\cvarlist *pak*)
 * this can lead to misleading situations, see https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=540
 */
static void
FS_ReorderPurePaks(void)
{
	searchpath_t *s;
	int i;
	searchpath_t **p_insert_index,	/* for linked list reordering */
	**p_previous;			/* when doing the scan */

	fs_reordered = qfalse;

	/* only relevant when connected to pure server */
	if(!fs_numServerPaks)
		return;

	p_insert_index = &fs_searchpaths;	/* we insert in order at the beginning of the list */
	for(i = 0; i < fs_numServerPaks; i++){
		p_previous = p_insert_index;	/* track the pointer-to-current-item */
		for(s = *p_insert_index; s; s = s->next){
			/* the part of the list before p_insert_index has been sorted already */
			if(s->pack && fs_serverPaks[i] == s->pack->checksum){
				fs_reordered = qtrue;
				/* move this element to the insert list */
				*p_previous = s->next;
				s->next = *p_insert_index;
				*p_insert_index = s;
				/* increment insert list */
				p_insert_index = &s->next;
				break;	/* iterate to next server pack */
			}
			p_previous = &s->next;
		}
	}
}

static void
FS_Startup(const char *gameName)
{
	const char *homePath;

	Com_Printf("----- FS_Startup -----\n");
	fs_packFiles = 0;
	fs_debug = Cvar_Get("fs_debug", "0", 0);
	fs_basepath = Cvar_Get("fs_basepath",
		Sys_DefaultInstallPath(), CVAR_INIT|CVAR_PROTECTED);
	fs_basegame = Cvar_Get("fs_basegame", "", CVAR_INIT);
#ifdef _WIN32
	/*
	 * Don't use home on windows. It would be cleaner to allow the user to
	 * decide whether to use the home dir via an archived cvar such as
	 * "fs_usehome"; but, of course, the filesystem isn't initialized at this
	 * point, so we cannot get cvars from the config.
	 */
	homePath = fs_basepath->string;
#else
	homePath = Sys_DefaultHomePath();
	if(!homePath || !homePath[0])
		homePath = fs_basepath->string;		/* fallback */

#endif
	fs_homepath = Cvar_Get("fs_homepath", homePath,
		CVAR_INIT|CVAR_PROTECTED);
	fs_gamedirvar = Cvar_Get("fs_game", "", CVAR_INIT|CVAR_SYSTEMINFO);

	/* add search path elements in reverse priority order */
	if(fs_basepath->string[0])
		FS_AddGameDirectory(fs_basepath->string, gameName);
	/* fs_homepath is somewhat particular to *nix systems, only add if relevant */

	#ifdef MACOS_X
	fs_apppath = Cvar_Get("fs_apppath", Sys_DefaultAppPath(),
		CVAR_INIT|CVAR_PROTECTED);
	/* Make MacOSX also include the base path included with the .app bundle */
	if(fs_apppath->string[0])
		FS_AddGameDirectory(fs_apppath->string, gameName);
	#endif

	/* NOTE: same filtering below for mods and basegame */
	if(fs_homepath->string[0] && Q_stricmp(fs_homepath->string,
		   fs_basepath->string)){
		FS_CreatePath(fs_homepath->string);
		FS_AddGameDirectory(fs_homepath->string, gameName);
	}

	/* check for additional base game so mods can be based upon other mods */
	if(fs_basegame->string[0] && Q_stricmp(fs_basegame->string, gameName)){
		if(fs_basepath->string[0])
			FS_AddGameDirectory(fs_basepath->string,
				fs_basegame->string);
		if(fs_homepath->string[0] &&
		   Q_stricmp(fs_homepath->string,fs_basepath->string))
			FS_AddGameDirectory(fs_homepath->string,
				fs_basegame->string);
	}

	/* check for additional game folder for mods */
	if(fs_gamedirvar->string[0] &&
	   Q_stricmp(fs_gamedirvar->string, gameName)){
		if(fs_basepath->string[0])
			FS_AddGameDirectory(fs_basepath->string,
				fs_gamedirvar->string);
		if(fs_homepath->string[0] &&
		   Q_stricmp(fs_homepath->string,fs_basepath->string))
			FS_AddGameDirectory(fs_homepath->string,
				fs_gamedirvar->string);
	}

	/* add our commands */
	Cmd_AddCommand("path", FS_Path_f);
	Cmd_AddCommand("dir", FS_Dir_f);
	Cmd_AddCommand("fdir", FS_NewDir_f);
	Cmd_AddCommand("touchFile", FS_TouchFile_f);
	Cmd_AddCommand("which", FS_Which_f);

	/* https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=506 */
	/* reorder the pure pk3 files according to server order */
	FS_ReorderPurePaks();

	/* print the current search paths */
	FS_Path_f();

	fs_gamedirvar->modified = qfalse;	/* We just loaded, it's not modified */

	Com_Printf("----------------------\n");

#ifdef FS_MISSING
	if(missingFiles == NULL)
		missingFiles = fopen("\\missing.txt", "ab");
#endif
	Com_Printf("%d files in pk3 files\n", fs_packFiles);
}

#ifndef STANDALONE
/*
 * Check whether any of the original id pak files is present,
 * and start up in standalone mode, if there are none and a
 * different com_basegame was set.
 * Note: If you're building a game that doesn't depend on the
 * Q3 media pak0.pk3, you'll want to remove this by defining
 * STANDALONE in q_shared.h
 */
static void
FS_CheckPak0(void)
{
	searchpath_t *path;
	pack_t *curpack;
	qbool founddemo = qfalse;
	unsigned int foundPak = 0, foundTA = 0;

	for(path = fs_searchpaths; path; path = path->next){
		const char * pakBasename = path->pack->pakBasename;

		if(!path->pack)
			continue;

		curpack = path->pack;

		if(!Q_stricmpn(curpack->pakGamename, "demoq3", MAX_OSPATH)
		   && !Q_stricmpn(pakBasename, "pak0", MAX_OSPATH)){
			if(curpack->checksum == DEMO_PAK0_CHECKSUM)
				founddemo = qtrue;
		}else if(!Q_stricmpn(curpack->pakGamename, BASEGAME, MAX_OSPATH)
			 && strlen(pakBasename) == 4 &&
			 !Q_stricmpn(pakBasename, "pak", 3)
			 && pakBasename[3] >= '0' && pakBasename[3] <= '0' +
			 NUM_ID_PAKS - 1){
			if(curpack->checksum !=
			   pak_checksums[pakBasename[3]-'0']){
				if(pakBasename[3] == '0')
					Com_Printf(
						"\n\n"
						"**************************************************\n"
						"WARNING: " BASEGAME
						"/pak0.pk3 is present but its checksum (%u)\n"
						"is not correct. Please re-copy pak0.pk3 from your\n"
						"legitimate Q3 CDROM.\n"
						"**************************************************\n\n\n",
						curpack->checksum);
				else
					Com_Printf(
						"\n\n"
						"**************************************************\n"
						"WARNING: " BASEGAME
						"/pak%d.pk3 is present but its checksum (%u)\n"
						"is not correct. Please re-install the point release\n"
						"**************************************************\n\n\n",
						pakBasename[3]-'0',
						curpack->checksum);
			}

			foundPak |= 1<<(pakBasename[3]-'0');
		}else if(!Q_stricmpn(curpack->pakGamename, BASETA, MAX_OSPATH)
			 && strlen(pakBasename) == 4 &&
			 !Q_stricmpn(pakBasename, "pak", 3)
			 && pakBasename[3] >= '0' && pakBasename[3] <= '0' +
			 NUM_TA_PAKS - 1){
			if(curpack->checksum !=
			   missionpak_checksums[pakBasename[3]-'0'])
				Com_Printf(
					"\n\n"
					"**************************************************\n"
					"WARNING: " BASETA
					"/pak%d.pk3 is present but its checksum (%u)\n"
					"is not correct. Please re-install Team Arena\n"
					"**************************************************\n\n\n",
					pakBasename[3]-'0', curpack->checksum);

			foundTA |= 1 << (pakBasename[3]-'0');
		}else{
			int index;

			/* Finally check whether this pak's checksum is listed because the user tried
			 * to trick us by renaming the file, and set foundPak's highest bit to indicate this case. */

			for(index = 0; index < ARRAY_LEN(pak_checksums); index++)
				if(curpack->checksum == pak_checksums[index]){
					Com_Printf(
						"\n\n"
						"**************************************************\n"
						"WARNING: %s is renamed pak file %s%cpak%d.pk3\n"
						"Running in standalone mode won't work\n"
						"Please rename, or remove this file\n"
						"**************************************************\n\n\n",
						curpack->pakFilename, BASEGAME,
						PATH_SEP, index);


					foundPak |= 0x80000000;
				}

			for(index = 0; index < ARRAY_LEN(missionpak_checksums);
			    index++)
				if(curpack->checksum ==
				   missionpak_checksums[index]){
					Com_Printf(
						"\n\n"
						"**************************************************\n"
						"WARNING: %s is renamed pak file %s%cpak%d.pk3\n"
						"Running in standalone mode won't work\n"
						"Please rename, or remove this file\n"
						"**************************************************\n\n\n",
						curpack->pakFilename, BASETA,
						PATH_SEP, index);

					foundTA |= 0x80000000;
				}
		}
	}

	if(!foundPak && !foundTA && Q_stricmp(com_basegame->string, BASEGAME))
		Cvar_Set("com_standalone", "1");
	else
		Cvar_Set("com_standalone", "0");

	if(!com_standalone->integer){
		if(!(foundPak & 0x01))
			if(founddemo){
				Com_Printf(
					"\n\n"
					"**************************************************\n"
					"WARNING: It looks like you're using pak0.pk3\n"
					"from the demo. This may work fine, but it is not\n"
					"guaranteed or supported.\n"
					"**************************************************\n\n\n");

				foundPak |= 0x01;
			}
	}


	if(!com_standalone->integer && (foundPak & 0x1ff) != 0x1ff){
		char errorText[MAX_STRING_CHARS] = "";

		if((foundPak & 0x01) != 0x01)
			Q_strcat(errorText, sizeof(errorText),
				"\"pak0.pk3\" is missing. Please copy it "
				"from your legitimate Q3 CDROM. ");

		if((foundPak & 0x1fe) != 0x1fe)
			Q_strcat(errorText, sizeof(errorText),
				"Point Release files are missing. Please "
				"re-install the 1.32 point release. ");

		Q_strcat(errorText, sizeof(errorText),
			va("Also check that your ioq3 executable is in "
			   "the correct place and that every file "
			   "in the \"%s\" directory is present and readable",
				BASEGAME));

		Com_Errorf(ERR_FATAL, "%s", errorText);
	}

	if(!com_standalone->integer && foundTA && (foundTA & 0x0f) != 0x0f){
		char errorText[MAX_STRING_CHARS] = "";

		if((foundTA & 0x01) != 0x01)
			Q_sprintf(
				errorText, sizeof(errorText),
				"\"" BASETA
				"%cpak0.pk3\" is missing. Please copy it "
				"from your legitimate Quake 3 Team Arena CDROM. ",
				PATH_SEP);

		if((foundTA & 0x0e) != 0x0e)
			Q_strcat(
				errorText, sizeof(errorText),
				"Team Arena Point Release files are missing. Please "
				"re-install the latest Team Arena point release.");

		Com_Errorf(ERR_FATAL, "%s", errorText);
	}
}
#endif

/*
 * Returns a space separated string containing the checksums of all loaded pk3 files.
 * Servers with sv_pure set will get this string and pass it to clients.
 */
const char *
FS_LoadedPakChecksums(void)
{
	static char info[BIG_INFO_STRING];
	searchpath_t *search;

	info[0] = 0;

	for(search = fs_searchpaths; search; search = search->next){
		/* is the element a pak file? */
		if(!search->pack)
			continue;

		Q_strcat(info, sizeof(info), va("%i ", search->pack->checksum));
	}

	return info;
}

/*
 * Returns a space separated string containing the names of all loaded pk3 files.
 * Servers with sv_pure set will get this string and pass it to clients.
 */
const char *
FS_LoadedPakNames(void)
{
	static char info[BIG_INFO_STRING];
	searchpath_t *search;

	info[0] = 0;

	for(search = fs_searchpaths; search; search = search->next){
		/* is the element a pak file? */
		if(!search->pack)
			continue;

		if(*info)
			Q_strcat(info, sizeof(info), " ");
		Q_strcat(info, sizeof(info), search->pack->pakBasename);
	}

	return info;
}

/*
 * Returns a space separated string containing the pure checksums of all loaded pk3 files.
 * Servers with sv_pure use these checksums to compare with the checksums the clients send
 * back to the server.
 */
const char *
FS_LoadedPakPureChecksums(void)
{
	static char info[BIG_INFO_STRING];
	searchpath_t *search;

	info[0] = 0;

	for(search = fs_searchpaths; search; search = search->next){
		/* is the element a pak file? */
		if(!search->pack)
			continue;

		Q_strcat(info, sizeof(info),
			va("%i ", search->pack->pure_checksum));
	}

	return info;
}

/*
 * Returns a space separated string containing the checksums of all referenced pk3 files.
 * The server will send this to the clients so they can check which files should be auto-downloaded.
 */
const char*
FS_ReferencedPakChecksums(void)
{
	static char info[BIG_INFO_STRING];
	searchpath_t *search;

	info[0] = 0;
	for(search = fs_searchpaths; search; search = search->next)
		/* is the element a pak file? */
		if(search->pack)
			if(search->pack->referenced ||
			   Q_stricmpn(search->pack->pakGamename,
				   com_basegame->string,
				   strlen(com_basegame->string)))
				Q_strcat(info, sizeof(info),
					va("%i ", search->pack->checksum));
	return info;
}

/*
 * Returns a space separated string containing the pure checksums of all referenced pk3 files.
 * Servers with sv_pure set will get this string back from clients for pure validation
 *
 * The string has a specific order, "cgame ui @ ref1 ref2 ref3 ..."
 */
const char*
FS_ReferencedPakPureChecksums(void)
{
	static char info[BIG_INFO_STRING];
	searchpath_t *search;
	int nFlags, numPaks, checksum;

	info[0] = 0;

	checksum = fs_checksumFeed;
	numPaks = 0;
	for(nFlags = FS_CGAME_REF; nFlags; nFlags = nFlags >> 1){
		if(nFlags & FS_GENERAL_REF){
			/* add a delimter between must haves and general refs
			 * Q_strcat(info, sizeof(info), "@ "); */
			info[strlen(info)+1] = '\0';
			info[strlen(info)+2] = '\0';
			info[strlen(info)] = '@';
			info[strlen(info)] = ' ';
		}
		for(search = fs_searchpaths; search; search = search->next)
			/* is the element a pak file and has it been referenced based on flag? */
			if(search->pack &&
			   (search->pack->referenced & nFlags)){
				Q_strcat(info, sizeof(info),
					va("%i ", search->pack->pure_checksum));
				if(nFlags & (FS_CGAME_REF | FS_UI_REF))
					break;
				checksum ^= search->pack->pure_checksum;
				numPaks++;
			}
	}
	/* last checksum is the encoded number of referenced pk3s */
	checksum ^= numPaks;
	Q_strcat(info, sizeof(info), va("%i ", checksum));

	return info;
}

/*
 * Returns a space separated string containing the names of all referenced pk3 files.
 * The server will send this to the clients so they can check which files should be auto-downloaded.
 */
const char *
FS_ReferencedPakNames(void)
{
	static char info[BIG_INFO_STRING];
	searchpath_t *search;

	info[0] = 0;

	/* we want to return ALL pk3's from the fs_game path
	 * and referenced one's from baseq3 */
	for(search = fs_searchpaths; search; search = search->next){
		/* is the element a pak file? */
		if(search->pack)
			if(search->pack->referenced ||
			   Q_stricmpn(search->pack->pakGamename,
			   com_basegame->string,
			   strlen(com_basegame->string)))
			then{
				if(*info)
					Q_strcat(info, sizeof(info), " ");
				Q_strcat(info, sizeof(info),
					search->pack->pakGamename);
				Q_strcat(info, sizeof(info), "/");
				Q_strcat(info, sizeof(info),
					search->pack->pakBasename);
			}
	}

	return info;
}

/* clears referenced booleans on loaded pk3s */
void
FS_ClearPakReferences(int flags)
{
	searchpath_t *search;

	if(!flags)
		flags = -1;
	for(search = fs_searchpaths; search; search = search->next)
		/* is the element a pak file and has it been referenced? */
		if(search->pack)
			search->pack->referenced &= ~flags;
}

/*
 * If the string is empty, all data sources will be allowed.
 * If not empty, only pk3 files that match one of the space
 * separated checksums will be checked for files, with the
 * exception of .cfg and .dat files.
 */
void
FS_PureServerSetLoadedPaks(const char *pakSums, const char *pakNames)
{
	int i, c, d;

	Cmd_TokenizeString(pakSums);

	c = Cmd_Argc();
	if(c > MAX_SEARCH_PATHS)
		c = MAX_SEARCH_PATHS;

	fs_numServerPaks = c;

	for(i = 0; i < c; i++)
		fs_serverPaks[i] = atoi(Cmd_Argv(i));

	if(fs_numServerPaks)
		Com_DPrintf("Connected to a pure server.\n");
	else if(fs_reordered){
		/* https://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=540
		 * force a restart to make sure the search order will be correct */
		Com_DPrintf("FS search reorder is required\n");
		FS_Restart(fs_checksumFeed);
		return;
	}

	for(i = 0; i < c; i++){
		if(fs_serverPakNames[i])
			Z_Free(fs_serverPakNames[i]);
		fs_serverPakNames[i] = NULL;
	}
	if(pakNames && *pakNames){
		Cmd_TokenizeString(pakNames);

		d = Cmd_Argc();
		if(d > MAX_SEARCH_PATHS)
			d = MAX_SEARCH_PATHS;

		for(i = 0; i < d; i++)
			fs_serverPakNames[i] = Copystr(Cmd_Argv(i));
	}
}

/*
 * The checksums and names of the pk3 files referenced at the server
 * are sent to the client and stored here. The client will use these
 * checksums to see if any pk3 files need to be auto-downloaded.
 */
void
FS_PureServerSetReferencedPaks(const char *pakSums, const char *pakNames)
{
	int i, c, d = 0;

	Cmd_TokenizeString(pakSums);

	c = Cmd_Argc();
	if(c > MAX_SEARCH_PATHS)
		c = MAX_SEARCH_PATHS;

	for(i = 0; i < c; i++)
		fs_serverReferencedPaks[i] = atoi(Cmd_Argv(i));

	for(i = 0; i < ARRAY_LEN(fs_serverReferencedPakNames); i++){
		if(fs_serverReferencedPakNames[i])
			Z_Free(fs_serverReferencedPakNames[i]);

		fs_serverReferencedPakNames[i] = NULL;
	}

	if(pakNames && *pakNames){
		Cmd_TokenizeString(pakNames);
		d = Cmd_Argc();
		if(d > c)
			d = c;
		for(i = 0; i < d; i++)
			fs_serverReferencedPakNames[i] = Copystr(Cmd_Argv(i));
	}
	/* ensure that there are as many checksums as there are pak names. */
	if(d < c)
		c = d;
	fs_numServerReferencedPaks = c;
}

/*
 * Called only at inital startup, not when the filesystem
 * is resetting due to a game change
 */
void
FS_InitFilesystem(void)
{
	/* allow command line parms to override our defaults
	 * we have to specially handle this, because normal command
	 * line variable sets don't happen until after the filesystem
	 * has already been initialized */
	Com_Startupvar("fs_basepath");
	Com_Startupvar("fs_homepath");
	Com_Startupvar("fs_game");

	if(!FS_FilenameCompare(Cvar_VariableString("fs_game"),
		   com_basegame->string))
		Cvar_Set("fs_game", "");

	/* try to start up normally */
	FS_Startup(com_basegame->string);

#ifndef STANDALONE
	FS_CheckPak0( );
#endif

	/* if we can't find default.cfg, assume that the paths are
	 * busted and error out now, rather than getting an unreadable
	 * graphics screen when the font fails to load */
	if(FS_ReadFile("default.cfg", NULL) <= 0)
		Com_Errorf(ERR_FATAL, "Couldn't load default.cfg");

	Q_strncpyz(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	Q_strncpyz(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));
}

/* shutdown and restart the filesystem so changes to fs_gamedir can take effect */
void
FS_Restart(int checksumFeed)
{
	FS_Shutdown(qfalse);	/* free anything we currently have loaded */
	fs_checksumFeed = checksumFeed;
	FS_ClearPakReferences(0);
	FS_Startup(com_basegame->string);	/* try to start up normally */
#ifndef STANDALONE
	FS_CheckPak0( );
#endif
	/* 
	 * if we can't find default.cfg, assume that the paths are
	 * busted and error out now, rather than getting an unreadable
	 * graphics screen when the font fails to load 
	 */
	if(FS_ReadFile("default.cfg", NULL) <= 0){
		/* 
		 * this might happen when connecting to a pure server not using BASEGAME/pak0.pk3
		 * (for instance a TA demo server) 
		 */
		if(lastValidBase[0]){
			FS_PureServerSetLoadedPaks("", "");
			Cvar_Set("fs_basepath", lastValidBase);
			Cvar_Set("fs_gamedirvar", lastValidGame);
			lastValidBase[0] = '\0';
			lastValidGame[0] = '\0';
			FS_Restart(checksumFeed);
			Com_Errorf(ERR_DROP, "Invalid game folder");
			return;
		}
		Com_Errorf(ERR_FATAL, "Couldn't load default.cfg");
	}
	if(Q_stricmp(fs_gamedirvar->string, lastValidGame))
		/* skip the user.cfg if "safe" is on the command line */
		if(!Com_Insafemode())
			Cbuf_AddText ("exec " Q3CONFIG_CFG "\n");
	Q_strncpyz(lastValidBase, fs_basepath->string, sizeof(lastValidBase));
	Q_strncpyz(lastValidGame, fs_gamedirvar->string, sizeof(lastValidGame));
}

/*
 * Restart if necessary
 * Return qtrue if restarting due to game directory changed, qfalse otherwise
 */
qbool
FS_ConditionalRestart(int checksumFeed, qbool disconnect)
{
	if(fs_gamedirvar->modified){
		if(FS_FilenameCompare(lastValidGame, fs_gamedirvar->string) &&
		   (*lastValidGame ||
		    FS_FilenameCompare(fs_gamedirvar->string,
			    com_basegame->string)) &&
		   (*fs_gamedirvar->string ||
		    FS_FilenameCompare(lastValidGame, com_basegame->string)))
		then{
			Com_Gamerestart(checksumFeed, disconnect);
			return qtrue;
		}else
			fs_gamedirvar->modified = qfalse;
	}

	if(checksumFeed != fs_checksumFeed)
		FS_Restart(checksumFeed);
	else if(fs_numServerPaks && !fs_reordered)
		FS_ReorderPurePaks();

	return qfalse;
}

/*
 * Handle-based file calls for virtual machines
 */

int
FS_FOpenFileByMode(const char *qpath, fileHandle_t *f, fsMode_t mode)
{
	int r;
	qbool sync;

	sync = qfalse;

	switch(mode){
	case FS_READ:
		r = FS_FOpenFileRead(qpath, f, qtrue);
		break;
	case FS_WRITE:
		*f = FS_FOpenFileWrite(qpath);
		r = 0;
		if(*f == 0)
			r = -1;
		break;
	case FS_APPEND_SYNC:
		sync = qtrue;
	case FS_APPEND:
		*f = FS_FOpenFileAppend(qpath);
		r = 0;
		if(*f == 0)
			r = -1;
		break;
	default:
		Com_Errorf(ERR_FATAL, "FS_FOpenFileByMode: bad mode");
		return -1;
	}

	if(!f)
		return r;

	if(*f){
		if(fsh[*f].zipFile == qtrue)
			fsh[*f].baseOffset = unztell(fsh[*f].handleFiles.file.z);
		else
			fsh[*f].baseOffset = ftell(fsh[*f].handleFiles.file.o);
		fsh[*f].fileSize = r;
		fsh[*f].streamed = qfalse;

		if(mode == FS_READ)
			fsh[*f].streamed = qtrue;
	}
	fsh[*f].handleSync = sync;

	return r;
}

int
FS_FTell(fileHandle_t f)
{
	int pos;
	if(fsh[f].zipFile == qtrue)
		pos = unztell(fsh[f].handleFiles.file.z);
	else
		pos = ftell(fsh[f].handleFiles.file.o);
	return pos;
}

void
FS_Flush(fileHandle_t f)
{
	fflush(fsh[f].handleFiles.file.o);
}

void
FS_FilenameCompletion(const char *dir, const char *ext,
		      qbool stripExt, void (*callback)(
			      const char *s), qbool allowNonPureFilesOnDisk)
{
	char	**filenames;
	int	nfiles;
	int	i;
	char	filename[ MAX_STRING_CHARS ];

	filenames = FS_ListFilteredFiles(dir, ext, NULL, &nfiles,
		allowNonPureFilesOnDisk);

	FS_SortFileList(filenames, nfiles);

	for(i = 0; i < nfiles; i++){
		FS_ConvertPath(filenames[ i ]);
		Q_strncpyz(filename, filenames[ i ], MAX_STRING_CHARS);

		if(stripExt)
			Q_stripext(filename, filename, sizeof(filename));

		callback(filename);
	}
	FS_FreeFileList(filenames);
}

const char *
FS_GetCurrentGameDir(void)
{
	if(fs_gamedirvar->string[0])
		return fs_gamedirvar->string;

	return com_basegame->string;
}
