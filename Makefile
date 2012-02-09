# If you require a different configuration from the defaults below, create a
# new file named "Makefile.local" in the same directory as this file and define
# your parameters there. This allows you to change configuration without
# causing problems with keeping up to date with the repository.

COMPILE_PLATFORM=$(shell uname|sed -e s/_.*//|tr '[:upper:]' '[:lower:]'|sed -e 's/\//_/g')

COMPILE_ARCH=$(shell uname -m | sed -e s/i.86/i386/)

ifeq ($(COMPILE_PLATFORM),sunos)
  # Solaris uname and GNU uname differ
  COMPILE_ARCH=$(shell uname -p | sed -e s/i.86/i386/)
endif
ifeq ($(COMPILE_PLATFORM),darwin)
  # Apple does some things a little differently...
  COMPILE_ARCH=$(shell uname -p | sed -e s/i.86/i386/)
endif

ifeq ($(COMPILE_PLATFORM),mingw32)
  ifeq ($(COMPILE_ARCH),i386)
    COMPILE_ARCH=x86
  endif
  ifeq ($(COMPILE_ARCH),x86_64)
    COMPILE_ARCH=x64
  endif
endif

ifndef BUILD_STANDALONE
  BUILD_STANDALONE = 1
endif
ifndef BUILD_CLIENT
  BUILD_CLIENT     = 1
endif
ifndef BUILD_CLIENT_SMP
  BUILD_CLIENT_SMP = 0
endif
ifndef BUILD_SERVER
  BUILD_SERVER     = 1
endif
ifndef BUILD_GAME_SO
  BUILD_GAME_SO    = 1
endif
ifndef BUILD_GAME_QVM
  BUILD_GAME_QVM   =
endif
ifndef BUILD_BASEGAME
	BUILD_BASEGAME =
endif
ifndef BUILD_MISSIONPACK
  BUILD_MISSIONPACK=
endif
ifndef BUILD_RENDERER_GL2
  BUILD_RENDERER_GL2 = 1
endif
ifndef BUILD_RENDERER_GL1
  BUILD_RENDERER_GL1 = 0
endif

ifneq ($(PLATFORM),darwin)
  BUILD_CLIENT_SMP = 0
endif

#########
-include makefile.local

ifndef PLATFORM
PLATFORM=$(COMPILE_PLATFORM)
endif
export PLATFORM

ifeq ($(COMPILE_ARCH),powerpc)
  COMPILE_ARCH=ppc
endif
ifeq ($(COMPILE_ARCH),powerpc64)
  COMPILE_ARCH=ppc64
endif

ifndef ARCH
ARCH=$(COMPILE_ARCH)
endif
export ARCH

ifneq ($(PLATFORM),$(COMPILE_PLATFORM))
  CROSS_COMPILING=1
else
  CROSS_COMPILING=0

  ifneq ($(ARCH),$(COMPILE_ARCH))
    CROSS_COMPILING=1
  endif
endif
export CROSS_COMPILING

ifndef VERSION
	VERSION=0.1.1
endif

ifndef CLIENTBIN
	CLIENTBIN=yantar
endif

ifndef SERVERBIN
	SERVERBIN=yantarded
endif

ifndef BASEGAME
	BASEGAME=base
endif

ifndef BASEGAME_CFLAGS
	BASEGAME_CFLAGS=
endif

ifndef MISSIONPACK
	MISSIONPACK=missionpack
endif

ifndef MISSIONPACK_CFLAGS
	MISSIONPACK_CFLAGS=-DMISSIONPACK
endif

ifndef COPYDIR
COPYDIR="/usr/local/games/quake3"
endif

ifndef COPYBINDIR
COPYBINDIR=$(COPYDIR)
endif

ifndef MOUNT_DIR
MOUNT_DIR=src
endif

ifndef BUILD_DIR
BUILD_DIR=bin
endif

ifndef TEMPDIR
TEMPDIR=/tmp
endif

ifndef GENERATE_DEPENDENCIES
GENERATE_DEPENDENCIES=1
endif

ifndef USE_OPENAL
USE_OPENAL=1
endif

ifndef USE_OPENAL_DLOPEN
USE_OPENAL_DLOPEN=1
endif

ifndef USE_CURL
USE_CURL=1
endif

ifndef USE_CURL_DLOPEN
  ifeq ($(PLATFORM),mingw32)
    USE_CURL_DLOPEN=0
  else
    USE_CURL_DLOPEN=1
  endif
endif

ifndef USE_CODEC_VORBIS
USE_CODEC_VORBIS=1
endif

ifndef USE_MUMBLE
USE_MUMBLE=0
endif

ifndef USE_VOIP
USE_VOIP=0
endif

ifndef USE_INTERNAL_SPEEX
USE_INTERNAL_SPEEX=1
endif

ifndef USE_INTERNAL_ZLIB
USE_INTERNAL_ZLIB=1
endif

ifndef USE_INTERNAL_JPEG
USE_INTERNAL_JPEG=1
endif

ifndef USE_LOCAL_HEADERS
USE_LOCAL_HEADERS=1
endif

ifndef USE_RENDERER_DLOPEN
USE_RENDERER_DLOPEN=1
endif

ifndef DEBUG_CFLAGS
DEBUG_CFLAGS=-g -O0
endif

ifndef USE_OLD_VM64
USE_OLD_VM64=0
endif

#############################################################################

BD=$(BUILD_DIR)/debug-$(PLATFORM)-$(ARCH)
BR=$(BUILD_DIR)/release-$(PLATFORM)-$(ARCH)
O=obj
CDIR=$(MOUNT_DIR)/client
SDIR=$(MOUNT_DIR)/server
RDIR=$(MOUNT_DIR)/renderer
R2DIR=$(MOUNT_DIR)/renderergl2
CMDIR=$(MOUNT_DIR)/qcommon
SDLDIR=$(MOUNT_DIR)/sdl
ASMDIR=$(MOUNT_DIR)/asm
SYSDIR=$(MOUNT_DIR)/sys
GDIR=$(MOUNT_DIR)/game
CGDIR=$(MOUNT_DIR)/cgame
BLIBDIR=$(MOUNT_DIR)/botlib
NDIR=$(MOUNT_DIR)/null
UIDIR=$(MOUNT_DIR)/ui
Q3UIDIR=$(MOUNT_DIR)/q3_ui
JPDIR=$(MOUNT_DIR)/jpeg-8c
SPEEXDIR=$(MOUNT_DIR)/libspeex
ZDIR=$(MOUNT_DIR)/zlib
Q3ASMDIR=$(MOUNT_DIR)/tools/asm
LBURGDIR=$(MOUNT_DIR)/tools/lcc/lburg
Q3CPPDIR=$(MOUNT_DIR)/tools/lcc/cpp
Q3LCCETCDIR=$(MOUNT_DIR)/tools/lcc/etc
Q3LCCSRCDIR=$(MOUNT_DIR)/tools/lcc/src
LOKISETUPDIR=misc/setup
NSISDIR=misc/nsis
SDLHDIR=$(MOUNT_DIR)/SDL12
LIBSDIR=$(MOUNT_DIR)/libs

bin_path=$(shell which $(1) 2> /dev/null)

# We won't need this if we only build the server
ifneq ($(BUILD_CLIENT),0)
  # set PKG_CONFIG_PATH to influence this, e.g.
  # PKG_CONFIG_PATH=/opt/cross/i386-mingw32msvc/lib/pkgconfig
  ifneq ($(call bin_path, pkg-config),)
    CURL_CFLAGS=$(shell pkg-config --silence-errors --cflags libcurl)
    CURL_LIBS=$(shell pkg-config --silence-errors --libs libcurl)
    OPENAL_CFLAGS=$(shell pkg-config --silence-errors --cflags openal)
    OPENAL_LIBS=$(shell pkg-config --silence-errors --libs openal)
    SDL_CFLAGS=$(shell pkg-config --silence-errors --cflags sdl|sed 's/-Dmain=SDL_main//')
    SDL_LIBS=$(shell pkg-config --silence-errors --libs sdl)
  endif
  # Use sdl-config if all else fails
  ifeq ($(SDL_CFLAGS),)
    ifneq ($(call bin_path, sdl-config),)
      SDL_CFLAGS=$(shell sdl-config --cflags)
      SDL_LIBS=$(shell sdl-config --static-libs)
    endif
  endif
endif

# Add svn version info
USE_SVN=
ifeq ($(wildcard .svn),.svn)
  SVN_REV=$(shell LANG=C svnversion .)
  ifneq ($(SVN_REV),)
    VERSION:=$(VERSION)_SVN$(SVN_REV)
    USE_SVN=1
  endif
else
ifeq ($(wildcard .git/svn/.metadata),.git/svn/.metadata)
  SVN_REV=$(shell LANG=C git svn info | awk '$$1 == "Revision:" {print $$2; exit 0}')
  ifneq ($(SVN_REV),)
    VERSION:=$(VERSION)_SVN$(SVN_REV)
  endif
endif
endif


#############################################################################
# SETUP AND BUILD -- LINUX
#############################################################################

## Defaults
LIB=lib

INSTALL=install
MKDIR=mkdir

ifneq (,$(findstring "$(PLATFORM)", "linux" "gnu_kfreebsd" "kfreebsd-gnu"))

  ifeq ($(ARCH),axp)
    ARCH=alpha
  else
  ifeq ($(ARCH),x86_64)
    LIB=lib64
  else
  ifeq ($(ARCH),ppc64)
    LIB=lib64
  else
  ifeq ($(ARCH),s390x)
    LIB=lib64
  endif
  endif
  endif
  endif

  BASE_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes \
    -pipe -DUSE_ICON
  CLIENT_CFLAGS += $(SDL_CFLAGS)

  OPTIMIZEVM = -O3 -funroll-loops -fomit-frame-pointer
  OPTIMIZE = $(OPTIMIZEVM) -ffast-math

  ifeq ($(ARCH),x86_64)
    OPTIMIZEVM = -O3 -fomit-frame-pointer -funroll-loops \
      -falign-loops=2 -falign-jumps=2 -falign-functions=2 \
      -fstrength-reduce
    OPTIMIZE = $(OPTIMIZEVM) -ffast-math
    HAVE_VM_COMPILED = true
  else
  ifeq ($(ARCH),i386)
    OPTIMIZEVM = -O3 -march=i586 -fomit-frame-pointer \
      -funroll-loops -falign-loops=2 -falign-jumps=2 \
      -falign-functions=2 -fstrength-reduce
    OPTIMIZE = $(OPTIMIZEVM) -ffast-math
    HAVE_VM_COMPILED=true
  else
  ifeq ($(ARCH),ppc)
    BASE_CFLAGS += -maltivec
    HAVE_VM_COMPILED=true
  endif
  ifeq ($(ARCH),ppc64)
    BASE_CFLAGS += -maltivec
    HAVE_VM_COMPILED=true
  endif
  ifeq ($(ARCH),sparc)
    OPTIMIZE += -mtune=ultrasparc3 -mv8plus
    OPTIMIZEVM += -mtune=ultrasparc3 -mv8plus
    HAVE_VM_COMPILED=true
  endif
  ifeq ($(ARCH),alpha)
    # According to http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=410555
    # -ffast-math will cause the client to die with SIGFPE on Alpha
    OPTIMIZE = $(OPTIMIZEVM)
  endif
  endif
  endif

  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC -fvisibility=hidden
  SHLIBLDFLAGS=-shared $(LDFLAGS)

  THREAD_LIBS=-lpthread
  LIBS=-ldl -lm

  CLIENT_LIBS=$(SDL_LIBS)
  RENDERER_LIBS = $(SDL_LIBS) -lGL

  ifeq ($(USE_OPENAL),1)
    ifneq ($(USE_OPENAL_DLOPEN),1)
      CLIENT_LIBS += -lopenal
    endif
  endif

  ifeq ($(USE_CURL),1)
    ifneq ($(USE_CURL_DLOPEN),1)
      CLIENT_LIBS += -lcurl
    endif
  endif

  ifeq ($(USE_CODEC_VORBIS),1)
    CLIENT_LIBS += -lvorbisfile -lvorbis -logg
  endif

  ifeq ($(USE_MUMBLE),1)
    CLIENT_LIBS += -lrt
  endif

  ifeq ($(USE_LOCAL_HEADERS),1)
    CLIENT_CFLAGS += -I$(SDLHDIR)/include
  endif

  ifeq ($(ARCH),i386)
    # linux32 make ...
    BASE_CFLAGS += -m32
  else
  ifeq ($(ARCH),ppc64)
    BASE_CFLAGS += -m64
  endif
  endif
else # ifeq Linux

#############################################################################
# SETUP AND BUILD -- MAC OS X
#############################################################################

ifeq ($(PLATFORM),darwin)
  HAVE_VM_COMPILED=true
  LIBS = -framework Cocoa
  CLIENT_LIBS=
  RENDERER_LIBS=
  OPTIMIZEVM=
  
  BASE_CFLAGS = -Wall -Wimplicit -Wstrict-prototypes

  ifeq ($(ARCH),ppc)
    BASE_CFLAGS += -arch ppc -faltivec -mmacosx-version-min=10.2
    OPTIMIZEVM += -O3
  endif
  ifeq ($(ARCH),ppc64)
    BASE_CFLAGS += -arch ppc64 -faltivec -mmacosx-version-min=10.2
  endif
  ifeq ($(ARCH),i386)
    OPTIMIZEVM += -march=prescott -mfpmath=sse
    # x86 vm will crash without -mstackrealign since MMX instructions will be
    # used no matter what and they corrupt the frame pointer in VM calls
    BASE_CFLAGS += -arch i386 -m32 -mstackrealign
  endif
  ifeq ($(ARCH),x86_64)
    OPTIMIZEVM += -arch x86_64 -mfpmath=sse
  endif

  BASE_CFLAGS += -fno-strict-aliasing -DMACOS_X -fno-common -pipe

  ifeq ($(USE_OPENAL),1)
    ifneq ($(USE_OPENAL_DLOPEN),1)
      CLIENT_LIBS += -framework OpenAL
    endif
  endif

  ifeq ($(USE_CURL),1)
    ifneq ($(USE_CURL_DLOPEN),1)
      CLIENT_LIBS += -lcurl
    endif
  endif

  ifeq ($(USE_CODEC_VORBIS),1)
    CLIENT_LIBS += -lvorbisfile -lvorbis -logg
  endif

  BASE_CFLAGS += -D_THREAD_SAFE=1

  ifeq ($(USE_LOCAL_HEADERS),1)
    BASE_CFLAGS += -I$(SDLHDIR)/include
  endif

  # We copy sdlmain before ranlib'ing it so that subversion doesn't think
  #  the file has been modified by each build.
  LIBSDLMAIN=$(B)/libSDLmain.a
  LIBSDLMAINSRC=$(LIBSDIR)/macosx/libSDLmain.a
  CLIENT_LIBS += -framework IOKit \
    $(LIBSDIR)/macosx/libSDL-1.2.0.dylib
  RENDERER_LIBS += -framework OpenGL $(LIBSDIR)/macosx/libSDL-1.2.0.dylib

  OPTIMIZEVM += -falign-loops=16
  OPTIMIZE = $(OPTIMIZEVM) -ffast-math

  SHLIBEXT=dylib
  SHLIBCFLAGS=-fPIC -fno-common
  SHLIBLDFLAGS=-dynamiclib $(LDFLAGS) -Wl,-U,_com_altivec

  NOTSHLIBCFLAGS=-mdynamic-no-pic

  TOOLS_CFLAGS += -DMACOS_X

else # ifeq darwin


#############################################################################
# SETUP AND BUILD -- MINGW32
#############################################################################

ifeq ($(PLATFORM),mingw32)

  # Some MinGW installations define CC to cc, but don't actually provide cc,
  # so explicitly use gcc instead (which is the only option anyway)
  ifeq ($(call bin_path, $(CC)),)
    CC=gcc
  endif

  ifndef WINDRES
    WINDRES=windres
  endif

  # Give windres a target flag
  ifeq ($(COMPILE_ARCH),x86)
    WINDRES_FLAGS=-Fpe-i386
  else
    ifeq ($(COMPILE_ARCH),x64)
      WINDRES_FLAGS=-Fpe-x86-64
    else
      WINDRES_FLAGS=
    endif
  endif

  BASE_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes \
    -DUSE_ICON

  # In the absence of wspiapi.h, require Windows XP or later
  ifeq ($(shell test -e $(CMDIR)/wspiapi.h; echo $$?),1)
    BASE_CFLAGS += -DWINVER=0x501
  endif

  ifeq ($(USE_OPENAL),1)
    CLIENT_CFLAGS += $(OPENAL_CFLAGS)
    ifneq ($(USE_OPENAL_DLOPEN),1)
      CLIENT_LDFLAGS += $(OPENAL_LDFLAGS)
    endif
  endif
  
  ifeq ($(ARCH),x64)
    OPTIMIZEVM = -O3 -fno-omit-frame-pointer \
      -falign-loops=2 -funroll-loops -falign-jumps=2 -falign-functions=2 \
      -fstrength-reduce
    OPTIMIZE = $(OPTIMIZEVM) --fast-math
    HAVE_VM_COMPILED = true
  endif
  ifeq ($(ARCH),x86)
    OPTIMIZEVM = -O3 -march=i586 -fno-omit-frame-pointer \
      -falign-loops=2 -funroll-loops -falign-jumps=2 -falign-functions=2 \
      -fstrength-reduce
    OPTIMIZE = $(OPTIMIZEVM) -ffast-math
    HAVE_VM_COMPILED = true
  endif

  SHLIBEXT=dll
  SHLIBCFLAGS=
  SHLIBLDFLAGS=-shared $(LDFLAGS)

  BINEXT=.exe

  LIBS= -lws2_32 -lwinmm -lpsapi
  CLIENT_LDFLAGS += -mwindows
  CLIENT_LIBS = -lgdi32 -lole32
  RENDERER_LIBS = -lgdi32 -lole32 -lopengl32 -lfreetype
  
  ifeq ($(USE_CURL),1)
    CLIENT_CFLAGS += $(CURL_CFLAGS)
    ifneq ($(USE_CURL_DLOPEN),1)
      ifeq ($(USE_LOCAL_HEADERS),1)
        CLIENT_CFLAGS += -DCURL_STATICLIB
        ifeq ($(ARCH),x64)
          CLIENT_LIBS += $(LIBSDIR)/win64/libcurl.a
        else
          CLIENT_LIBS += $(LIBSDIR)/win32/libcurl.a
        endif
      else
        CLIENT_LIBS += $(CURL_LIBS)
      endif
    endif
  endif

  ifeq ($(USE_CODEC_VORBIS),1)
    CLIENT_LIBS += -lvorbisfile -lvorbis -logg
  endif

  ifeq ($(ARCH),x86)
    # build 32bit
    BASE_CFLAGS += -m32
  else
    BASE_CFLAGS += -m64
  endif

  # libmingw32 must be linked before libSDLmain
  CLIENT_LIBS += -lmingw32
  RENDERER_LIBS += -lmingw32
  
  ifeq ($(USE_LOCAL_HEADERS),1)
    CLIENT_CFLAGS += -I$(SDLHDIR)/include
    ifeq ($(ARCH), x86)
    CLIENT_LIBS += $(LIBSDIR)/win32/libSDLmain.a \
                      $(LIBSDIR)/win32/libSDL.dll.a
    RENDERER_LIBS += $(LIBSDIR)/win32/libSDLmain.a \
                      $(LIBSDIR)/win32/libSDL.dll.a
    else
    CLIENT_LIBS += $(LIBSDIR)/win64/libSDLmain.a \
                      $(LIBSDIR)/win64/libSDL64.dll.a
    RENDERER_LIBS += $(LIBSDIR)/win64/libSDLmain.a \
                      $(LIBSDIR)/win64/libSDL64.dll.a
    endif
  else
    CLIENT_CFLAGS += $(SDL_CFLAGS)
    CLIENT_LIBS += $(SDL_LIBS)
    RENDERER_LIBS += $(SDL_LIBS)
  endif

  BUILD_CLIENT_SMP = 0

else # ifeq mingw32

#############################################################################
# SETUP AND BUILD -- FREEBSD
#############################################################################

ifeq ($(PLATFORM),freebsd)

  # flags
  BASE_CFLAGS = $(shell env MACHINE_ARCH=$(ARCH) make -f /dev/null -VCFLAGS) \
    -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes \
    -DUSE_ICON -DMAP_ANONYMOUS=MAP_ANON
  CLIENT_CFLAGS += $(SDL_CFLAGS)
  HAVE_VM_COMPILED = true

  OPTIMIZEVM = -O3 -funroll-loops -fomit-frame-pointer
  OPTIMIZE = $(OPTIMIZEVM) -ffast-math

  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-shared $(LDFLAGS)

  THREAD_LIBS=-lpthread
  # don't need -ldl (FreeBSD)
  LIBS=-lm

  CLIENT_LIBS =

  CLIENT_LIBS += $(SDL_LIBS)
  RENDERER_LIBS = $(SDL_LIBS) -lGL

  # optional features/libraries
  ifeq ($(USE_OPENAL),1)
    ifeq ($(USE_OPENAL_DLOPEN),1)
      CLIENT_LIBS += $(THREAD_LIBS) -lopenal
    endif
  endif

  ifeq ($(USE_CURL),1)
    ifeq ($(USE_CURL_DLOPEN),1)
      CLIENT_LIBS += -lcurl
    endif
  endif

  ifeq ($(USE_CODEC_VORBIS),1)
    CLIENT_LIBS += -lvorbisfile -lvorbis -logg
  endif

  # cross-compiling tweaks
  ifeq ($(ARCH),i386)
    ifeq ($(CROSS_COMPILING),1)
      BASE_CFLAGS += -m32
    endif
  endif
  ifeq ($(ARCH),amd64)
    ifeq ($(CROSS_COMPILING),1)
      BASE_CFLAGS += -m64
    endif
  endif

else # ifeq freebsd

#############################################################################
# SETUP AND BUILD -- OPENBSD
#############################################################################

ifeq ($(PLATFORM),openbsd)

  ARCH=$(shell uname -m)

  BASE_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes \
    -DUSE_ICON -DMAP_ANONYMOUS=MAP_ANON
  CLIENT_CFLAGS += $(SDL_CFLAGS)

  ifeq ($(USE_CURL),1)
    CLIENT_CFLAGS += $(CURL_CFLAGS)
    USE_CURL_DLOPEN=0
  endif

  SHLIBEXT=so
  SHLIBNAME=.$(SHLIBEXT)
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-shared $(LDFLAGS)

  THREAD_LIBS=-pthread
  LIBS=-lm

  CLIENT_LIBS =

  CLIENT_LIBS += $(SDL_LIBS)
  RENDERER_LIBS = $(SDL_LIBS) -lGL

  ifeq ($(USE_OPENAL),1)
    ifneq ($(USE_OPENAL_DLOPEN),1)
      CLIENT_LIBS += $(THREAD_LIBS) -lossaudio -lopenal
    endif
  endif

  ifeq ($(USE_CODEC_VORBIS),1)
    CLIENT_LIBS += -lvorbisfile -lvorbis -logg
  endif

  ifeq ($(USE_CURL),1) 
    ifneq ($(USE_CURL_DLOPEN),1)
      CLIENT_LIBS += -lcurl
    endif
  endif

else # ifeq openbsd

#############################################################################
# SETUP AND BUILD -- NETBSD
#############################################################################

ifeq ($(PLATFORM),netbsd)

  ifeq ($(shell uname -m),i386)
    ARCH=i386
  endif

  LIBS=-lm
  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-shared $(LDFLAGS)
  THREAD_LIBS=-lpthread

  BASE_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes

  ifeq ($(ARCH),i386)
    HAVE_VM_COMPILED=true
  endif

  BUILD_CLIENT = 0

else # ifeq netbsd

#############################################################################
# SETUP AND BUILD -- IRIX
#############################################################################

ifeq ($(PLATFORM),irix64)

  ARCH=mips

  CC = c99
  MKDIR = mkdir -p

  BASE_CFLAGS=-Dstricmp=strcasecmp -Xcpluscomm -woff 1185 \
    -I. -I$(ROOT)/usr/include
  CLIENT_CFLAGS += $(SDL_CFLAGS)
  OPTIMIZE = -O3
  
  SHLIBEXT=so
  SHLIBCFLAGS=
  SHLIBLDFLAGS=-shared

  LIBS=-ldl -lm -lgen
  # FIXME: The X libraries probably aren't necessary?
  CLIENT_LIBS=-L/usr/X11/$(LIB) $(SDL_LIBS) \
    -lX11 -lXext -lm
  RENDERER_LIBS = $(SDL_LIBS) -lGL

else # ifeq IRIX

#############################################################################
# SETUP AND BUILD -- SunOS
#############################################################################

ifeq ($(PLATFORM),sunos)

  CC=gcc
  INSTALL=ginstall
  MKDIR=gmkdir
  COPYDIR="/usr/local/share/games/quake3"

  ifneq (,$(findstring i86pc,$(shell uname -m)))
    ARCH=i386
  else #default to sparc
    ARCH=sparc
  endif

  ifneq ($(ARCH),i386)
    ifneq ($(ARCH),sparc)
      $(error arch $(ARCH) is currently not supported)
    endif
  endif

  BASE_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes \
    -pipe -DUSE_ICON
  CLIENT_CFLAGS += $(SDL_CFLAGS)

  OPTIMIZEVM = -O3 -funroll-loops

  ifeq ($(ARCH),sparc)
    OPTIMIZEVM += -O3 \
      -fstrength-reduce -falign-functions=2 \
      -mtune=ultrasparc3 -mv8plus -mno-faster-structs
    HAVE_VM_COMPILED=true
  else
  ifeq ($(ARCH),i386)
    OPTIMIZEVM += -march=i586 -fomit-frame-pointer \
      -falign-loops=2 -falign-jumps=2 \
      -falign-functions=2 -fstrength-reduce
    HAVE_VM_COMPILED=true
    BASE_CFLAGS += -m32
    CLIENT_CFLAGS += -I/usr/X11/include/NVIDIA
    CLIENT_LDFLAGS += -L/usr/X11/lib/NVIDIA -R/usr/X11/lib/NVIDIA
  endif
  endif
  
  OPTIMIZE = $(OPTIMIZEVM) -ffast-math

  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-shared $(LDFLAGS)

  THREAD_LIBS=-lpthread
  LIBS=-lsocket -lnsl -ldl -lm

  BOTCFLAGS=-O0

  CLIENT_LIBS +=$(SDL_LIBS) -lX11 -lXext -liconv -lm
  RENDERER_LIBS = $(SDL_LIBS) -lGL

else # ifeq sunos

#############################################################################
# SETUP AND BUILD -- GENERIC
#############################################################################
  BASE_CFLAGS=
  OPTIMIZE = -O3

  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-shared

endif #Linux
endif #darwin
endif #mingw32
endif #FreeBSD
endif #OpenBSD
endif #NetBSD
endif #IRIX
endif #SunOS

ifneq ($(HAVE_VM_COMPILED),true)
  BASE_CFLAGS += -DNO_VM_COMPILED
  BUILD_GAME_QVM=0
endif

TARGETS =

ifndef FULLBINEXT
  FULLBINEXT=.$(ARCH)$(BINEXT)
endif

ifndef SHLIBNAME
  SHLIBNAME=$(ARCH).$(SHLIBEXT)
endif

ifneq ($(BUILD_SERVER),0)
  TARGETS += $(B)/$(SERVERBIN)$(FULLBINEXT)
endif

ifneq ($(BUILD_CLIENT),0)
  ifneq ($(USE_RENDERER_DLOPEN),0)
    TARGETS += $(B)/$(CLIENTBIN)$(FULLBINEXT)
    ifneq ($(BUILD_RENDERER_GL1),0)
      TARGETS += $(B)/renderer_opengl1_$(SHLIBNAME)
        ifneq ($(BUILD_CLIENT_SMP),0)
          TARGETS += $(B)/renderer_opengl1_smp_$(SHLIBNAME)
        endif
    endif
    ifneq ($(BUILD_RENDERER_GL2), 0)
      TARGETS += $(B)/renderer_opengl2_$(SHLIBNAME)
      ifneq ($(BUILD_CLIENT_SMP),0)
        TARGETS += $(B)/renderer_opengl2_smp_$(SHLIBNAME)
      endif
    endif
  else
    TARGETS += $(B)/$(CLIENTBIN)$(FULLBINEXT)
    ifneq ($(BUILD_CLIENT_SMP),0)
      TARGETS += $(B)/$(CLIENTBIN)-smp$(FULLBINEXT)
    endif
  endif
endif

ifneq ($(BUILD_GAME_SO),0)
  TARGETS += \
    $(B)/$(BASEGAME)/cgame$(SHLIBNAME) \
    $(B)/$(BASEGAME)/qagame$(SHLIBNAME) \
    $(B)/$(BASEGAME)/ui$(SHLIBNAME)
  ifneq ($(BUILD_MISSIONPACK),0)
    TARGETS += \
    $(B)/$(MISSIONPACK)/cgame$(SHLIBNAME) \
    $(B)/$(MISSIONPACK)/qagame$(SHLIBNAME) \
    $(B)/$(MISSIONPACK)/ui$(SHLIBNAME)
  endif
endif

ifneq ($(BUILD_GAME_QVM),0)
  ifneq ($(CROSS_COMPILING),1)
    TARGETS += \
      $(B)/$(BASEGAME)/vm/cgame.qvm \
      $(B)/$(BASEGAME)/vm/qagame.qvm \
      $(B)/$(BASEGAME)/vm/ui.qvm
    ifneq ($(BUILD_MISSIONPACK),0)
      TARGETS += \
      $(B)/$(MISSIONPACK)/vm/qagame.qvm \
      $(B)/$(MISSIONPACK)/vm/cgame.qvm \
      $(B)/$(MISSIONPACK)/vm/ui.qvm
    endif
  endif
endif

ifeq ($(USE_OPENAL),1)
  CLIENT_CFLAGS += -DUSE_OPENAL
  ifeq ($(USE_OPENAL_DLOPEN),1)
    CLIENT_CFLAGS += -DUSE_OPENAL_DLOPEN
  endif
endif

ifeq ($(USE_CURL),1)
  CLIENT_CFLAGS += -DUSE_CURL
  ifeq ($(USE_CURL_DLOPEN),1)
    CLIENT_CFLAGS += -DUSE_CURL_DLOPEN
  endif
endif

ifeq ($(USE_CODEC_VORBIS),1)
  CLIENT_CFLAGS += -DUSE_CODEC_VORBIS
endif

ifeq ($(USE_RENDERER_DLOPEN),1)
  CLIENT_CFLAGS += -DUSE_RENDERER_DLOPEN
endif

ifeq ($(USE_MUMBLE),1)
  CLIENT_CFLAGS += -DUSE_MUMBLE
endif

ifeq ($(USE_VOIP),1)
  CLIENT_CFLAGS += -DUSE_VOIP
  SERVER_CFLAGS += -DUSE_VOIP
  ifeq ($(USE_INTERNAL_SPEEX),1)
    CLIENT_CFLAGS += -DFLOATING_POINT -DUSE_ALLOCA -I$(SPEEXDIR)/include
  else
    CLIENT_LIBS += -lspeex -lspeexdsp
  endif
endif

ifeq ($(USE_INTERNAL_ZLIB),1)
  BASE_CFLAGS += -DNO_GZIP
  BASE_CFLAGS += -I$(ZDIR)
else
  LIBS += -lz
endif

ifeq ($(USE_INTERNAL_JPEG),1)
  BASE_CFLAGS += -DUSE_INTERNAL_JPEG
  BASE_CFLAGS += -I$(JPDIR)
else
  RENDERER_LIBS += -ljpeg
endif

ifeq ("$(CC)", $(findstring "$(CC)", "clang" "clang++"))
	BASE_CFLAGS += -Qunused-arguments
endif

ifdef DEFAULT_BASEDIR
  BASE_CFLAGS += -DDEFAULT_BASEDIR=\\\"$(DEFAULT_BASEDIR)\\\"
endif

ifeq ($(USE_LOCAL_HEADERS),1)
  BASE_CFLAGS += -DUSE_LOCAL_HEADERS
endif

ifeq ($(BUILD_STANDALONE),1)
  BASE_CFLAGS += -DSTANDALONE
endif

ifeq ($(GENERATE_DEPENDENCIES),1)
  DEPEND_CFLAGS = -MMD
else
  DEPEND_CFLAGS =
endif

ifeq ($(NO_STRIP),1)
  STRIP_FLAG =
else
  STRIP_FLAG = -s
endif

BASE_CFLAGS += -DPRODUCT_VERSION=\\\"$(VERSION)\\\"
BASE_CFLAGS += -Wformat=2 -Wno-format-zero-length -Wformat-security -Wno-format-nonliteral
BASE_CFLAGS += -Wstrict-aliasing=2 -Wmissing-format-attribute
BASE_CFLAGS += -Wdisabled-optimization
BASE_CFLAGS += -Werror-implicit-function-declaration

ifeq ($(V),1)
echo_cmd=@:
Q=
else
echo_cmd=@echo
Q=@
endif

define DO_CC
$(echo_cmd) "CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(OPTIMIZE) -o $@ -c $<
endef

define DO_REF_CC
$(echo_cmd) "REF_CC $<"
$(Q)$(CC) $(SHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(OPTIMIZE) -o $@ -c $<
endef

define DO_SMP_CC
$(echo_cmd) "SMP_CC $<"
$(Q)$(CC) $(SHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(OPTIMIZE) -DSMP -o $@ -c $<
endef

define DO_BOT_CC
$(echo_cmd) "BOT_CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) $(CFLAGS) $(BOTCFLAGS) $(OPTIMIZE) -DBOTLIB -o $@ -c $<
endef

ifeq ($(GENERATE_DEPENDENCIES),1)
  DO_QVM_DEP=cat $(@:%.o=%.d) | sed -e 's/\.o/\.asm/g' >> $(@:%.o=%.d)
endif

define DO_SHLIB_CC
$(echo_cmd) "SHLIB_CC $<"
$(Q)$(CC) $(BASEGAME_CFLAGS) $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_GAME_CC
$(echo_cmd) "GAME_CC $<"
$(Q)$(CC) $(BASEGAME_CFLAGS) -DQAGAME $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_CGAME_CC
$(echo_cmd) "CGAME_CC $<"
$(Q)$(CC) $(BASEGAME_CFLAGS) -DCGAME $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_UI_CC
$(echo_cmd) "UI_CC $<"
$(Q)$(CC) $(BASEGAME_CFLAGS) -DUI $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_SHLIB_CC_MISSIONPACK
$(echo_cmd) "SHLIB_CC_MISSIONPACK $<"
$(Q)$(CC) $(MISSIONPACK_CFLAGS) $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_GAME_CC_MISSIONPACK
$(echo_cmd) "GAME_CC_MISSIONPACK $<"
$(Q)$(CC) $(MISSIONPACK_CFLAGS) -DQAGAME $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_CGAME_CC_MISSIONPACK
$(echo_cmd) "CGAME_CC_MISSIONPACK $<"
$(Q)$(CC) $(MISSIONPACK_CFLAGS) -DCGAME $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_UI_CC_MISSIONPACK
$(echo_cmd) "UI_CC_MISSIONPACK $<"
$(Q)$(CC) $(MISSIONPACK_CFLAGS) -DUI $(SHLIBCFLAGS) $(CFLAGS) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_AS
$(echo_cmd) "AS $<"
$(Q)$(CC) $(CFLAGS) $(OPTIMIZE) -x assembler-with-cpp -o $@ -c $<
endef

define DO_DED_CC
$(echo_cmd) "DED_CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) -DDEDICATED $(CFLAGS) $(SERVER_CFLAGS) $(OPTIMIZE) -o $@ -c $<
endef

define DO_WINDRES
$(echo_cmd) "WINDRES $<"
$(Q)$(WINDRES) $(WINDRES_FLAGS) -i $< -o $@
endef


#############################################################################
# MAIN TARGETS
#############################################################################

default: release
all: debug release

debug:
	@$(MAKE) targets B=$(BD) O=$(BD)/obj CFLAGS="$(CFLAGS) $(BASE_CFLAGS) $(DEPEND_CFLAGS)" \
	  OPTIMIZE="$(DEBUG_CFLAGS)" OPTIMIZEVM="$(DEBUG_CFLAGS)" \
	  CLIENT_CFLAGS="$(CLIENT_CFLAGS)" SERVER_CFLAGS="$(SERVER_CFLAGS)" V=$(V)

release:
	@$(MAKE) targets B=$(BR) O=$(BR)/obj CFLAGS="$(CFLAGS) $(BASE_CFLAGS) $(DEPEND_CFLAGS)" \
	  OPTIMIZE="-DNDEBUG $(OPTIMIZE)" OPTIMIZEVM="-DNDEBUG $(OPTIMIZEVM)" \
	  CLIENT_CFLAGS="$(CLIENT_CFLAGS)" SERVER_CFLAGS="$(SERVER_CFLAGS)" V=$(V)

# Create the build directories, check libraries and print out
# an informational message, then start building
targets: makedirs
	@echo ""
	@echo "Building $(CLIENTBIN) in $(B):"
	@echo "  PLATFORM: $(PLATFORM)"
	@echo "  ARCH: $(ARCH)"
	@echo "  VERSION: $(VERSION)"
	@echo "  COMPILE_PLATFORM: $(COMPILE_PLATFORM)"
	@echo "  COMPILE_ARCH: $(COMPILE_ARCH)"
	@echo "  CC: $(CC)"
	@echo ""
	@echo "  CFLAGS:"
	-@for i in $(CFLAGS); \
	do \
		echo "    $$i"; \
	done
	-@for i in $(OPTIMIZE); \
	do \
		echo "    $$i"; \
	done
	@echo ""
	@echo "  CLIENT_CFLAGS:"
	-@for i in $(CLIENT_CFLAGS); \
	do \
		echo "    $$i"; \
	done
	@echo ""
	@echo "  SERVER_CFLAGS:"
	-@for i in $(SERVER_CFLAGS); \
	do \
		echo "    $$i"; \
	done
	@echo ""
	@echo "  LDFLAGS:"
	-@for i in $(LDFLAGS); \
	do \
		echo "    $$i"; \
	done
	@echo ""
	@echo "  LIBS:"
	-@for i in $(LIBS); \
	do \
		echo "    $$i"; \
	done
	@echo ""
	@echo "  CLIENT_LIBS:"
	-@for i in $(CLIENT_LIBS); \
	do \
		echo "    $$i"; \
	done
	@echo ""
	@echo "  Output:"
	-@for i in $(TARGETS); \
	do \
		echo "    $$i"; \
	done
	@echo ""
ifneq ($(TARGETS),)
	@$(MAKE) $(TARGETS) V=$(V)
endif

makedirs:
	@if [ ! -d $(BUILD_DIR) ];then $(MKDIR) $(BUILD_DIR);fi
	@if [ ! -d $(B) ];then $(MKDIR) $(B);fi
	@if [ ! -d $(O) ];then $(MKDIR) $(O);fi
	@if [ ! -d $(O)/client ];then $(MKDIR) $(O)/client;fi
	@if [ ! -d $(O)/renderer ];then $(MKDIR) $(O)/renderer;fi
	@if [ ! -d $(O)/renderergl2 ];then $(MKDIR) $(O)/renderergl2;fi
	@if [ ! -d $(O)/renderersmp ];then $(MKDIR) $(O)/renderersmp;fi
	@if [ ! -d $(O)/ded ];then $(MKDIR) $(O)/ded;fi
	@if [ ! -d $(B)/$(BASEGAME) ];then $(MKDIR) $(B)/$(BASEGAME);fi
	@if [ ! -d $(O)/$(BASEGAME) ];then $(MKDIR) $(O)/$(BASEGAME);fi
	@if [ ! -d $(O)/$(BASEGAME)/cgame ];then $(MKDIR) $(O)/$(BASEGAME)/cgame;fi
	@if [ ! -d $(O)/$(BASEGAME)/game ];then $(MKDIR) $(O)/$(BASEGAME)/game;fi
	@if [ ! -d $(O)/$(BASEGAME)/ui ];then $(MKDIR) $(O)/$(BASEGAME)/ui;fi
	@if [ ! -d $(O)/$(BASEGAME)/qcommon ];then $(MKDIR) $(O)/$(BASEGAME)/qcommon;fi
	@if [ ! -d $(B)/$(BASEGAME)/vm ];then $(MKDIR) $(B)/$(BASEGAME)/vm;fi
	@if [ ! -d $(O)/$(BASEGAME)/vm ];then $(MKDIR) $(O)/$(BASEGAME)/vm;fi
	@if [ ! -d $(B)/$(MISSIONPACK) ];then $(MKDIR) $(B)/$(MISSIONPACK);fi
	@if [ ! -d $(O)/$(MISSIONPACK) ];then $(MKDIR) $(O)/$(MISSIONPACK);fi
	@if [ ! -d $(O)/$(MISSIONPACK)/cgame ];then $(MKDIR) $(O)/$(MISSIONPACK)/cgame;fi
	@if [ ! -d $(O)/$(MISSIONPACK)/game ];then $(MKDIR) $(O)/$(MISSIONPACK)/game;fi
	@if [ ! -d $(O)/$(MISSIONPACK)/ui ];then $(MKDIR) $(O)/$(MISSIONPACK)/ui;fi
	@if [ ! -d $(O)/$(MISSIONPACK)/qcommon ];then $(MKDIR) $(O)/$(MISSIONPACK)/qcommon;fi
	@if [ ! -d $(B)/$(MISSIONPACK)/vm ];then $(MKDIR) $(B)/$(MISSIONPACK)/vm;fi
	@if [ ! -d $(O)/$(MISSIONPACK)/vm ];then $(MKDIR) $(O)/$(MISSIONPACK)/vm;fi
	@if [ ! -d $(B)/tools ];then $(MKDIR) $(B)/tools;fi
	@if [ ! -d $(O)/tools ];then $(MKDIR) $(O)/tools;fi
	@if [ ! -d $(B)/tools/asm ];then $(MKDIR) $(B)/tools/asm;fi
	@if [ ! -d $(O)/tools/asm ];then $(MKDIR) $(O)/tools/asm;fi
	@if [ ! -d $(B)/tools/etc ];then $(MKDIR) $(B)/tools/etc;fi
	@if [ ! -d $(O)/tools/etc ];then $(MKDIR) $(O)/tools/etc;fi
	@if [ ! -d $(B)/tools/rcc ];then $(MKDIR) $(B)/tools/rcc;fi
	@if [ ! -d $(O)/tools/rcc ];then $(MKDIR) $(O)/tools/rcc;fi
	@if [ ! -d $(B)/tools/cpp ];then $(MKDIR) $(B)/tools/cpp;fi
	@if [ ! -d $(O)/tools/cpp ];then $(MKDIR) $(O)/tools/cpp;fi
	@if [ ! -d $(B)/tools/lburg ];then $(MKDIR) $(B)/tools/lburg;fi
	@if [ ! -d $(O)/tools/lburg ];then $(MKDIR) $(O)/tools/lburg;fi

#############################################################################
# QVM BUILD TOOLS
#############################################################################

TOOLS_OPTIMIZE = -g -Wall -fno-strict-aliasing
TOOLS_CFLAGS += $(TOOLS_OPTIMIZE) \
                -DTEMPDIR=\"$(TEMPDIR)\" -DSYSTEM=\"\" \
                -I$(Q3LCCSRCDIR) \
                -I$(LBURGDIR)
TOOLS_LIBS =
TOOLS_LDFLAGS =

ifeq ($(GENERATE_DEPENDENCIES),1)
	TOOLS_CFLAGS += -MMD
endif

define DO_TOOLS_CC
$(echo_cmd) "TOOLS_CC $<"
$(Q)$(CC) $(TOOLS_CFLAGS) -o $@ -c $<
endef

define DO_TOOLS_CC_DAGCHECK
$(echo_cmd) "TOOLS_CC_DAGCHECK $<"
$(Q)$(CC) $(TOOLS_CFLAGS) -Wno-unused -o $@ -c $<
endef

LBURG       = $(B)/tools/lburg/lburg$(BINEXT)
DAGCHECK_C  = $(B)/tools/rcc/dagcheck.c
Q3RCC       = $(B)/tools/q3rcc$(BINEXT)
Q3CPP       = $(B)/tools/q3cpp$(BINEXT)
Q3LCC       = $(B)/tools/q3lcc$(BINEXT)
Q3ASM       = $(B)/tools/q3asm$(BINEXT)

LBURGOBJ= \
	$(O)/tools/lburg/lburg.o \
	$(O)/tools/lburg/gram.o

$(O)/tools/lburg/%.o: $(LBURGDIR)/%.c
	$(DO_TOOLS_CC)

$(LBURG): $(LBURGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)

Q3RCCOBJ = \
  $(O)/tools/rcc/alloc.o \
  $(O)/tools/rcc/bind.o \
  $(O)/tools/rcc/bytecode.o \
  $(O)/tools/rcc/dag.o \
  $(O)/tools/rcc/dagcheck.o \
  $(O)/tools/rcc/decl.o \
  $(O)/tools/rcc/enode.o \
  $(O)/tools/rcc/error.o \
  $(O)/tools/rcc/event.o \
  $(O)/tools/rcc/expr.o \
  $(O)/tools/rcc/gen.o \
  $(O)/tools/rcc/init.o \
  $(O)/tools/rcc/inits.o \
  $(O)/tools/rcc/input.o \
  $(O)/tools/rcc/lex.o \
  $(O)/tools/rcc/list.o \
  $(O)/tools/rcc/main.o \
  $(O)/tools/rcc/null.o \
  $(O)/tools/rcc/output.o \
  $(O)/tools/rcc/prof.o \
  $(O)/tools/rcc/profio.o \
  $(O)/tools/rcc/simp.o \
  $(O)/tools/rcc/stmt.o \
  $(O)/tools/rcc/string.o \
  $(O)/tools/rcc/sym.o \
  $(O)/tools/rcc/symbolic.o \
  $(O)/tools/rcc/trace.o \
  $(O)/tools/rcc/tree.o \
  $(O)/tools/rcc/types.o

$(DAGCHECK_C): $(LBURG) $(Q3LCCSRCDIR)/dagcheck.md
	$(echo_cmd) "LBURG $(Q3LCCSRCDIR)/dagcheck.md"
	$(Q)$(LBURG) $(Q3LCCSRCDIR)/dagcheck.md $@

$(O)/tools/rcc/dagcheck.o: $(DAGCHECK_C)
	$(DO_TOOLS_CC_DAGCHECK)

$(O)/tools/rcc/%.o: $(Q3LCCSRCDIR)/%.c
	$(DO_TOOLS_CC)

$(Q3RCC): $(Q3RCCOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)

Q3CPPOBJ = \
	$(O)/tools/cpp/cpp.o \
	$(O)/tools/cpp/lex.o \
	$(O)/tools/cpp/nlist.o \
	$(O)/tools/cpp/tokens.o \
	$(O)/tools/cpp/macro.o \
	$(O)/tools/cpp/eval.o \
	$(O)/tools/cpp/include.o \
	$(O)/tools/cpp/hideset.o \
	$(O)/tools/cpp/getopt.o \
	$(O)/tools/cpp/unix.o

$(O)/tools/cpp/%.o: $(Q3CPPDIR)/%.c
	$(DO_TOOLS_CC)

$(Q3CPP): $(Q3CPPOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)

Q3LCCOBJ = \
	$(O)/tools/etc/lcc.o \
	$(O)/tools/etc/bytecode.o

$(O)/tools/etc/%.o: $(Q3LCCETCDIR)/%.c
	$(DO_TOOLS_CC)

$(Q3LCC): $(Q3LCCOBJ) $(Q3RCC) $(Q3CPP)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $(Q3LCCOBJ) $(TOOLS_LIBS)

define DO_Q3LCC
$(echo_cmd) "Q3LCC $<"
$(Q)$(Q3LCC) $(BASEGAME_CFLAGS) -o $@ $<
endef

define DO_CGAME_Q3LCC
$(echo_cmd) "CGAME_Q3LCC $<"
$(Q)$(Q3LCC) $(BASEGAME_CFLAGS) -DCGAME -o $@ $<
endef

define DO_GAME_Q3LCC
$(echo_cmd) "GAME_Q3LCC $<"
$(Q)$(Q3LCC) $(BASEGAME_CFLAGS) -DQAGAME -o $@ $<
endef

define DO_UI_Q3LCC
$(echo_cmd) "UI_Q3LCC $<"
$(Q)$(Q3LCC) $(BASEGAME_CFLAGS) -DUI -o $@ $<
endef

define DO_Q3LCC_MISSIONPACK
$(echo_cmd) "Q3LCC_MISSIONPACK $<"
$(Q)$(Q3LCC) $(MISSIONPACK_CFLAGS) -o $@ $<
endef

define DO_CGAME_Q3LCC_MISSIONPACK
$(echo_cmd) "CGAME_Q3LCC_MISSIONPACK $<"
$(Q)$(Q3LCC) $(MISSIONPACK_CFLAGS) -DCGAME -o $@ $<
endef

define DO_GAME_Q3LCC_MISSIONPACK
$(echo_cmd) "GAME_Q3LCC_MISSIONPACK $<"
$(Q)$(Q3LCC) $(MISSIONPACK_CFLAGS) -DQAGAME -o $@ $<
endef

define DO_UI_Q3LCC_MISSIONPACK
$(echo_cmd) "UI_Q3LCC_MISSIONPACK $<"
$(Q)$(Q3LCC) $(MISSIONPACK_CFLAGS) -DUI -o $@ $<
endef


Q3ASMOBJ = \
  $(O)/tools/asm/q3asm.o \
  $(O)/tools/asm/cmdlib.o

$(O)/tools/asm/%.o: $(Q3ASMDIR)/%.c
	$(DO_TOOLS_CC)

$(Q3ASM): $(Q3ASMOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)


#############################################################################
# CLIENT/SERVER
#############################################################################

Q3OBJ = \
  $(O)/client/cl_cgame.o \
  $(O)/client/cl_cin.o \
  $(O)/client/cl_console.o \
  $(O)/client/cl_input.o \
  $(O)/client/cl_keys.o \
  $(O)/client/cl_main.o \
  $(O)/client/cl_net_chan.o \
  $(O)/client/cl_parse.o \
  $(O)/client/cl_scrn.o \
  $(O)/client/cl_ui.o \
  $(O)/client/cl_avi.o \
  \
  $(O)/client/cm_load.o \
  $(O)/client/cm_patch.o \
  $(O)/client/cm_polylib.o \
  $(O)/client/cm_test.o \
  $(O)/client/cm_trace.o \
  \
  $(O)/client/cmd.o \
  $(O)/client/common.o \
  $(O)/client/cvar.o \
  $(O)/client/files.o \
  $(O)/client/md4.o \
  $(O)/client/md5.o \
  $(O)/client/msg.o \
  $(O)/client/net_chan.o \
  $(O)/client/net_ip.o \
  $(O)/client/huffman.o \
  \
  $(O)/client/snd_adpcm.o \
  $(O)/client/snd_dma.o \
  $(O)/client/snd_mem.o \
  $(O)/client/snd_mix.o \
  $(O)/client/snd_wavelet.o \
  \
  $(O)/client/snd_main.o \
  $(O)/client/snd_codec.o \
  $(O)/client/snd_codec_wav.o \
  $(O)/client/snd_codec_ogg.o \
  \
  $(O)/client/qal.o \
  $(O)/client/snd_openal.o \
  \
  $(O)/client/cl_curl.o \
  \
  $(O)/client/sv_bot.o \
  $(O)/client/sv_ccmds.o \
  $(O)/client/sv_client.o \
  $(O)/client/sv_game.o \
  $(O)/client/sv_init.o \
  $(O)/client/sv_main.o \
  $(O)/client/sv_net_chan.o \
  $(O)/client/sv_snapshot.o \
  $(O)/client/sv_world.o \
  \
  $(O)/client/q_math.o \
  $(O)/client/q_shared.o \
  \
  $(O)/client/unzip.o \
  $(O)/client/ioapi.o \
  $(O)/client/puff.o \
  $(O)/client/vm.o \
  $(O)/client/vm_interpreted.o \
  \
  $(O)/client/be_aas_bspq3.o \
  $(O)/client/be_aas_cluster.o \
  $(O)/client/be_aas_debug.o \
  $(O)/client/be_aas_entity.o \
  $(O)/client/be_aas_file.o \
  $(O)/client/be_aas_main.o \
  $(O)/client/be_aas_move.o \
  $(O)/client/be_aas_optimize.o \
  $(O)/client/be_aas_reach.o \
  $(O)/client/be_aas_route.o \
  $(O)/client/be_aas_routealt.o \
  $(O)/client/be_aas_sample.o \
  $(O)/client/be_ai_char.o \
  $(O)/client/be_ai_chat.o \
  $(O)/client/be_ai_gen.o \
  $(O)/client/be_ai_goal.o \
  $(O)/client/be_ai_move.o \
  $(O)/client/be_ai_weap.o \
  $(O)/client/be_ai_weight.o \
  $(O)/client/be_ea.o \
  $(O)/client/be_interface.o \
  $(O)/client/l_crc.o \
  $(O)/client/l_libvar.o \
  $(O)/client/l_log.o \
  $(O)/client/l_memory.o \
  $(O)/client/l_precomp.o \
  $(O)/client/l_script.o \
  $(O)/client/l_struct.o \
  \
  $(O)/client/sdl_input.o \
  $(O)/client/sdl_snd.o \
  \
  $(O)/client/con_passive.o \
  $(O)/client/con_log.o \
  $(O)/client/sys_main.o

Q3R2OBJ = \
  $(O)/renderergl2/tr_animation.o \
  $(O)/renderergl2/tr_backend.o \
  $(O)/renderergl2/tr_bsp.o \
  $(O)/renderergl2/tr_cmds.o \
  $(O)/renderergl2/tr_curve.o \
  $(O)/renderergl2/tr_extramath.o \
  $(O)/renderergl2/tr_extensions.o \
  $(O)/renderergl2/tr_fbo.o \
  $(O)/renderergl2/tr_flares.o \
  $(O)/renderergl2/tr_font.o \
  $(O)/renderergl2/tr_glsl.o \
  $(O)/renderergl2/tr_image.o \
  $(O)/renderer/tr_image_png.o \
  $(O)/renderer/tr_image_jpg.o \
  $(O)/renderer/tr_image_bmp.o \
  $(O)/renderer/tr_image_tga.o \
  $(O)/renderer/tr_image_pcx.o \
  $(O)/renderergl2/tr_init.o \
  $(O)/renderergl2/tr_light.o \
  $(O)/renderergl2/tr_main.o \
  $(O)/renderergl2/tr_marks.o \
  $(O)/renderergl2/tr_mesh.o \
  $(O)/renderergl2/tr_model.o \
  $(O)/renderergl2/tr_model_iqm.o \
  $(O)/renderer/tr_noise.o \
  $(O)/renderergl2/tr_postprocess.o \
  $(O)/renderergl2/tr_scene.o \
  $(O)/renderergl2/tr_shade.o \
  $(O)/renderergl2/tr_shade_calc.o \
  $(O)/renderergl2/tr_shader.o \
  $(O)/renderergl2/tr_shadows.o \
  $(O)/renderergl2/tr_sky.o \
  $(O)/renderergl2/tr_surface.o \
  $(O)/renderergl2/tr_vbo.o \
  $(O)/renderergl2/tr_world.o \
  \
  $(O)/renderer/sdl_gamma.o \
  $(O)/renderer/sdl_glimp.o

Q3ROBJ = \
  $(O)/renderer/tr_animation.o \
  $(O)/renderer/tr_backend.o \
  $(O)/renderer/tr_bsp.o \
  $(O)/renderer/tr_cmds.o \
  $(O)/renderer/tr_curve.o \
  $(O)/renderer/tr_flares.o \
  $(O)/renderer/tr_font.o \
  $(O)/renderer/tr_image.o \
  $(O)/renderer/tr_image_png.o \
  $(O)/renderer/tr_image_jpg.o \
  $(O)/renderer/tr_image_bmp.o \
  $(O)/renderer/tr_image_tga.o \
  $(O)/renderer/tr_image_pcx.o \
  $(O)/renderer/tr_init.o \
  $(O)/renderer/tr_light.o \
  $(O)/renderer/tr_main.o \
  $(O)/renderer/tr_marks.o \
  $(O)/renderer/tr_mesh.o \
  $(O)/renderer/tr_model.o \
  $(O)/renderer/tr_model_iqm.o \
  $(O)/renderer/tr_noise.o \
  $(O)/renderer/tr_scene.o \
  $(O)/renderer/tr_shade.o \
  $(O)/renderer/tr_shade_calc.o \
  $(O)/renderer/tr_shader.o \
  $(O)/renderer/tr_shadows.o \
  $(O)/renderer/tr_sky.o \
  $(O)/renderer/tr_surface.o \
  $(O)/renderer/tr_world.o \
  \
  $(O)/renderer/sdl_gamma.o \
  $(O)/renderer/sdl_glimp.o

ifneq ($(USE_RENDERER_DLOPEN), 0)
  Q3ROBJ += \
    $(O)/renderer/q_shared.o \
    $(O)/renderer/puff.o \
    $(O)/renderer/q_math.o \
    $(O)/renderer/tr_subs.o

  Q3R2OBJ += \
    $(O)/renderer/q_shared.o \
    $(O)/renderer/puff.o \
    $(O)/renderer/q_math.o \
    $(O)/renderer/tr_subs.o
endif

ifneq ($(USE_INTERNAL_JPEG),0)
  JPGOBJ = \
    $(O)/renderer/jaricom.o \
    $(O)/renderer/jcapimin.o \
    $(O)/renderer/jcapistd.o \
    $(O)/renderer/jcarith.o \
    $(O)/renderer/jccoefct.o  \
    $(O)/renderer/jccolor.o \
    $(O)/renderer/jcdctmgr.o \
    $(O)/renderer/jchuff.o   \
    $(O)/renderer/jcinit.o \
    $(O)/renderer/jcmainct.o \
    $(O)/renderer/jcmarker.o \
    $(O)/renderer/jcmaster.o \
    $(O)/renderer/jcomapi.o \
    $(O)/renderer/jcparam.o \
    $(O)/renderer/jcprepct.o \
    $(O)/renderer/jcsample.o \
    $(O)/renderer/jctrans.o \
    $(O)/renderer/jdapimin.o \
    $(O)/renderer/jdapistd.o \
    $(O)/renderer/jdarith.o \
    $(O)/renderer/jdatadst.o \
    $(O)/renderer/jdatasrc.o \
    $(O)/renderer/jdcoefct.o \
    $(O)/renderer/jdcolor.o \
    $(O)/renderer/jddctmgr.o \
    $(O)/renderer/jdhuff.o \
    $(O)/renderer/jdinput.o \
    $(O)/renderer/jdmainct.o \
    $(O)/renderer/jdmarker.o \
    $(O)/renderer/jdmaster.o \
    $(O)/renderer/jdmerge.o \
    $(O)/renderer/jdpostct.o \
    $(O)/renderer/jdsample.o \
    $(O)/renderer/jdtrans.o \
    $(O)/renderer/jerror.o \
    $(O)/renderer/jfdctflt.o \
    $(O)/renderer/jfdctfst.o \
    $(O)/renderer/jfdctint.o \
    $(O)/renderer/jidctflt.o \
    $(O)/renderer/jidctfst.o \
    $(O)/renderer/jidctint.o \
    $(O)/renderer/jmemmgr.o \
    $(O)/renderer/jmemnobs.o \
    $(O)/renderer/jquant1.o \
    $(O)/renderer/jquant2.o \
    $(O)/renderer/jutils.o
endif

ifeq ($(ARCH),i386)
  Q3OBJ += \
    $(O)/client/snd_mixa.o \
    $(O)/client/matha.o \
    $(O)/client/snapvector.o \
    $(O)/client/ftola.o
endif
ifeq ($(ARCH),x86)
  Q3OBJ += \
    $(O)/client/snd_mixa.o \
    $(O)/client/matha.o \
    $(O)/client/snapvector.o \
    $(O)/client/ftola.o
endif
ifeq ($(ARCH),x86_64)
  Q3OBJ += \
    $(O)/client/snapvector.o \
    $(O)/client/ftola.o
endif
ifeq ($(ARCH),amd64)
  Q3OBJ += \
    $(O)/client/snapvector.o \
    $(O)/client/ftola.o
endif
ifeq ($(ARCH),x64)
  Q3OBJ += \
    $(O)/client/snapvector.o \
    $(O)/client/ftola.o
endif

ifeq ($(USE_VOIP),1)
ifeq ($(USE_INTERNAL_SPEEX),1)
Q3OBJ += \
  $(O)/client/bits.o \
  $(O)/client/buffer.o \
  $(O)/client/cb_search.o \
  $(O)/client/exc_10_16_table.o \
  $(O)/client/exc_10_32_table.o \
  $(O)/client/exc_20_32_table.o \
  $(O)/client/exc_5_256_table.o \
  $(O)/client/exc_5_64_table.o \
  $(O)/client/exc_8_128_table.o \
  $(O)/client/fftwrap.o \
  $(O)/client/filterbank.o \
  $(O)/client/filters.o \
  $(O)/client/gain_table.o \
  $(O)/client/gain_table_lbr.o \
  $(O)/client/hexc_10_32_table.o \
  $(O)/client/hexc_table.o \
  $(O)/client/high_lsp_tables.o \
  $(O)/client/jitter.o \
  $(O)/client/kiss_fft.o \
  $(O)/client/kiss_fftr.o \
  $(O)/client/lpc.o \
  $(O)/client/lsp.o \
  $(O)/client/lsp_tables_nb.o \
  $(O)/client/ltp.o \
  $(O)/client/mdf.o \
  $(O)/client/modes.o \
  $(O)/client/modes_wb.o \
  $(O)/client/nb_celp.o \
  $(O)/client/preprocess.o \
  $(O)/client/quant_lsp.o \
  $(O)/client/resample.o \
  $(O)/client/sb_celp.o \
  $(O)/client/smallft.o \
  $(O)/client/speex.o \
  $(O)/client/speex_callbacks.o \
  $(O)/client/speex_header.o \
  $(O)/client/stereo.o \
  $(O)/client/vbr.o \
  $(O)/client/vq.o \
  $(O)/client/window.o
endif
endif

ifeq ($(USE_INTERNAL_ZLIB),1)
Q3OBJ += \
  $(O)/client/adler32.o \
  $(O)/client/crc32.o \
  $(O)/client/inffast.o \
  $(O)/client/inflate.o \
  $(O)/client/inftrees.o \
  $(O)/client/zutil.o
endif

ifeq ($(HAVE_VM_COMPILED),true)
  ifeq ($(ARCH),i386)
    Q3OBJ += \
      $(O)/client/vm_x86.o
  endif
  ifeq ($(ARCH),x86)
    Q3OBJ += \
      $(O)/client/vm_x86.o
  endif
  ifeq ($(ARCH),x86_64)
    ifeq ($(USE_OLD_VM64),1)
      Q3OBJ += \
        $(O)/client/vm_x86_64.o \
        $(O)/client/vm_x86_64_assembler.o
    else
      Q3OBJ += \
        $(O)/client/vm_x86.o
    endif
  endif
  ifeq ($(ARCH),amd64)
    ifeq ($(USE_OLD_VM64),1)
      Q3OBJ += \
        $(O)/client/vm_x86_64.o \
        $(O)/client/vm_x86_64_assembler.o
    else
      Q3OBJ += \
        $(O)/client/vm_x86.o
    endif
  endif
  ifeq ($(ARCH),x64)
    ifeq ($(USE_OLD_VM64),1)
      Q3OBJ += \
        $(O)/client/vm_x86_64.o \
        $(O)/client/vm_x86_64_assembler.o
    else
      Q3OBJ += \
        $(O)/client/vm_x86.o
    endif
  endif
  ifeq ($(ARCH),ppc)
    Q3OBJ += $(O)/client/vm_powerpc.o $(B)/client/vm_powerpc_asm.o
  endif
  ifeq ($(ARCH),ppc64)
    Q3OBJ += $(O)/client/vm_powerpc.o $(B)/client/vm_powerpc_asm.o
  endif
  ifeq ($(ARCH),sparc)
    Q3OBJ += $(O)/client/vm_sparc.o
  endif
endif

ifeq ($(PLATFORM),mingw32)
  Q3OBJ += \
    $(O)/client/win_resource.o \
    $(O)/client/sys_win32.o
else
  Q3OBJ += \
    $(O)/client/sys_unix.o
endif

ifeq ($(PLATFORM),darwin)
  Q3OBJ += \
    $(O)/client/sys_osx.o
endif

ifeq ($(USE_MUMBLE),1)
  Q3OBJ += \
    $(O)/client/libmumblelink.o
endif

ifneq ($(USE_RENDERER_DLOPEN),0)
$(B)/$(CLIENTBIN)$(FULLBINEXT): $(Q3OBJ) $(LIBSDLMAIN)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CLIENT_CFLAGS) $(CFLAGS) $(CLIENT_LDFLAGS) $(LDFLAGS) \
		-o $@ $(Q3OBJ) \
		$(LIBSDLMAIN) $(CLIENT_LIBS) $(LIBS)

$(B)/renderer_opengl1_$(SHLIBNAME): $(Q3ROBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3ROBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(RENDERER_LIBS) $(LIBS)

$(B)/renderer_opengl1_smp_$(SHLIBNAME): $(Q3ROBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3ROBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(RENDERER_LIBS) $(LIBS)

$(B)/renderer_opengl2_$(SHLIBNAME): $(Q3R2OBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3R2OBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(RENDERER_LIBS) $(LIBS)

$(B)/renderer_opengl2_smp_$(SHLIBNAME): $(Q3R2OBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3R2OBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(RENDERER_LIBS) $(LIBS)

else
$(B)/$(CLIENTBIN)$(FULLBINEXT): $(Q3OBJ) $(Q3ROBJ) $(LIBSDLMAIN)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CLIENT_CFLAGS) $(CFLAGS) $(CLIENT_LDFLAGS) $(LDFLAGS) \
		-o $@ $(Q3OBJ) $(Q3ROBJ) $(JPGOBJ) \
		$(LIBSDLMAIN) $(CLIENT_LIBS) $(RENDERER_LIBS) $(LIBS)

$(B)/$(CLIENTBIN)-smp$(FULLBINEXT): $(Q3OBJ) $(Q3ROBJ) $(LIBSDLMAIN)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CLIENT_CFLAGS) $(CFLAGS) $(CLIENT_LDFLAGS) $(LDFLAGS) $(THREAD_LDFLAGS) \
		-o $@ $(Q3OBJ) $(Q3ROBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(CLIENT_LIBS) $(RENDERER_LIBS) $(LIBS)
endif

ifneq ($(strip $(LIBSDLMAIN)),)
ifneq ($(strip $(LIBSDLMAINSRC)),)
$(LIBSDLMAIN) : $(LIBSDLMAINSRC)
	cp $< $@
	ranlib $@
endif
endif



#############################################################################
# DEDICATED SERVER
#############################################################################

Q3DOBJ = \
  $(O)/ded/sv_bot.o \
  $(O)/ded/sv_client.o \
  $(O)/ded/sv_ccmds.o \
  $(O)/ded/sv_game.o \
  $(O)/ded/sv_init.o \
  $(O)/ded/sv_main.o \
  $(O)/ded/sv_net_chan.o \
  $(O)/ded/sv_snapshot.o \
  $(O)/ded/sv_world.o \
  \
  $(O)/ded/cm_load.o \
  $(O)/ded/cm_patch.o \
  $(O)/ded/cm_polylib.o \
  $(O)/ded/cm_test.o \
  $(O)/ded/cm_trace.o \
  $(O)/ded/cmd.o \
  $(O)/ded/common.o \
  $(O)/ded/cvar.o \
  $(O)/ded/files.o \
  $(O)/ded/md4.o \
  $(O)/ded/msg.o \
  $(O)/ded/net_chan.o \
  $(O)/ded/net_ip.o \
  $(O)/ded/huffman.o \
  \
  $(O)/ded/q_math.o \
  $(O)/ded/q_shared.o \
  \
  $(O)/ded/unzip.o \
  $(O)/ded/ioapi.o \
  $(O)/ded/vm.o \
  $(O)/ded/vm_interpreted.o \
  \
  $(O)/ded/be_aas_bspq3.o \
  $(O)/ded/be_aas_cluster.o \
  $(O)/ded/be_aas_debug.o \
  $(O)/ded/be_aas_entity.o \
  $(O)/ded/be_aas_file.o \
  $(O)/ded/be_aas_main.o \
  $(O)/ded/be_aas_move.o \
  $(O)/ded/be_aas_optimize.o \
  $(O)/ded/be_aas_reach.o \
  $(O)/ded/be_aas_route.o \
  $(O)/ded/be_aas_routealt.o \
  $(O)/ded/be_aas_sample.o \
  $(O)/ded/be_ai_char.o \
  $(O)/ded/be_ai_chat.o \
  $(O)/ded/be_ai_gen.o \
  $(O)/ded/be_ai_goal.o \
  $(O)/ded/be_ai_move.o \
  $(O)/ded/be_ai_weap.o \
  $(O)/ded/be_ai_weight.o \
  $(O)/ded/be_ea.o \
  $(O)/ded/be_interface.o \
  $(O)/ded/l_crc.o \
  $(O)/ded/l_libvar.o \
  $(O)/ded/l_log.o \
  $(O)/ded/l_memory.o \
  $(O)/ded/l_precomp.o \
  $(O)/ded/l_script.o \
  $(O)/ded/l_struct.o \
  \
  $(O)/ded/null_client.o \
  $(O)/ded/null_input.o \
  $(O)/ded/null_snddma.o \
  \
  $(O)/ded/con_log.o \
  $(O)/ded/sys_main.o

ifeq ($(ARCH),i386)
  Q3DOBJ += \
      $(O)/ded/matha.o \
      $(O)/ded/snapvector.o \
      $(O)/ded/ftola.o
endif
ifeq ($(ARCH),x86)
  Q3DOBJ += \
      $(O)/ded/matha.o \
      $(O)/ded/snapvector.o \
      $(O)/ded/ftola.o 
endif
ifeq ($(ARCH),x86_64)
  Q3DOBJ += \
      $(O)/ded/snapvector.o \
      $(O)/ded/ftola.o 
endif
ifeq ($(ARCH),amd64)
  Q3DOBJ += \
      $(O)/ded/snapvector.o \
      $(O)/ded/ftola.o 
endif
ifeq ($(ARCH),x64)
  Q3DOBJ += \
      $(O)/ded/snapvector.o \
      $(O)/ded/ftola.o 
endif

ifeq ($(USE_INTERNAL_ZLIB),1)
Q3DOBJ += \
  $(O)/ded/adler32.o \
  $(O)/ded/crc32.o \
  $(O)/ded/inffast.o \
  $(O)/ded/inflate.o \
  $(O)/ded/inftrees.o \
  $(O)/ded/zutil.o
endif

ifeq ($(HAVE_VM_COMPILED),true)
  ifeq ($(ARCH),i386)
    Q3DOBJ += \
      $(O)/ded/vm_x86.o
  endif
  ifeq ($(ARCH),x86)
    Q3DOBJ += \
      $(O)/ded/vm_x86.o
  endif
  ifeq ($(ARCH),x86_64)
    ifeq ($(USE_OLD_VM64),1)
      Q3DOBJ += \
        $(O)/ded/vm_x86_64.o \
        $(O)/ded/vm_x86_64_assembler.o
    else
      Q3DOBJ += \
        $(O)/ded/vm_x86.o
    endif
  endif
  ifeq ($(ARCH),amd64)
    ifeq ($(USE_OLD_VM64),1)
      Q3DOBJ += \
        $(O)/ded/vm_x86_64.o \
        $(O)/ded/vm_x86_64_assembler.o
    else
      Q3DOBJ += \
        $(O)/ded/vm_x86.o
    endif
  endif
  ifeq ($(ARCH),x64)
    ifeq ($(USE_OLD_VM64),1)
      Q3DOBJ += \
        $(O)/ded/vm_x86_64.o \
        $(O)/ded/vm_x86_64_assembler.o
    else
      Q3DOBJ += \
        $(O)/ded/vm_x86.o
    endif
  endif
  ifeq ($(ARCH),ppc)
    Q3DOBJ += \
      $(O)/ded/vm_powerpc.o \
      $(O)/ded/vm_powerpc_asm.o
  endif
  ifeq ($(ARCH),ppc64)
    Q3DOBJ += \
      $(O)/ded/vm_powerpc.o \
      $(O)/ded/vm_powerpc_asm.o
  endif
  ifeq ($(ARCH),sparc)
    Q3DOBJ += $(O)/ded/vm_sparc.o
  endif
endif

ifeq ($(PLATFORM),mingw32)
  Q3DOBJ += \
    $(O)/ded/win_resource.o \
    $(O)/ded/sys_win32.o \
    $(O)/ded/con_win32.o
else
  Q3DOBJ += \
    $(O)/ded/sys_unix.o \
    $(O)/ded/con_tty.o
endif

ifeq ($(PLATFORM),darwin)
  Q3DOBJ += \
    $(O)/ded/sys_osx.o
endif

$(B)/$(SERVERBIN)$(FULLBINEXT): $(Q3DOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(Q3DOBJ) $(LIBS)



#############################################################################
## BASEQ3 CGAME
#############################################################################

Q3CGOBJ_ = \
  $(O)/$(BASEGAME)/cgame/cg_main.o \
  $(O)/$(BASEGAME)/cgame/bg_misc.o \
  $(O)/$(BASEGAME)/cgame/bg_pmove.o \
  $(O)/$(BASEGAME)/cgame/bg_slidemove.o \
  $(O)/$(BASEGAME)/cgame/bg_lib.o \
  $(O)/$(BASEGAME)/cgame/cg_consolecmds.o \
  $(O)/$(BASEGAME)/cgame/cg_draw.o \
  $(O)/$(BASEGAME)/cgame/cg_drawtools.o \
  $(O)/$(BASEGAME)/cgame/cg_effects.o \
  $(O)/$(BASEGAME)/cgame/cg_ents.o \
  $(O)/$(BASEGAME)/cgame/cg_event.o \
  $(O)/$(BASEGAME)/cgame/cg_info.o \
  $(O)/$(BASEGAME)/cgame/cg_localents.o \
  $(O)/$(BASEGAME)/cgame/cg_marks.o \
  $(O)/$(BASEGAME)/cgame/cg_particles.o \
  $(O)/$(BASEGAME)/cgame/cg_players.o \
  $(O)/$(BASEGAME)/cgame/cg_playerstate.o \
  $(O)/$(BASEGAME)/cgame/cg_predict.o \
  $(O)/$(BASEGAME)/cgame/cg_scoreboard.o \
  $(O)/$(BASEGAME)/cgame/cg_servercmds.o \
  $(O)/$(BASEGAME)/cgame/cg_snapshot.o \
  $(O)/$(BASEGAME)/cgame/cg_view.o \
  $(O)/$(BASEGAME)/cgame/cg_weapons.o \
  \
  $(O)/$(BASEGAME)/qcommon/q_math.o \
  $(O)/$(BASEGAME)/qcommon/q_shared.o

Q3CGOBJ = $(Q3CGOBJ_) $(O)/$(BASEGAME)/cgame/cg_syscalls.o
Q3CGVMOBJ = $(Q3CGOBJ_:%.o=%.asm)

$(B)/$(BASEGAME)/cgame$(SHLIBNAME): $(Q3CGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3CGOBJ)

$(B)/$(BASEGAME)/vm/cgame.qvm: $(Q3CGVMOBJ) $(CGDIR)/cg_syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(Q3CGVMOBJ) $(CGDIR)/cg_syscalls.asm

#############################################################################
## MISSIONPACK CGAME
#############################################################################

MPCGOBJ_ = \
  $(O)/$(MISSIONPACK)/cgame/cg_main.o \
  $(O)/$(MISSIONPACK)/cgame/bg_misc.o \
  $(O)/$(MISSIONPACK)/cgame/bg_pmove.o \
  $(O)/$(MISSIONPACK)/cgame/bg_slidemove.o \
  $(O)/$(MISSIONPACK)/cgame/bg_lib.o \
  $(O)/$(MISSIONPACK)/cgame/cg_consolecmds.o \
  $(O)/$(MISSIONPACK)/cgame/cg_newdraw.o \
  $(O)/$(MISSIONPACK)/cgame/cg_draw.o \
  $(O)/$(MISSIONPACK)/cgame/cg_drawtools.o \
  $(O)/$(MISSIONPACK)/cgame/cg_effects.o \
  $(O)/$(MISSIONPACK)/cgame/cg_ents.o \
  $(O)/$(MISSIONPACK)/cgame/cg_event.o \
  $(O)/$(MISSIONPACK)/cgame/cg_info.o \
  $(O)/$(MISSIONPACK)/cgame/cg_localents.o \
  $(O)/$(MISSIONPACK)/cgame/cg_marks.o \
  $(O)/$(MISSIONPACK)/cgame/cg_particles.o \
  $(O)/$(MISSIONPACK)/cgame/cg_players.o \
  $(O)/$(MISSIONPACK)/cgame/cg_playerstate.o \
  $(O)/$(MISSIONPACK)/cgame/cg_predict.o \
  $(O)/$(MISSIONPACK)/cgame/cg_scoreboard.o \
  $(O)/$(MISSIONPACK)/cgame/cg_servercmds.o \
  $(O)/$(MISSIONPACK)/cgame/cg_snapshot.o \
  $(O)/$(MISSIONPACK)/cgame/cg_view.o \
  $(O)/$(MISSIONPACK)/cgame/cg_weapons.o \
  $(O)/$(MISSIONPACK)/ui/ui_shared.o \
  \
  $(O)/$(MISSIONPACK)/qcommon/q_math.o \
  $(O)/$(MISSIONPACK)/qcommon/q_shared.o

MPCGOBJ = $(MPCGOBJ_) $(O)/$(MISSIONPACK)/cgame/cg_syscalls.o
MPCGVMOBJ = $(MPCGOBJ_:%.o=%.asm)

$(B)/$(MISSIONPACK)/cgame$(SHLIBNAME): $(MPCGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(MPCGOBJ)

$(B)/$(MISSIONPACK)/vm/cgame.qvm: $(MPCGVMOBJ) $(CGDIR)/cg_syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(MPCGVMOBJ) $(CGDIR)/cg_syscalls.asm



#############################################################################
## BASEQ3 GAME
#############################################################################

Q3GOBJ_ = \
  $(O)/$(BASEGAME)/game/g_main.o \
  $(O)/$(BASEGAME)/game/ai_chat.o \
  $(O)/$(BASEGAME)/game/ai_cmd.o \
  $(O)/$(BASEGAME)/game/ai_dmnet.o \
  $(O)/$(BASEGAME)/game/ai_dmq3.o \
  $(O)/$(BASEGAME)/game/ai_main.o \
  $(O)/$(BASEGAME)/game/ai_team.o \
  $(O)/$(BASEGAME)/game/ai_vcmd.o \
  $(O)/$(BASEGAME)/game/bg_misc.o \
  $(O)/$(BASEGAME)/game/bg_pmove.o \
  $(O)/$(BASEGAME)/game/bg_slidemove.o \
  $(O)/$(BASEGAME)/game/bg_lib.o \
  $(O)/$(BASEGAME)/game/g_active.o \
  $(O)/$(BASEGAME)/game/g_arenas.o \
  $(O)/$(BASEGAME)/game/g_bot.o \
  $(O)/$(BASEGAME)/game/g_client.o \
  $(O)/$(BASEGAME)/game/g_cmds.o \
  $(O)/$(BASEGAME)/game/g_combat.o \
  $(O)/$(BASEGAME)/game/g_items.o \
  $(O)/$(BASEGAME)/game/g_mem.o \
  $(O)/$(BASEGAME)/game/g_misc.o \
  $(O)/$(BASEGAME)/game/g_missile.o \
  $(O)/$(BASEGAME)/game/g_mover.o \
  $(O)/$(BASEGAME)/game/g_session.o \
  $(O)/$(BASEGAME)/game/g_spawn.o \
  $(O)/$(BASEGAME)/game/g_svcmds.o \
  $(O)/$(BASEGAME)/game/g_target.o \
  $(O)/$(BASEGAME)/game/g_team.o \
  $(O)/$(BASEGAME)/game/g_trigger.o \
  $(O)/$(BASEGAME)/game/g_utils.o \
  $(O)/$(BASEGAME)/game/g_weapon.o \
  \
  $(O)/$(BASEGAME)/qcommon/q_math.o \
  $(O)/$(BASEGAME)/qcommon/q_shared.o

Q3GOBJ = $(Q3GOBJ_) $(O)/$(BASEGAME)/game/g_syscalls.o
Q3GVMOBJ = $(Q3GOBJ_:%.o=%.asm)

$(B)/$(BASEGAME)/qagame$(SHLIBNAME): $(Q3GOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3GOBJ)

$(B)/$(BASEGAME)/vm/qagame.qvm: $(Q3GVMOBJ) $(GDIR)/g_syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(Q3GVMOBJ) $(GDIR)/g_syscalls.asm

#############################################################################
## MISSIONPACK GAME
#############################################################################

MPGOBJ_ = \
  $(O)/$(MISSIONPACK)/game/g_main.o \
  $(O)/$(MISSIONPACK)/game/ai_chat.o \
  $(O)/$(MISSIONPACK)/game/ai_cmd.o \
  $(O)/$(MISSIONPACK)/game/ai_dmnet.o \
  $(O)/$(MISSIONPACK)/game/ai_dmq3.o \
  $(O)/$(MISSIONPACK)/game/ai_main.o \
  $(O)/$(MISSIONPACK)/game/ai_team.o \
  $(O)/$(MISSIONPACK)/game/ai_vcmd.o \
  $(O)/$(MISSIONPACK)/game/bg_misc.o \
  $(O)/$(MISSIONPACK)/game/bg_pmove.o \
  $(O)/$(MISSIONPACK)/game/bg_slidemove.o \
  $(O)/$(MISSIONPACK)/game/bg_lib.o \
  $(O)/$(MISSIONPACK)/game/g_active.o \
  $(O)/$(MISSIONPACK)/game/g_arenas.o \
  $(O)/$(MISSIONPACK)/game/g_bot.o \
  $(O)/$(MISSIONPACK)/game/g_client.o \
  $(O)/$(MISSIONPACK)/game/g_cmds.o \
  $(O)/$(MISSIONPACK)/game/g_combat.o \
  $(O)/$(MISSIONPACK)/game/g_items.o \
  $(O)/$(MISSIONPACK)/game/g_mem.o \
  $(O)/$(MISSIONPACK)/game/g_misc.o \
  $(O)/$(MISSIONPACK)/game/g_missile.o \
  $(O)/$(MISSIONPACK)/game/g_mover.o \
  $(O)/$(MISSIONPACK)/game/g_session.o \
  $(O)/$(MISSIONPACK)/game/g_spawn.o \
  $(O)/$(MISSIONPACK)/game/g_svcmds.o \
  $(O)/$(MISSIONPACK)/game/g_target.o \
  $(O)/$(MISSIONPACK)/game/g_team.o \
  $(O)/$(MISSIONPACK)/game/g_trigger.o \
  $(O)/$(MISSIONPACK)/game/g_utils.o \
  $(O)/$(MISSIONPACK)/game/g_weapon.o \
  \
  $(O)/$(MISSIONPACK)/qcommon/q_math.o \
  $(O)/$(MISSIONPACK)/qcommon/q_shared.o

MPGOBJ = $(MPGOBJ_) $(O)/$(MISSIONPACK)/game/g_syscalls.o
MPGVMOBJ = $(MPGOBJ_:%.o=%.asm)

$(B)/$(MISSIONPACK)/qagame$(SHLIBNAME): $(MPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(MPGOBJ)

$(B)/$(MISSIONPACK)/vm/qagame.qvm: $(MPGVMOBJ) $(GDIR)/g_syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(MPGVMOBJ) $(GDIR)/g_syscalls.asm



#############################################################################
## BASEQ3 UI
#############################################################################

Q3UIOBJ_ = \
  $(O)/$(BASEGAME)/ui/ui_main.o \
  $(O)/$(BASEGAME)/ui/bg_misc.o \
  $(O)/$(BASEGAME)/ui/bg_lib.o \
  $(O)/$(BASEGAME)/ui/ui_addbots.o \
  $(O)/$(BASEGAME)/ui/ui_atoms.o \
  $(O)/$(BASEGAME)/ui/ui_cdkey.o \
  $(O)/$(BASEGAME)/ui/ui_cinematics.o \
  $(O)/$(BASEGAME)/ui/ui_confirm.o \
  $(O)/$(BASEGAME)/ui/ui_connect.o \
  $(O)/$(BASEGAME)/ui/ui_controls2.o \
  $(O)/$(BASEGAME)/ui/ui_credits.o \
  $(O)/$(BASEGAME)/ui/ui_demo2.o \
  $(O)/$(BASEGAME)/ui/ui_display.o \
  $(O)/$(BASEGAME)/ui/ui_gameinfo.o \
  $(O)/$(BASEGAME)/ui/ui_ingame.o \
  $(O)/$(BASEGAME)/ui/ui_loadconfig.o \
  $(O)/$(BASEGAME)/ui/ui_menu.o \
  $(O)/$(BASEGAME)/ui/ui_mfield.o \
  $(O)/$(BASEGAME)/ui/ui_mods.o \
  $(O)/$(BASEGAME)/ui/ui_network.o \
  $(O)/$(BASEGAME)/ui/ui_options.o \
  $(O)/$(BASEGAME)/ui/ui_playermodel.o \
  $(O)/$(BASEGAME)/ui/ui_players.o \
  $(O)/$(BASEGAME)/ui/ui_playersettings.o \
  $(O)/$(BASEGAME)/ui/ui_preferences.o \
  $(O)/$(BASEGAME)/ui/ui_qmenu.o \
  $(O)/$(BASEGAME)/ui/ui_removebots.o \
  $(O)/$(BASEGAME)/ui/ui_saveconfig.o \
  $(O)/$(BASEGAME)/ui/ui_serverinfo.o \
  $(O)/$(BASEGAME)/ui/ui_servers2.o \
  $(O)/$(BASEGAME)/ui/ui_setup.o \
  $(O)/$(BASEGAME)/ui/ui_sound.o \
  $(O)/$(BASEGAME)/ui/ui_sparena.o \
  $(O)/$(BASEGAME)/ui/ui_specifyserver.o \
  $(O)/$(BASEGAME)/ui/ui_splevel.o \
  $(O)/$(BASEGAME)/ui/ui_sppostgame.o \
  $(O)/$(BASEGAME)/ui/ui_spskill.o \
  $(O)/$(BASEGAME)/ui/ui_startserver.o \
  $(O)/$(BASEGAME)/ui/ui_team.o \
  $(O)/$(BASEGAME)/ui/ui_teamorders.o \
  $(O)/$(BASEGAME)/ui/ui_video.o \
  \
  $(O)/$(BASEGAME)/qcommon/q_math.o \
  $(O)/$(BASEGAME)/qcommon/q_shared.o

Q3UIOBJ = $(Q3UIOBJ_) $(O)/$(MISSIONPACK)/ui/ui_syscalls.o
Q3UIVMOBJ = $(Q3UIOBJ_:%.o=%.asm)

$(B)/$(BASEGAME)/ui$(SHLIBNAME): $(Q3UIOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3UIOBJ)

$(B)/$(BASEGAME)/vm/ui.qvm: $(Q3UIVMOBJ) $(UIDIR)/ui_syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(Q3UIVMOBJ) $(UIDIR)/ui_syscalls.asm

#############################################################################
## MISSIONPACK UI
#############################################################################

MPUIOBJ_ = \
  $(O)/$(MISSIONPACK)/ui/ui_main.o \
  $(O)/$(MISSIONPACK)/ui/ui_atoms.o \
  $(O)/$(MISSIONPACK)/ui/ui_gameinfo.o \
  $(O)/$(MISSIONPACK)/ui/ui_players.o \
  $(O)/$(MISSIONPACK)/ui/ui_shared.o \
  \
  $(O)/$(MISSIONPACK)/ui/bg_misc.o \
  $(O)/$(MISSIONPACK)/ui/bg_lib.o \
  \
  $(O)/$(MISSIONPACK)/qcommon/q_math.o \
  $(O)/$(MISSIONPACK)/qcommon/q_shared.o

MPUIOBJ = $(MPUIOBJ_) $(O)/$(MISSIONPACK)/ui/ui_syscalls.o
MPUIVMOBJ = $(MPUIOBJ_:%.o=%.asm)

$(B)/$(MISSIONPACK)/ui$(SHLIBNAME): $(MPUIOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(MPUIOBJ)

$(B)/$(MISSIONPACK)/vm/ui.qvm: $(MPUIVMOBJ) $(UIDIR)/ui_syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(MPUIVMOBJ) $(UIDIR)/ui_syscalls.asm



#############################################################################
## CLIENT/SERVER RULES
#############################################################################

$(O)/client/%.o: $(ASMDIR)/%.s
	$(DO_AS)

# k8 so inline assembler knows about SSE
$(O)/client/%.o: $(ASMDIR)/%.c
	$(DO_CC) -march=k8

$(O)/client/%.o: $(CDIR)/%.c
	$(DO_CC)

$(O)/client/%.o: $(SDIR)/%.c
	$(DO_CC)

$(O)/client/%.o: $(CMDIR)/%.c
	$(DO_CC)

$(O)/client/%.o: $(BLIBDIR)/%.c
	$(DO_BOT_CC)

$(O)/client/%.o: $(SPEEXDIR)/%.c
	$(DO_CC)

$(O)/client/%.o: $(ZDIR)/%.c
	$(DO_CC)

$(O)/client/%.o: $(SDLDIR)/%.c
	$(DO_CC)

$(O)/renderersmp/%.o: $(SDLDIR)/%.c
	$(DO_SMP_CC)

$(O)/client/%.o: $(SYSDIR)/%.c
	$(DO_CC)

$(O)/client/%.o: $(SYSDIR)/%.m
	$(DO_CC)

$(O)/client/%.o: $(SYSDIR)/%.rc
	$(DO_WINDRES)


$(O)/renderer/%.o: $(CMDIR)/%.c
	$(DO_REF_CC)

$(O)/renderer/%.o: $(SDLDIR)/%.c
	$(DO_REF_CC)

$(O)/renderer/%.o: $(JPDIR)/%.c
	$(DO_REF_CC)

$(O)/renderer/%.o: $(RDIR)/%.c
	$(DO_REF_CC)
	
$(O)/renderergl2/%.o: $(R2DIR)/%.c
	$(DO_REF_CC)


$(O)/ded/%.o: $(ASMDIR)/%.s
	$(DO_AS)

# k8 so inline assembler knows about SSE
$(O)/ded/%.o: $(ASMDIR)/%.c
	$(DO_CC) -march=k8

$(O)/ded/%.o: $(SDIR)/%.c
	$(DO_DED_CC)

$(O)/ded/%.o: $(CMDIR)/%.c
	$(DO_DED_CC)

$(O)/ded/%.o: $(ZDIR)/%.c
	$(DO_DED_CC)

$(O)/ded/%.o: $(BLIBDIR)/%.c
	$(DO_BOT_CC)

$(O)/ded/%.o: $(SYSDIR)/%.c
	$(DO_DED_CC)

$(O)/ded/%.o: $(SYSDIR)/%.m
	$(DO_DED_CC)

$(O)/ded/%.o: $(SYSDIR)/%.rc
	$(DO_WINDRES)

$(O)/ded/%.o: $(NDIR)/%.c
	$(DO_DED_CC)

# Extra dependencies to ensure the SVN version is incorporated
ifeq ($(USE_SVN),1)
  $(O)/client/cl_console.o : .svn/entries
  $(O)/client/common.o : .svn/entries
  $(O)/ded/common.o : .svn/entries
endif


#############################################################################
## GAME MODULE RULES
#############################################################################

$(O)/$(BASEGAME)/cgame/bg_%.o: $(GDIR)/bg_%.c
	$(DO_CGAME_CC)

$(O)/$(BASEGAME)/cgame/%.o: $(CGDIR)/%.c
	$(DO_CGAME_CC)

$(O)/$(BASEGAME)/cgame/bg_%.asm: $(GDIR)/bg_%.c $(Q3LCC)
	$(DO_CGAME_Q3LCC)

$(O)/$(BASEGAME)/cgame/%.asm: $(CGDIR)/%.c $(Q3LCC)
	$(DO_CGAME_Q3LCC)

$(O)/$(MISSIONPACK)/cgame/bg_%.o: $(GDIR)/bg_%.c
	$(DO_CGAME_CC_MISSIONPACK)

$(O)/$(MISSIONPACK)/cgame/%.o: $(CGDIR)/%.c
	$(DO_CGAME_CC_MISSIONPACK)

$(O)/$(MISSIONPACK)/cgame/bg_%.asm: $(GDIR)/bg_%.c $(Q3LCC)
	$(DO_CGAME_Q3LCC_MISSIONPACK)

$(O)/$(MISSIONPACK)/cgame/%.asm: $(CGDIR)/%.c $(Q3LCC)
	$(DO_CGAME_Q3LCC_MISSIONPACK)


$(O)/$(BASEGAME)/game/%.o: $(GDIR)/%.c
	$(DO_GAME_CC)

$(O)/$(BASEGAME)/game/%.asm: $(GDIR)/%.c $(Q3LCC)
	$(DO_GAME_Q3LCC)

$(O)/$(MISSIONPACK)/game/%.o: $(GDIR)/%.c
	$(DO_GAME_CC_MISSIONPACK)

$(O)/$(MISSIONPACK)/game/%.asm: $(GDIR)/%.c $(Q3LCC)
	$(DO_GAME_Q3LCC_MISSIONPACK)


$(O)/$(BASEGAME)/ui/bg_%.o: $(GDIR)/bg_%.c
	$(DO_UI_CC)

$(O)/$(BASEGAME)/ui/%.o: $(Q3UIDIR)/%.c
	$(DO_UI_CC)

$(O)/$(BASEGAME)/ui/bg_%.asm: $(GDIR)/bg_%.c $(Q3LCC)
	$(DO_UI_Q3LCC)

$(O)/$(BASEGAME)/ui/%.asm: $(Q3UIDIR)/%.c $(Q3LCC)
	$(DO_UI_Q3LCC)

$(O)/$(MISSIONPACK)/ui/bg_%.o: $(GDIR)/bg_%.c
	$(DO_UI_CC_MISSIONPACK)

$(O)/$(MISSIONPACK)/ui/%.o: $(UIDIR)/%.c
	$(DO_UI_CC_MISSIONPACK)

$(O)/$(MISSIONPACK)/ui/bg_%.asm: $(GDIR)/bg_%.c $(Q3LCC)
	$(DO_UI_Q3LCC_MISSIONPACK)

$(O)/$(MISSIONPACK)/ui/%.asm: $(UIDIR)/%.c $(Q3LCC)
	$(DO_UI_Q3LCC_MISSIONPACK)


$(O)/$(BASEGAME)/qcommon/%.o: $(CMDIR)/%.c
	$(DO_SHLIB_CC)

$(O)/$(BASEGAME)/qcommon/%.asm: $(CMDIR)/%.c $(Q3LCC)
	$(DO_Q3LCC)

$(O)/$(MISSIONPACK)/qcommon/%.o: $(CMDIR)/%.c
	$(DO_SHLIB_CC_MISSIONPACK)

$(O)/$(MISSIONPACK)/qcommon/%.asm: $(CMDIR)/%.c $(Q3LCC)
	$(DO_Q3LCC_MISSIONPACK)


#############################################################################
# MISC
#############################################################################

OBJ = $(Q3OBJ) $(Q3ROBJ) $(Q3R2OBJ) $(Q3DOBJ) $(JPGOBJ) \
  $(MPGOBJ) $(Q3GOBJ) $(Q3CGOBJ) $(MPCGOBJ) $(Q3UIOBJ) $(MPUIOBJ) \
  $(MPGVMOBJ) $(Q3GVMOBJ) $(Q3CGVMOBJ) $(MPCGVMOBJ) $(Q3UIVMOBJ) $(MPUIVMOBJ)
TOOLSOBJ = $(LBURGOBJ) $(Q3CPPOBJ) $(Q3RCCOBJ) $(Q3LCCOBJ) $(Q3ASMOBJ)


copyfiles: release
	@if [ ! -d $(COPYDIR)/$(BASEGAME) ]; then echo "You need to set COPYDIR to where your Quake3 data is!"; fi
ifneq ($(BUILD_GAME_SO),0)
  ifneq ($(BUILD_BASEGAME), 0)
		-$(MKDIR) -p -m 0755 $(COPYDIR)/$(BASEGAME)
  endif
  ifneq ($(BUILD_MISSIONPACK),0)
	-$(MKDIR) -p -m 0755 $(COPYDIR)/$(MISSIONPACK)
  endif
endif

ifneq ($(BUILD_CLIENT),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(CLIENTBIN)$(FULLBINEXT) $(COPYBINDIR)/$(CLIENTBIN)$(FULLBINEXT)
  ifneq ($(USE_RENDERER_DLOPEN),0)
    ifneq ($(BUILD_RENDERER_GL1),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/renderer_opengl1_$(SHLIBNAME) $(COPYBINDIR)/renderer_opengl1_$(SHLIBNAME)
    endif
    ifneq ($(BUILD_RENDERER_GL2),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/renderer_opengl2_$(SHLIBNAME) $(COPYBINDIR)/renderer_opengl2_$(SHLIBNAME)
    endif
  endif
endif

# Don't copy the SMP until it's working together with SDL.
ifneq ($(BUILD_CLIENT_SMP),0)
  ifneq ($(USE_RENDERER_DLOPEN),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/renderer_opengl1_smp_$(SHLIBNAME) $(COPYBINDIR)/renderer_opengl1_smp_$(SHLIBNAME)
  else
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(CLIENTBIN)-smp$(FULLBINEXT) $(COPYBINDIR)/$(CLIENTBIN)-smp$(FULLBINEXT)
  endif
endif

ifneq ($(BUILD_SERVER),0)
	@if [ -f $(BR)/$(SERVERBIN)$(FULLBINEXT) ]; then \
		$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(SERVERBIN)$(FULLBINEXT) $(COPYBINDIR)/$(SERVERBIN)$(FULLBINEXT); \
	fi
endif

ifneq ($(BUILD_GAME_SO),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(BASEGAME)/cgame$(SHLIBNAME) \
					$(COPYDIR)/$(BASEGAME)/.
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(BASEGAME)/qagame$(SHLIBNAME) \
					$(COPYDIR)/$(BASEGAME)/.
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(BASEGAME)/ui$(SHLIBNAME) \
					$(COPYDIR)/$(BASEGAME)/.
  ifneq ($(BUILD_MISSIONPACK),0)
	-$(MKDIR) -p -m 0755 $(COPYDIR)/$(MISSIONPACK)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(MISSIONPACK)/cgame$(SHLIBNAME) \
					$(COPYDIR)/$(MISSIONPACK)/.
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(MISSIONPACK)/qagame$(SHLIBNAME) \
					$(COPYDIR)/$(MISSIONPACK)/.
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(MISSIONPACK)/ui$(SHLIBNAME) \
					$(COPYDIR)/$(MISSIONPACK)/.
  endif
endif

clean: clean-debug clean-release
ifeq ($(PLATFORM),mingw32)
	@$(MAKE) -C $(NSISDIR) clean
else
	@$(MAKE) -C $(LOKISETUPDIR) clean
endif

clean-debug:
	@$(MAKE) clean2 B=$(BD) O=$(BD)/obj

clean-release:
	@$(MAKE) clean2 B=$(BR) O=$(BR)/obj

clean2:
	@echo "clean $(B) & $(O)"
	@rm -f $(OBJ)
	@rm -f $(OBJ_D_FILES)
	@rm -f $(TARGETS)

toolsclean: toolsclean-debug toolsclean-release

toolsclean-debug:
	@$(MAKE) toolsclean2 B=$(BD) O=$(BD)/obj

toolsclean-release:
	@$(MAKE) toolsclean2 B=$(BR) O=$(BR)/obj 

toolsclean2:
	@echo "TOOLS_CLEAN $(B)"
	@rm -f $(TOOLSOBJ)
	@rm -f $(TOOLSOBJ_D_FILES)
	@rm -f $(LBURG) $(DAGCHECK_C) $(Q3RCC) $(Q3CPP) $(Q3LCC) $(Q3ASM)

distclean: clean toolsclean
	@rm -rf $(BUILD_DIR)

installer: release
ifeq ($(PLATFORM),mingw32)
	@$(MAKE) VERSION=$(VERSION) -C $(NSISDIR) V=$(V) \
		USE_RENDERER_DLOPEN=$(USE_RENDERER_DLOPEN) \
		USE_OPENAL_DLOPEN=$(USE_OPENAL_DLOPEN) \
		USE_CURL_DLOPEN=$(USE_CURL_DLOPEN) \
		USE_INTERNAL_SPEEX=$(USE_INTERNAL_SPEEX) \
		USE_INTERNAL_ZLIB=$(USE_INTERNAL_ZLIB) \
		USE_INTERNAL_JPEG=$(USE_INTERNAL_JPEG)
else
	@$(MAKE) VERSION=$(VERSION) -C $(LOKISETUPDIR) V=$(V)
endif

dist:
	git archive HEAD | xz > $(CLIENTBIN)-$(VERSION)-src.tar.xz

#############################################################################
# DEPENDENCIES
#############################################################################

ifneq ($(B),)
  OBJ_D_FILES=$(filter %.d,$(OBJ:%.o=%.d))
  TOOLSOBJ_D_FILES=$(filter %.d,$(TOOLSOBJ:%.o=%.d))
  -include $(OBJ_D_FILES) $(TOOLSOBJ_D_FILES)
endif

.PHONY: all clean clean2 clean-debug clean-release copyfiles \
	debug default dist distclean installer makedirs \
	release targets \
	toolsclean toolsclean2 toolsclean-debug toolsclean-release \
	$(OBJ_D_FILES) $(TOOLSOBJ_D_FILES)