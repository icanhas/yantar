/* virtual machine */
/*
 * Copyright (C) 1999-2005 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License.
 */

/*
 * intermix code and data
 * symbol table
 *
 * a dll has one imported function: VM_SystemCall
 * and one exported function: Perform
 */

#include "local.h"

enum { MAX_VM = 3 };
Vm *currentVM = NULL;
Vm *lastVM = NULL;
Vm vmTable[MAX_VM];
int vm_debugLevel;
static int forced_unload;	/* used by comerrorf to get rid of running
					 * vm's before longjmp */

void VM_VmInfo_f(void);
void VM_VmProfile_f(void);

#if 0	/* 64bit! */
/* converts a VM pointer to a C pointer and
 * checks to make sure that the range is acceptable */
void    *
VM_VM2C(vmptr_t p, int length)
{
	return (void*)p;
}
#endif

void
vmdebug(int level)
{
	vm_debugLevel = level;
}

void
vminit(void)
{
	cvarget("vm_cgame", "0", CVAR_ARCHIVE);	/* !@# SHIP WITH SET TO 2 */
	cvarget("vm_game", "0", CVAR_ARCHIVE);		/* !@# SHIP WITH SET TO 2 */
	cvarget("vm_ui", "0", CVAR_ARCHIVE);			/* !@# SHIP WITH SET TO 2 */

	cmdadd ("vmprofile", VM_VmProfile_f);
	cmdadd ("vminfo", VM_VmInfo_f);

	Q_Memset(vmTable, 0, sizeof(vmTable));
}

/* Assumes a program counter value */
const char *
VM_ValueToSymbol(Vm *vm, int value)
{
	vmSymbol_t	*sym;
	static char	text[MAX_TOKEN_CHARS];

	sym = vm->symbols;
	if(!sym)
		return "NO SYMBOLS";
	/* find the symbol */
	while(sym->next && sym->next->symValue <= value)
		sym = sym->next;
	if(value == sym->symValue)
		return sym->symName;
	Q_sprintf(text, sizeof(text), "%s+%i", sym->symName,
		value - sym->symValue);
	return text;
}

/* For profiling, find the symbol behind this value */
vmSymbol_t *
VM_ValueToFunctionSymbol(Vm *vm, int value)
{
	vmSymbol_t *sym;
	static vmSymbol_t nullSym;

	sym = vm->symbols;
	if(!sym)
		return &nullSym;

	while(sym->next && sym->next->symValue <= value)
		sym = sym->next;

	return sym;
}

int
VM_SymbolToValue(Vm *vm, const char *symbol)
{
	vmSymbol_t *sym;

	for(sym = vm->symbols; sym; sym = sym->next)
		if(!strcmp(symbol, sym->symName))
			return sym->symValue;
	return 0;
}

#if 0	/* 64bit! */
const char *
VM_SymbolForCompiledPointer(Vm *vm, void *code)
{
	int i;

	if(code < (void*)vm->codeBase)
		return "Before code block";
	if(code >= (void*)(vm->codeBase + vm->codeLength))
		return "After code block";

	/* find which original instruction it is after */
	for(i = 0; i < vm->codeLength; i++)
		if((void*)vm->instructionPointers[i] > code)
			break;
	i--;

	/* now look up the bytecode instruction pointer */
	return VM_ValueToSymbol(vm, i);
}
#endif

int
ParseHex(const char *text)
{
	int	value;
	int	c;

	value = 0;
	while((c = *text++) != 0){
		if(c >= '0' && c <= '9'){
			value = value * 16 + c - '0';
			continue;
		}
		if(c >= 'a' && c <= 'f'){
			value = value * 16 + 10 + c - 'a';
			continue;
		}
		if(c >= 'A' && c <= 'F'){
			value = value * 16 + 10 + c - 'A';
			continue;
		}
	}

	return value;
}

void
VM_LoadSymbols(Vm *vm)
{
	union {
		char	*c;
		void	*v;
	} mapfile;
	char	*text_p, *token;
	char	name[MAX_QPATH];
	char	symbols[MAX_QPATH];
	vmSymbol_t **prev, *sym;
	int	count;
	int	value;
	int	chars;
	int	segment;
	int	numInstructions;

	/* don't load symbols if not developer */
	if(!com_developer->integer)
		return;

	Q_stripext(vm->name, name, sizeof(name));
	Q_sprintf(symbols, sizeof(symbols), Pvmfiles "/%s.map", name);
	fsreadfile(symbols, &mapfile.v);
	if(!mapfile.c){
		comprintf("Couldn't load symbol file: %s\n", symbols);
		return;
	}

	numInstructions = vm->instructionCount;

	/* parse the symbols */
	text_p = mapfile.c;
	prev	= &vm->symbols;
	count = 0;

	for(;;){
		token = Q_readtok(&text_p);
		if(!token[0])
			break;
		segment = ParseHex(token);
		if(segment){
			Q_readtok(&text_p);
			Q_readtok(&text_p);
			continue;	/* only load code segment values */
		}

		token = Q_readtok(&text_p);
		if(!token[0]){
			comprintf("WARNING: incomplete line at end of file\n");
			break;
		}
		value = ParseHex(token);

		token = Q_readtok(&text_p);
		if(!token[0]){
			comprintf("WARNING: incomplete line at end of file\n");
			break;
		}
		chars	= strlen(token);
		sym	= hunkalloc(sizeof(*sym) + chars, Hhigh);
		*prev	= sym;
		prev	= &sym->next;
		sym->next = NULL;

		/* convert value from an instruction number to a code offset */
		if(value >= 0 && value < numInstructions)
			value = vm->instructionPointers[value];

		sym->symValue = value;
		Q_strncpyz(sym->symName, token, chars + 1);

		count++;
	}

	vm->numSymbols = count;
	comprintf("%i symbols parsed from %s\n", count, symbols);
	fsfreefile(mapfile.v);
}

/*
 * Dlls will call this directly
 *
 * rcg010206 The horror; the horror.
 *
 * The syscall mechanism relies on stack manipulation to get its args.
 * This is likely due to C's inability to pass "..." parameters to
 * a function in one clean chunk. On PowerPC Linux, these parameters
 * are not necessarily passed on the stack, so while (&arg[0] == arg)
 * is true, (&arg[1] == 2nd function parameter) is not necessarily
 * accurate, as arg's value might have been stored to the stack or
 * other piece of scratch memory to give it a valid address, but the
 * next parameter might still be sitting in a register.
 *
 * Quake's syscall system also assumes that the stack grows downward,
 * and that any needed types can be squeezed, safely, into a signed int.
 *
 * This hack below copies all needed values for an argument to a
 * array in memory, so that Quake can get the correct values. This can
 * also be used on systems where the stack grows upwards, as the
 * presumably standard and safe stdargs.h macros are used.
 *
 * As for having enough space in a signed int for your datatypes, well,
 * it might be better to wait for DOOM 3 before you start porting.  :)
 *
 * The original code, while probably still inherently dangerous, seems
 * to work well enough for the platforms it already works on. Rather
 * than add the performance hit for those platforms, the original code
 * is still in use there.
 *
 * For speed, we just grab 15 arguments, and don't worry about exactly
 * how many the syscall actually needs; the extra is thrown away.
 *
 */
intptr_t QDECL
VM_DllSyscall(intptr_t arg, ...)
{
#if !id386 || defined(__clang__)
	/* rcg010206 - see commentary above */
	intptr_t	args[16];
	int i;
	va_list		ap;

	args[0] = arg;

	va_start(ap, arg);
	for(i = 1; i < ARRAY_LEN (args); i++)
		args[i] = va_arg(ap, intptr_t);
	va_end(ap);

	return currentVM->systemCall(args);
#else	/* original id code */
	return currentVM->systemCall(&arg);
#endif
}

/* Load a .qvm file */
Vmheader *
VM_LoadQVM(Vm *vm, qbool alloc, qbool unpure)
{
	int	dataLength;
	int	i;
	char	filename[MAX_QPATH];
	union {
		Vmheader	*h;
		void		*v;
	} header;

	/* load the image */
	Q_sprintf(filename, sizeof(filename), Pvmfiles "/%s.qvm", vm->name);
	comprintf("Loading vm file %s...\n", filename);

	fsreadfiledir(filename, vm->searchPath, unpure, &header.v);

	if(!header.h){
		comprintf("Failed.\n");
		vmfree(vm);
		comprintf(S_COLOR_YELLOW "Warning: Couldn't open VM file %s\n",
			filename);
		return NULL;
	}

	/* show where the qvm was loaded from */
	fswhich(filename, vm->searchPath);

	if(LittleLong(header.h->vmMagic) == VM_MAGIC_VER2){
		comprintf("...which has vmMagic VM_MAGIC_VER2\n");

		/* byte swap the header */
		for(i = 0; i < sizeof(Vmheader) / 4; i++)
			((int*)header.h)[i] = LittleLong(((int*)header.h)[i]);

		/* validate */
		if(header.h->jtrgLength < 0
		   || header.h->bssLength < 0
		   || header.h->dataLength < 0
		   || header.h->litLength < 0
		   || header.h->codeLength <= 0){
			vmfree(vm);
			fsfreefile(header.v);

			comprintf(S_COLOR_YELLOW "Warning: %s has bad header\n",
				filename);
			return NULL;
		}
	}else if(LittleLong(header.h->vmMagic) == VM_MAGIC){
		/* byte swap the header
		 * sizeof( Vmheader ) - sizeof( int ) is the 1.32b vm header size */
		for(i = 0; i < (sizeof(Vmheader) - sizeof(int)) / 4; i++)
			((int*)header.h)[i] = LittleLong(((int*)header.h)[i]);

		/* validate */
		if(header.h->bssLength < 0
		   || header.h->dataLength < 0
		   || header.h->litLength < 0
		   || header.h->codeLength <= 0){
			vmfree(vm);
			fsfreefile(header.v);

			comprintf(S_COLOR_YELLOW "Warning: %s has bad header\n",
				filename);
			return NULL;
		}
	}else{
		vmfree(vm);
		fsfreefile(header.v);

		comprintf(
			S_COLOR_YELLOW
			"Warning: %s does not have a recognisable "
			"magic number in its header\n",
			filename);
		return NULL;
	}

	/* round up to next power of 2 so all data operations can
	 * be mask protected */
	dataLength = header.h->dataLength + header.h->litLength +
		     header.h->bssLength;
	for(i = 0; dataLength > (1 << i); i++)
		;
	dataLength = 1 << i;

	if(alloc){
		/* allocate zero filled space for initialized and uninitialized data */
		vm->dataBase = hunkalloc(dataLength, Hhigh);
		vm->dataMask = dataLength - 1;
	}else{
		/* clear the data, but make sure we're not clearing more than allocated */
		if(vm->dataMask + 1 != dataLength){
			vmfree(vm);
			fsfreefile(header.v);

			comprintf(S_COLOR_YELLOW
				"Warning: Data region size of %s not matching after "
				"vmrestart()\n", filename);
			return NULL;
		}

		Q_Memset(vm->dataBase, 0, dataLength);
	}

	/* copy the intialized data */
	Q_Memcpy(vm->dataBase, (byte*)header.h + header.h->dataOffset,
		header.h->dataLength + header.h->litLength);

	/* byte swap the longs */
	for(i = 0; i < header.h->dataLength; i += 4)
		*(int*)(vm->dataBase +
			i) = LittleLong(*(int*)(vm->dataBase + i));

	if(header.h->vmMagic == VM_MAGIC_VER2){
		int prevNumJumpTableTargets = vm->numJumpTableTargets;

		header.h->jtrgLength &= ~0x03;
		vm->numJumpTableTargets = header.h->jtrgLength >> 2;
		comprintf("Loading %d jump table targets\n",
			vm->numJumpTableTargets);

		if(alloc)
			vm->jumpTableTargets = hunkalloc(header.h->jtrgLength,
				Hhigh);
		else{
			if(vm->numJumpTableTargets != prevNumJumpTableTargets){
				vmfree(vm);
				fsfreefile(header.v);

				comprintf(
					S_COLOR_YELLOW
					"Warning: Jump table size of %s not matching after "
					"vmrestart()\n",
					filename);
				return NULL;
			}

			Q_Memset(vm->jumpTableTargets, 0, header.h->jtrgLength);
		}

		Q_Memcpy(
			vm->jumpTableTargets, (byte*)header.h +
			header.h->dataOffset +
			header.h->dataLength + header.h->litLength,
			header.h->jtrgLength);

		/* byte swap the longs */
		for(i = 0; i < header.h->jtrgLength; i += 4)
			*(int*)(vm->jumpTableTargets +
				i) =
				LittleLong(*(int*)(vm->jumpTableTargets + i));
	}

	return header.h;
}

/*
 * Reload the data, but leave everything else in place
 * This allows a server to do a map_restart without changing memory allocation
 *
 * We need to make sure that servers can access unpure QVMs (not contained in any pak)
 * even if the client is pure, so take "unpure" as argument.
 */
Vm *
vmrestart(Vm *vm, qbool unpure)
{
	Vmheader *header;

	/* DLL's can't be restarted in place */
	if(vm->dllHandle){
		char name[MAX_QPATH];
		intptr_t (*systemCall)(intptr_t *parms);

		systemCall = vm->systemCall;
		Q_strncpyz(name, vm->name, sizeof(name));

		vmfree(vm);

		vm = vmcreate(name, systemCall, VMnative);
		return vm;
	}
	
	/* load the image */
	comprintf("vmrestart()\n");
	if(!(header = VM_LoadQVM(vm, qfalse, unpure))){
		comerrorf(ERR_DROP, "vmrestart failed");
		return NULL;
	}

	/* free the original file */
	fsfreefile(header);
	return vm;
}

/*
 * If image ends in .qvm it will be interpreted, otherwise
 * it will attempt to load as a system dll
 */
Vm *
vmcreate(const char *module, intptr_t (*systemCalls)(intptr_t *),
	  Vmmode interpret)
{
	Vm	*vm;
	Vmheader *header;
	int	i, remaining, retval;
	char	filename[MAX_OSPATH];
	void	*startSearch = NULL;

	if(!module || !module[0] || !systemCalls)
		comerrorf(ERR_FATAL, "vmcreate: bad parms");

	remaining = hunkmemremaining();

	/* see if we already have the VM */
	for(i = 0; i < MAX_VM; i++)
		if(!Q_stricmp(vmTable[i].name, module)){
			vm = &vmTable[i];
			return vm;
		}

	/* find a free vm */
	for(i = 0; i < MAX_VM; i++)
		if(!vmTable[i].name[0])
			break;

	if(i == MAX_VM)
		comerrorf(ERR_FATAL, "vmcreate: no free Vm");

	vm = &vmTable[i];

	Q_strncpyz(vm->name, module, sizeof(vm->name));

	do {
		retval = fsfindvm(&startSearch, filename, sizeof(filename),
				module,
				(interpret == VMnative));

		if(retval == VMnative){
			comprintf("Try loading dll file %s\n", filename);

			vm->dllHandle =
				sysloadgamedll(filename, &vm->entryPoint,
					VM_DllSyscall);

			if(vm->dllHandle){
				vm->systemCall = systemCalls;
				return vm;
			}

			comprintf("Failed loading dll, trying next\n");
		}else if(retval == VMcompiled){
			vm->searchPath = startSearch;
			if((header = VM_LoadQVM(vm, qtrue, qfalse)))
				break;

			/* vmfree overwrites the name on failed load */
			Q_strncpyz(vm->name, module, sizeof(vm->name));
		}
	} while(retval >= 0);

	if(retval < 0)
		return NULL;

	vm->systemCall = systemCalls;

	/* allocate space for the jump targets, which will be filled in by the compile/prep functions */
	vm->instructionCount = header->instructionCount;
	vm->instructionPointers =
		hunkalloc(vm->instructionCount *
			sizeof(*vm->instructionPointers),
			Hhigh);

	/* copy or compile the instructions */
	vm->codeLength = header->codeLength;
	vm->compiled = qfalse;

#ifdef NO_VM_COMPILED
	if(interpret >= VMcompiled){
		comprintf(
			"Architecture doesn't have a bytecode compiler, using interpreter\n");
		interpret = VMbytecode;
	}
#else
	if(interpret != VMbytecode){
		vm->compiled = qtrue;
		VM_Compile(vm, header);
	}
#endif
	/* VM_Compile may have reset vm->compiled if compilation failed */
	if(!vm->compiled)
		VM_PrepareInterpreter(vm, header);

	/* free the original file */
	fsfreefile(header);
	
	/* load the map file */
	VM_LoadSymbols(vm);
	
	/* the stack is implicitly at the end of the image */
	vm->programStack = vm->dataMask + 1;
	vm->stackBottom = vm->programStack - PROGRAM_STACK_SIZE;

	comprintf("%s loaded in %d bytes on the hunk\n", module,
		remaining - hunkmemremaining());
	return vm;
}

void
vmfree(Vm *vm)
{

	if(!vm)
		return;

	if(vm->callLevel){
		if(!forced_unload){
			comerrorf(ERR_FATAL, "vmfree(%s) on running vm",
				vm->name);
			return;
		}else
			comprintf("forcefully unloading %s vm\n", vm->name);
	}

	if(vm->destroy)
		vm->destroy(vm);
	if(vm->dllHandle){
		sysunloaddll(vm->dllHandle);
		Q_Memset(vm, 0, sizeof(*vm));
	}
#if 0	/* now automatically freed by hunk */
	if(vm->codeBase)
		zfree(vm->codeBase);
	if(vm->dataBase)
		zfree(vm->dataBase);
	if(vm->instructionPointers)
		zfree(vm->instructionPointers);

#endif
	Q_Memset(vm, 0, sizeof(*vm));

	currentVM = NULL;
	lastVM = NULL;
}

void
vmclear(void)
{
	int i;
	for(i=0; i<MAX_VM; i++)
		vmfree(&vmTable[i]);
}

void
vmsetforceunload(void)
{
	forced_unload = 1;
}

void
vmclearforceunload(void)
{
	forced_unload = 0;
}

void *
vmargptr(intptr_t intValue)
{
	if(!intValue)
		return NULL;
	/* currentVM is missing on reconnect */
	if(currentVM==NULL)
		return NULL;
	if(currentVM->entryPoint)
		return (void*)(currentVM->dataBase + intValue);
	else
		return (void*)(currentVM->dataBase +
			       (intValue & currentVM->dataMask));
}

void *
vmexplicitargptr(Vm *vm, intptr_t intValue)
{
	if(!intValue)
		return NULL;
	/* currentVM is missing on reconnect here as well? */
	if(currentVM==NULL)
		return NULL;
	if(vm->entryPoint)
		return (void*)(vm->dataBase + intValue);
	else
		return (void*)(vm->dataBase + (intValue & vm->dataMask));
}

/*
 * Upon a system call, the stack will look like:
 *
 * sp+32	parm1
 * sp+28	parm0
 * sp+24	return value
 * sp+20	return address
 * sp+16	local1
 * sp+14	local0
 * sp+12	arg1
 * sp+8	arg0
 * sp+4	return stack
 * sp		return address
 *
 * An interpreted function will immediately execute
 * an OP_ENTER instruction, which will subtract space for
 * locals from sp
 */
intptr_t QDECL
vmcall(Vm *vm, int callnum, ...)
{
	Vm	*oldVM;
	intptr_t r;
	int	i;

	if(!vm || !vm->name[0])
		comerrorf(ERR_FATAL, "vmcall with NULL vm");
	oldVM = currentVM;
	currentVM = vm;
	lastVM = vm;
	if(vm_debugLevel)
		comprintf("vmcall( %d )\n", callnum);

	++vm->callLevel;
	/* if we have a dll loaded, call it directly */
	if(vm->entryPoint){
		/* rcg010207 -  see dissertation at top of VM_DllSyscall() in this file. */
		int args[10];
		va_list ap;
		va_start(ap, callnum);
		for(i = 0; i < ARRAY_LEN(args); i++)
			args[i] = va_arg(ap, int);
		va_end(ap);

		r = vm->entryPoint(callnum,  args[0],  args[1],  args[2],
				args[3],
				args[4],  args[5],  args[6], args[7],
				args[8],
				args[9]);
	}else{
#if (id386 || idsparc) && !defined(__clang__)	/* i386/sparc calling convention doesn't need conversion */
#ifndef NO_VM_COMPILED
		if(vm->compiled)
			r = vmcallCompiled(vm, (int*)&callnum);
		else
#endif
		r = vmcallInterpreted(vm, (int*)&callnum);
#else
		struct {
			int	callnum;
			int	args[10];
		} a;
		va_list ap;

		a.callnum = callnum;
		va_start(ap, callnum);
		for(i = 0; i < ARRAY_LEN(a.args); i++)
			a.args[i] = va_arg(ap, int);
		va_end(ap);
#ifndef NO_VM_COMPILED
		if(vm->compiled)
			r = vmcallCompiled(vm, &a.callnum);
		else
#endif
		r = vmcallInterpreted(vm, &a.callnum);
#endif
	}
	--vm->callLevel;

	if(oldVM != NULL)
		currentVM = oldVM;
	return r;
}

static int QDECL
VM_ProfileSort(const void *a, const void *b)
{
	vmSymbol_t *sa, *sb;

	sa = *(vmSymbol_t**)a;
	sb = *(vmSymbol_t**)b;

	if(sa->profileCount < sb->profileCount)
		return -1;
	if(sa->profileCount > sb->profileCount)
		return 1;
	return 0;
}

void
VM_VmProfile_f(void)
{
	Vm	*vm;
	vmSymbol_t **sorted, *sym;
	int i;
	double total;

	if(!lastVM)
		return;
	vm = lastVM;
	if(!vm->numSymbols)
		return;
	sorted = zalloc(vm->numSymbols * sizeof(*sorted));
	sorted[0] = vm->symbols;
	total = sorted[0]->profileCount;
	for(i = 1; i < vm->numSymbols; i++){
		sorted[i] = sorted[i-1]->next;
		total += sorted[i]->profileCount;
	}
	qsort(sorted, vm->numSymbols, sizeof(*sorted), VM_ProfileSort);
	for(i = 0; i < vm->numSymbols; i++){
		int perc;

		sym = sorted[i];
		perc = 100 * (float)sym->profileCount / total;
		comprintf("%2i%% %9i %s\n", perc, sym->profileCount,
			sym->symName);
		sym->profileCount = 0;
	}
	comprintf("    %9.0f total\n", total);
	zfree(sorted);
}

void
VM_VmInfo_f(void)
{
	Vm	*vm;
	int i;

	comprintf("Registered virtual machines:\n");
	for(i = 0; i < MAX_VM; i++){
		vm = &vmTable[i];
		if(!vm->name[0])
			break;
		comprintf("%s : ", vm->name);
		if(vm->dllHandle){
			comprintf("native\n");
			continue;
		}
		if(vm->compiled)
			comprintf("compiled on load\n");
		else
			comprintf("interpreted\n");
		comprintf("    code length : %7i\n", vm->codeLength);
		comprintf("    table length: %7i\n", vm->instructionCount*4);
		comprintf("    data length : %7i\n", vm->dataMask + 1);
	}
}

/* Insert calls to this while debugging the vm compiler */
void
VM_LogSyscalls(int *args)
{
	static int callnum;
	static FILE *f;

	if(!f)
		f = fopen("syscalls.log", "w");
	callnum++;
	fprintf(f, "%i: %p (%i) = %i %i %i %i\n", callnum,
		(void*)(args - (int*)currentVM->dataBase),
		args[0], args[1], args[2], args[3], args[4]);
}

/* Executes a block copy operation within currentVM data space */
void
VM_BlockCopy(unsigned int dest, unsigned int src, size_t n)
{
	unsigned int dataMask = currentVM->dataMask;

	if((dest & dataMask) != dest
	   || (src & dataMask) != src
	   || ((dest + n) & dataMask) != dest + n
	   || ((src + n) & dataMask) != src + n)
		comerrorf(ERR_DROP, "OP_BLOCK_COPY out of range!");

	Q_Memcpy(currentVM->dataBase + dest, currentVM->dataBase + src, n);
}
