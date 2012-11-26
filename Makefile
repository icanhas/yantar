# If you want a different configuration from the defaults below, create
# a new file named 'Makeconfig' in the same directory as this file and
# define your parameters there.

subarch=s/i.86/x86/; s/x86_64/amd64/; s/x64/amd64/
HOSTSYS=$(shell uname | tr '[:upper:]' '[:lower:]' | sed 's/_.*//; s;/;_;g')
HOSTARCH=$(shell uname -m | sed '$(subarch)')

ifeq ($(HOSTSYS),sunos)
  # Solaris uname and GNU uname differ
  HOSTARCH=$(shell uname -p | sed '$(subarch)')
endif
ifeq ($(HOSTSYS),darwin)
  # Apple does some things a little differently...
  HOSTARCH=$(shell uname -p | sed '$(subarch)')
endif

ifndef BUILD_STANDALONE
  BUILD_STANDALONE=1
endif
ifndef BUILD_CLIENT
  BUILD_CLIENT=1
endif
ifndef BUILD_CLIENT_SMP
  BUILD_CLIENT_SMP=0
endif
ifndef BUILD_SERVER
  BUILD_SERVER=1
endif
ifndef BUILD_GAME_SO
  BUILD_GAME_SO=1
endif
ifndef BUILD_GAME_QVM
  BUILD_GAME_QVM=0
endif
ifndef BUILD_BASEGAME
  BUILD_BASEGAME=0
endif
ifndef BUILD_REF2
  BUILD_REF2=1
endif
ifndef BUILD_REF1
  BUILD_REF1=0
endif

ifneq ($(PLATFORM),darwin)
  BUILD_CLIENT_SMP=0
endif

-include Makeconfig

ifndef PLATFORM
PLATFORM=$(HOSTSYS)
endif
export PLATFORM

ifeq ($(HOSTARCH),powerpc)
  HOSTARCH=ppc
endif
ifeq ($(HOSTARCH),powerpc64)
  HOSTARCH=ppc64
endif

ifndef ARCH
ARCH=$(HOSTARCH)
endif
export ARCH

ifneq ($(PLATFORM),$(HOSTSYS))
  CROSS_COMPILING=1
else
  CROSS_COMPILING=0

  ifneq ($(ARCH),$(HOSTARCH))
    CROSS_COMPILING=1
  endif
endif
export CROSS_COMPILING

ifndef VERSION
	VERSION=0.1.5
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

ifndef COPYDIR
COPYDIR="/usr/local/games/yantar"
endif

ifndef COPYBINDIR
COPYBINDIR=$(COPYDIR)
endif

ifndef SRC
SRC=src
endif

ifndef BIN_DIR
BIN_DIR=bin
endif

ifndef OBJ_DIR
OBJ_DIR=.ofiles
endif

ifndef TEMPDIR
TEMPDIR=/tmp
endif

ifndef GENERATE_DEPENDENCIES
GENERATE_DEPENDENCIES=1
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

DP=debug-$(PLATFORM)-$(ARCH)
RP=release-$(PLATFORM)-$(ARCH)
BD=$(BIN_DIR)
BR=$(BIN_DIR)
YAN_DIR=$(SRC)/yantar
CDIR=$(YAN_DIR)/client
SNDDIR=$(CDIR)/snd
SDIR=$(YAN_DIR)/server
RDIR=$(YAN_DIR)/ref-trin
R2DIR=$(YAN_DIR)/ref2
COMDIR=$(YAN_DIR)/common
NETDIR=$(COMDIR)/net
ASMDIR=$(YAN_DIR)/asm
SYSDIR=$(YAN_DIR)/sys
SDLDIR=$(SYSDIR)/sdl
NDIR=$(SYSDIR)/null
UNIXDIR=$(SYSDIR)/unix
OSXDIR=$(SYSDIR)/osx
WINDIR=$(SYSDIR)/win
GDIR=$(YAN_DIR)/game
AIDIR=$(GDIR)/ai
BGDIR=$(GDIR)/bg
CGDIR=$(YAN_DIR)/cgame
BLIBDIR=$(YAN_DIR)/botlib
UIDIR=$(YAN_DIR)/ui
JPDIR=$(YAN_DIR)/jpeg-8c
SPEEXDIR=$(YAN_DIR)/libspeex
ZDIR=$(YAN_DIR)/zlib
Q3ASMDIR=$(SRC)/cmd/asm
LBURGDIR=$(SRC)/cmd/lcc/lburg
Q3CPPDIR=$(SRC)/cmd/lcc/cpp
Q3LCCETCDIR=$(SRC)/cmd/lcc/etc
Q3LCCSRCDIR=$(SRC)/cmd/lcc/src
LOKISETUPDIR=misc/setup
NSISDIR=misc/nsis
SDLHDIR=$(YAN_DIR)/SDL12
LIBSDIR=$(YAN_DIR)/libs
INCLUDES=-Iinclude -Iinclude/yantar

bin_path=$(shell which $(1) 2> /dev/null)

# We won't need this if we only build the server
ifneq ($(BUILD_CLIENT),0)
  # set PKG_CONFIG_PATH to influence this, e.g.
  # PKG_CONFIG_PATH=/opt/cross/i386-mingw32msvc/lib/pkgconfig
  ifneq ($(call bin_path, pkg-config),)
    CURL_CFLAGS=$(shell pkg-config --silence-errors --cflags libcurl)
    CURL_LIBS=$(shell pkg-config --silence-errors --libs libcurl)
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

#
# setup and build (Linux)
#

# Defaults
LIB=lib
INSTALL=install
MKDIR=mkdir

ifneq (,$(findstring "$(PLATFORM)", "linux" "gnu_kfreebsd" "kfreebsd-gnu"))
  ifeq ($(ARCH),axp)
    ARCH=alpha
  else
  ifeq ($(ARCH),amd64)
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

  ifeq ($(ARCH),amd64)
    OPTIMIZEVM = -O3 -fomit-frame-pointer -funroll-loops \
      -falign-loops=2 -falign-jumps=2 -falign-functions=2 \
      -fstrength-reduce
    OPTIMIZE = $(OPTIMIZEVM) -ffast-math
    HAVE_VM_COMPILED = true
  else
  ifeq ($(ARCH),x86)
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

  ifeq ($(ARCH),x86)
    # linux32 make ...
    BASE_CFLAGS += -m32
  else
  ifeq ($(ARCH),ppc64)
    BASE_CFLAGS += -m64
  endif
  endif
else # if Linux

#
# setup and build (OS X)
#

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
  ifeq ($(ARCH),x86)
    OPTIMIZEVM += -march=prescott -mfpmath=sse
    # x86 vm will crash without -mstackrealign since MMX instructions will be
    # used no matter what and they corrupt the frame pointer in VM calls
    BASE_CFLAGS += -arch i386 -m32 -mstackrealign
  endif
  ifeq ($(ARCH),amd64)
    OPTIMIZEVM += -arch x86_64 -mfpmath=sse
  endif

  BASE_CFLAGS += -fno-strict-aliasing -DMACOS_X -fno-common -pipe

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
else # if darwin

#
# setup and build (MinGW)
#

ifeq ($(PLATFORM),mingw32)
  # Some MinGW installations define CC to cc, but don't actually provide cc,
  # so explicitly use gcc instead (which is the only option anyway)
  ifeq ($(call bin_path, $(CC)),)
    CC=gcc
  endif

  ifndef WINDRES
    WINDRES=windres
  endif

  # give windres a target flag to avoid having it detect the host system's arch
  WINDRES_FLAGS=
  ifeq ($(ARCH),x86)
    WINDRES_FLAGS=-Fpe-i386
  endif
  ifeq ($(ARCH),amd64)
      WINDRES_FLAGS=-Fpe-x86-64
  endif

  BASE_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes \
    -DUSE_ICON

  # In the absence of wspiapi.h, require Windows XP or later
  ifeq ($(shell test -e $(COMDIR)/wspiapi.h; echo $$?),1)
    BASE_CFLAGS += -DWINVER=0x501
  endif
  
  ifeq ($(ARCH),amd64)
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
        ifeq ($(ARCH),amd64)
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
else # if mingw32

#
# setup and build (FreeBSD)
#

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

  ifeq ($(USE_CURL),1)
    ifeq ($(USE_CURL_DLOPEN),1)
      CLIENT_LIBS += -lcurl
    endif
  endif

  ifeq ($(USE_CODEC_VORBIS),1)
    CLIENT_LIBS += -lvorbisfile -lvorbis -logg
  endif

  # cross-compiling tweaks
  ifeq ($(ARCH),x86)
    ifeq ($(CROSS_COMPILING),1)
      BASE_CFLAGS += -m32
    endif
  endif
  ifeq ($(ARCH),amd64)
    ifeq ($(CROSS_COMPILING),1)
      BASE_CFLAGS += -m64
    endif
  endif
else # if freebsd

#
# setup and build (OpenBSD)
#

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

  ifeq ($(USE_CODEC_VORBIS),1)
    CLIENT_LIBS += -lvorbisfile -lvorbis -logg
  endif

  ifeq ($(USE_CURL),1) 
    ifneq ($(USE_CURL_DLOPEN),1)
      CLIENT_LIBS += -lcurl
    endif
  endif
else # if openbsd

#
# setup and build (NetBSD)
#

ifeq ($(PLATFORM),netbsd)
  ifeq ($(shell uname -m),i386)
    ARCH=x86
  endif

  LIBS=-lm
  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-shared $(LDFLAGS)
  THREAD_LIBS=-lpthread

  BASE_CFLAGS = -Wall -fno-strict-aliasing -Wimplicit -Wstrict-prototypes

  ifeq ($(ARCH),x86)
    HAVE_VM_COMPILED=true
  endif

  BUILD_CLIENT = 0
else # ifeq netbsd

#
# setup and build (IRIX)
#

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
else # if IRIX

#
# setup and build (SunOS)
#

ifeq ($(PLATFORM),sunos)
  CC=gcc
  INSTALL=ginstall
  MKDIR=gmkdir
  COPYDIR="/usr/local/share/games/quake3"

  ifneq (,$(findstring i86pc,$(shell uname -m)))
    ARCH=x86
  else #default to sparc
    ARCH=sparc
  endif

  ifneq ($(ARCH),x86)
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
  ifeq ($(ARCH),x86)
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
else # if sunos

#
# setup and build (generic)
#

  BASE_CFLAGS=
  OPTIMIZE = -O3

  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-shared

endif #Linux
endif #Darwin
endif #MinGW
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
  FULLBINEXT=-$(ARCH)$(BINEXT)
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
    ifneq ($(BUILD_REF1),0)
      TARGETS += $(B)/ref1-$(SHLIBNAME)
        ifneq ($(BUILD_CLIENT_SMP),0)
          TARGETS += $(B)/ref1-smp-$(SHLIBNAME)
        endif
    endif
    ifneq ($(BUILD_REF2), 0)
      TARGETS += $(B)/ref2-$(SHLIBNAME)
      ifneq ($(BUILD_CLIENT_SMP),0)
        TARGETS += $(B)/ref2-smp-$(SHLIBNAME)
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
    $(B)/$(BASEGAME)/cgame-$(SHLIBNAME) \
    $(B)/$(BASEGAME)/game-$(SHLIBNAME) \
    $(B)/$(BASEGAME)/ui-$(SHLIBNAME)
endif

ifneq ($(BUILD_GAME_QVM),0)
  ifneq ($(CROSS_COMPILING),1)
    TARGETS += \
      $(B)/$(BASEGAME)/vm/cgame.qvm \
      $(B)/$(BASEGAME)/vm/game.qvm \
      $(B)/$(BASEGAME)/vm/ui.qvm
  endif
endif

ifeq ($(USE_CURL),1)
  CLIENT_CFLAGS += -DUSE_CURL -Iinclude/libcurl
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
$(Q)$(CC) $(NOTSHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(INCLUDES) $(OPTIMIZE) -o $@ -c $<
endef

define DO_REF_CC
$(echo_cmd) "REF_CC $<"
$(Q)$(CC) $(SHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(INCLUDES) $(OPTIMIZE) -o $@ -c $<
endef

define DO_SMP_CC
$(echo_cmd) "SMP_CC $<"
$(Q)$(CC) $(SHLIBCFLAGS) $(CFLAGS) $(CLIENT_CFLAGS) $(INCLUDES) $(OPTIMIZE) -DSMP -o $@ -c $<
endef

define DO_BOT_CC
$(echo_cmd) "BOT_CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) $(CFLAGS) $(BOTCFLAGS) $(INCLUDES) $(OPTIMIZE) -DBOTLIB -o $@ -c $<
endef

ifeq ($(GENERATE_DEPENDENCIES),1)
  DO_QVM_DEP=cat $(@:%.o=%.d) | sed 's/\.o/\.asm/g' >> $(@:%.o=%.d)
endif

define DO_SHLIB_CC
$(echo_cmd) "SHLIB_CC $<"
$(Q)$(CC) $(BASEGAME_CFLAGS) $(SHLIBCFLAGS) $(CFLAGS) $(INCLUDES) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_GAME_CC
$(echo_cmd) "GAME_CC $<"
$(Q)$(CC) $(BASEGAME_CFLAGS) -DQAGAME $(SHLIBCFLAGS) $(CFLAGS) $(INCLUDES) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_CGAME_CC
$(echo_cmd) "CGAME_CC $<"
$(Q)$(CC) $(BASEGAME_CFLAGS) -DCGAME $(SHLIBCFLAGS) $(CFLAGS) $(INCLUDES) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_UI_CC
$(echo_cmd) "UI_CC $<"
$(Q)$(CC) $(BASEGAME_CFLAGS) -DUI $(SHLIBCFLAGS) $(CFLAGS) $(INCLUDES) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_AS
$(echo_cmd) "AS $<"
$(Q)$(CC) $(CFLAGS) $(OPTIMIZE) $(INCLUDES) -x assembler-with-cpp -o $@ -c $<
endef

define DO_DED_CC
$(echo_cmd) "DED_CC $<"
$(Q)$(CC) $(NOTSHLIBCFLAGS) -DDEDICATED $(CFLAGS) $(SERVER_CFLAGS) $(INCLUDES) $(OPTIMIZE) -o $@ -c $<
endef

define DO_WINDRES
$(echo_cmd) "WINDRES $<"
$(Q)$(WINDRES) $(WINDRES_FLAGS) -i $< -o $@
endef

#
# main targets
#

default: release
all: debug release

debug:
	@$(MAKE) targets B=$(BIN_DIR) O=$(OBJ_DIR)/$(DP) CFLAGS="$(CFLAGS) $(BASE_CFLAGS) $(DEPEND_CFLAGS)" \
	  OPTIMIZE="$(DEBUG_CFLAGS)" OPTIMIZEVM="$(DEBUG_CFLAGS)" \
	  CLIENT_CFLAGS="$(CLIENT_CFLAGS)" SERVER_CFLAGS="$(SERVER_CFLAGS)" V=$(V)

release:
	@$(MAKE) targets B=$(BIN_DIR) O=$(OBJ_DIR)/$(RP) CFLAGS="$(CFLAGS) $(BASE_CFLAGS) $(DEPEND_CFLAGS)" \
	  OPTIMIZE="-DNDEBUG $(OPTIMIZE)" OPTIMIZEVM="-DNDEBUG $(OPTIMIZEVM)" \
	  CLIENT_CFLAGS="$(CLIENT_CFLAGS)" SERVER_CFLAGS="$(SERVER_CFLAGS)" V=$(V)

# Create the build directories, check libraries, print out
# an informational message (if V=1), then start building
targets: makedirs
ifeq ($(V),1)
	@echo ""
	@echo "Building $(CLIENTBIN) in $(B):"
	@echo "  PLATFORM: $(PLATFORM)"
	@echo "  ARCH: $(ARCH)"
	@echo "  VERSION: $(VERSION)"
	@echo "  HOSTSYS: $(HOSTSYS)"
	@echo "  HOSTARCH: $(HOSTARCH)"
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
endif
ifneq ($(TARGETS),)
	@$(MAKE) $(TARGETS) V=$(V)
endif

makedirs:
	@$(MKDIR) -p \
		$(BIN_DIR) \
		$(OBJ_DIR) \
		$(O)/client/cm \
		$(O)/client/net \
		$(O)/client/server \
		$(O)/client/snd \
		$(O)/client/sys/null \
		$(O)/client/sys/sdl \
		$(O)/client/vm \
		$(O)/ref-trin/sys/sdl \
		$(O)/ref2/sys/sdl \
		$(O)/ref-trinsmp/sys/sdl \
		$(O)/ded/cm \
		$(O)/ded/net \
		$(O)/ded/server \
		$(O)/ded/sys/null \
		$(O)/ded/sys/sdl \
		$(O)/ded/vm \
		$(O)/$(BASEGAME)/cgame/bg \
		$(O)/$(BASEGAME)/game/ai \
		$(O)/$(BASEGAME)/game/bg \
		$(O)/$(BASEGAME)/ui/bg \
		$(O)/$(BASEGAME)/common/vmlibc \
		$(O)/$(BASEGAME)/vm \
		$(O)/cmd/asm \
		$(O)/cmd/etc \
		$(O)/cmd/rcc \
		$(O)/cmd/cpp \
		$(O)/cmd/lburg \
		$(B)/$(BASEGAME)/vm \
		$(B)/cmd/rcc
		
ifeq ($(PLATFORM),mingw32)
	@$(MKDIR) -p $(O)/client/sys/win
	@$(MKDIR) -p $(O)/ded/sys/win
else
	@$(MKDIR) -p $(O)/client/sys/unix
	@$(MKDIR) -p $(O)/ded/sys/unix
endif

ifeq ($(PLATFORM),darwin)
	@$(MKDIR) -p $(O)/client/sys/osx
	@$(MKDIR) -p $(O)/ded/sys/osx
endif

#
# qvm toolchain
#

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
$(Q)$(CC) $(TOOLS_CFLAGS)  $(INCLUDES) -o $@ -c $<
endef

define DO_TOOLS_CC_DAGCHECK
$(echo_cmd) "TOOLS_CC_DAGCHECK $<"
$(Q)$(CC) $(TOOLS_CFLAGS) -Wno-unused -o $@ -c $<
endef

LBURG       = $(B)/cmd/lburg$(BINEXT)
DAGCHECK_C  = $(B)/cmd/rcc/dagcheck.c
RCC       = $(B)/cmd/q3rcc$(BINEXT)
Q3CPP       = $(B)/cmd/q3cpp$(BINEXT)
Q3LCC       = $(B)/cmd/q3lcc$(BINEXT)
Q3ASM       = $(B)/cmd/q3asm$(BINEXT)

LBURGOBJ= \
	$(O)/cmd/lburg/lburg.o \
	$(O)/cmd/lburg/gram.o

$(O)/cmd/lburg/%.o: $(LBURGDIR)/%.c
	$(DO_TOOLS_CC)

$(LBURG): $(LBURGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)

RCCOBJ = \
  $(O)/cmd/rcc/alloc.o \
  $(O)/cmd/rcc/bind.o \
  $(O)/cmd/rcc/bytecode.o \
  $(O)/cmd/rcc/dag.o \
  $(O)/cmd/rcc/dagcheck.o \
  $(O)/cmd/rcc/decl.o \
  $(O)/cmd/rcc/enode.o \
  $(O)/cmd/rcc/error.o \
  $(O)/cmd/rcc/event.o \
  $(O)/cmd/rcc/expr.o \
  $(O)/cmd/rcc/gen.o \
  $(O)/cmd/rcc/init.o \
  $(O)/cmd/rcc/inits.o \
  $(O)/cmd/rcc/input.o \
  $(O)/cmd/rcc/lex.o \
  $(O)/cmd/rcc/list.o \
  $(O)/cmd/rcc/main.o \
  $(O)/cmd/rcc/null.o \
  $(O)/cmd/rcc/output.o \
  $(O)/cmd/rcc/prof.o \
  $(O)/cmd/rcc/profio.o \
  $(O)/cmd/rcc/simp.o \
  $(O)/cmd/rcc/stmt.o \
  $(O)/cmd/rcc/string.o \
  $(O)/cmd/rcc/sym.o \
  $(O)/cmd/rcc/symbolic.o \
  $(O)/cmd/rcc/trace.o \
  $(O)/cmd/rcc/tree.o \
  $(O)/cmd/rcc/types.o

$(DAGCHECK_C): $(LBURG) $(Q3LCCSRCDIR)/dagcheck.md
	$(echo_cmd) "LBURG $(Q3LCCSRCDIR)/dagcheck.md"
	$(Q)$(LBURG) $(Q3LCCSRCDIR)/dagcheck.md $@

$(O)/cmd/rcc/dagcheck.o: $(DAGCHECK_C)
	$(DO_TOOLS_CC_DAGCHECK)

$(O)/cmd/rcc/%.o: $(Q3LCCSRCDIR)/%.c
	$(DO_TOOLS_CC)

$(RCC): $(RCCOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)

Q3CPPOBJ = \
	$(O)/cmd/cpp/cpp.o \
	$(O)/cmd/cpp/eval.o \
	$(O)/cmd/cpp/getopt.o \
	$(O)/cmd/cpp/hideset.o \
	$(O)/cmd/cpp/include.o \
	$(O)/cmd/cpp/lex.o \
	$(O)/cmd/cpp/macro.o \
	$(O)/cmd/cpp/nlist.o \
	$(O)/cmd/cpp/tokens.o \
	$(O)/cmd/cpp/unix.o

$(O)/cmd/cpp/%.o: $(Q3CPPDIR)/%.c
	$(DO_TOOLS_CC)

$(Q3CPP): $(Q3CPPOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)

Q3LCCOBJ = \
	$(O)/cmd/etc/lcc.o \
	$(O)/cmd/etc/bytecode.o

$(O)/cmd/etc/%.o: $(Q3LCCETCDIR)/%.c
	$(DO_TOOLS_CC)

$(Q3LCC): $(Q3LCCOBJ) $(RCC) $(Q3CPP)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $(Q3LCCOBJ) $(TOOLS_LIBS)

define DO_Q3LCC
$(echo_cmd) "Q3LCC $<"
$(Q)$(Q3LCC) $(BASEGAME_CFLAGS) $(INCLUDES) -o $@ $<
endef

define DO_CGAME_Q3LCC
$(echo_cmd) "CGAME_Q3LCC $<"
$(Q)$(Q3LCC) $(BASEGAME_CFLAGS) -DCGAME $(INCLUDES) -o $@ $<
endef

define DO_GAME_Q3LCC
$(echo_cmd) "GAME_Q3LCC $<"
$(Q)$(Q3LCC) $(BASEGAME_CFLAGS) -DQAGAME $(INCLUDES) -o $@ $<
endef

define DO_UI_Q3LCC
$(echo_cmd) "UI_Q3LCC $<"
$(Q)$(Q3LCC) $(BASEGAME_CFLAGS) -DUI $(INCLUDES) -o $@ $<
endef


Q3ASMOBJ = \
  $(O)/cmd/asm/q3asm.o \
  $(O)/cmd/asm/cmdlib.o

$(O)/cmd/asm/%.o: $(Q3ASMDIR)/%.c
	$(DO_TOOLS_CC)

$(Q3ASM): $(Q3ASMOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)

#
# client/server
#

YOBJ = \
  $(O)/client/avi.o \
  $(O)/client/cgame.o \
  $(O)/client/cin.o \
  $(O)/client/console.o \
  $(O)/client/input.o \
  $(O)/client/keys.o \
  $(O)/client/main.o \
  $(O)/client/netchan.o \
  $(O)/client/parse.o \
  $(O)/client/scrn.o \
  $(O)/client/ui.o \
  \
  $(O)/client/cm/load.o \
  $(O)/client/cm/patch.o \
  $(O)/client/cm/polylib.o \
  $(O)/client/cm/test.o \
  $(O)/client/cm/trace.o \
  \
  $(O)/client/cmd.o \
  $(O)/client/common.o \
  $(O)/client/cvar.o \
  $(O)/client/files.o \
  $(O)/client/huffman.o \
  $(O)/client/md4.o \
  $(O)/client/md5.o \
  $(O)/client/bitmsg.o \
  $(O)/client/net/chan.o \
  $(O)/client/net/ip.o \
  \
  $(O)/client/snd/adpcm.o \
  $(O)/client/snd/dma.o \
  $(O)/client/snd/mem.o \
  $(O)/client/snd/mix.o \
  $(O)/client/snd/wavelet.o \
  \
  $(O)/client/snd/codec.o \
  $(O)/client/snd/codec_ogg.o \
  $(O)/client/snd/codec_wav.o \
  $(O)/client/snd/snd.o \
  \
  $(O)/client/curl.o \
  \
  $(O)/client/server/bot.o \
  $(O)/client/server/ccmds.o \
  $(O)/client/server/client.o \
  $(O)/client/server/game.o \
  $(O)/client/server/init.o \
  $(O)/client/server/main.o \
  $(O)/client/server/netchan.o \
  $(O)/client/server/snapshot.o \
  $(O)/client/server/world.o \
  \
  $(O)/client/maths.o \
  $(O)/client/shared.o \
  $(O)/client/utf.o \
  \
  $(O)/client/unzip.o \
  $(O)/client/ioapi.o \
  $(O)/client/puff.o \
  $(O)/client/vm/vm.o \
  $(O)/client/vm/interpreted.o \
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
  $(O)/client/sys/sdl/input.o \
  $(O)/client/sys/sdl/snd.o \
  \
  $(O)/client/sys/conspassive.o \
  $(O)/client/sys/conslog.o \
  $(O)/client/sys/main.o

R2OBJ = \
  $(O)/ref-trin/image_bmp.o \
  $(O)/ref-trin/image_jpg.o \
  $(O)/ref-trin/image_png.o \
  $(O)/ref-trin/image_tga.o \
  $(O)/ref-trin/noise.o \
  $(O)/ref2/animation.o \
  $(O)/ref2/backend.o \
  $(O)/ref2/bsp.o \
  $(O)/ref2/cmds.o \
  $(O)/ref2/curve.o \
  $(O)/ref2/extensions.o \
  $(O)/ref2/extramath.o \
  $(O)/ref2/fbo.o \
  $(O)/ref2/flares.o \
  $(O)/ref2/font.o \
  $(O)/ref2/glsl.o \
  $(O)/ref2/image.o \
  $(O)/ref2/init.o \
  $(O)/ref2/light.o \
  $(O)/ref2/main.o \
  $(O)/ref2/marks.o \
  $(O)/ref2/material.o \
  $(O)/ref2/material_parse.o \
  $(O)/ref2/mesh.o \
  $(O)/ref2/model.o \
  $(O)/ref2/model_iqm.o \
  $(O)/ref2/postprocess.o \
  $(O)/ref2/scene.o \
  $(O)/ref2/shade.o \
  $(O)/ref2/shade_calc.o \
  $(O)/ref2/shadows.o \
  $(O)/ref2/sky.o \
  $(O)/ref2/surface.o \
  $(O)/ref2/vbo.o \
  $(O)/ref2/world.o \
  \
  $(O)/ref-trin/sys/sdl/gamma.o \
  $(O)/ref-trin/sys/sdl/glimp.o

ROBJ = \
  $(O)/ref-trin/animation.o \
  $(O)/ref-trin/backend.o \
  $(O)/ref-trin/bsp.o \
  $(O)/ref-trin/cmds.o \
  $(O)/ref-trin/curve.o \
  $(O)/ref-trin/flares.o \
  $(O)/ref-trin/font.o \
  $(O)/ref-trin/image.o \
  $(O)/ref-trin/image_bmp.o \
  $(O)/ref-trin/image_jpg.o \
  $(O)/ref-trin/image_png.o \
  $(O)/ref-trin/image_tga.o \
  $(O)/ref-trin/init.o \
  $(O)/ref-trin/light.o \
  $(O)/ref-trin/main.o \
  $(O)/ref-trin/marks.o \
  $(O)/ref-trin/mesh.o \
  $(O)/ref-trin/model.o \
  $(O)/ref-trin/model_iqm.o \
  $(O)/ref-trin/noise.o \
  $(O)/ref-trin/scene.o \
  $(O)/ref-trin/shade.o \
  $(O)/ref-trin/shade_calc.o \
  $(O)/ref-trin/shader.o \
  $(O)/ref-trin/shadows.o \
  $(O)/ref-trin/sky.o \
  $(O)/ref-trin/surface.o \
  $(O)/ref-trin/world.o \
  \
  $(O)/ref-trin/sys/sdl/gamma.o \
  $(O)/ref-trin/sys/sdl/glimp.o

ifneq ($(USE_RENDERER_DLOPEN), 0)
  ROBJ += \
    $(O)/ref-trin/shared.o \
    $(O)/ref-trin/puff.o \
    $(O)/ref-trin/maths.o \
    $(O)/ref-trin/subs.o

  R2OBJ += \
    $(O)/ref-trin/shared.o \
    $(O)/ref-trin/puff.o \
    $(O)/ref-trin/maths.o \
    $(O)/ref-trin/subs.o
endif

ifneq ($(USE_INTERNAL_JPEG),0)
  JPGOBJ = \
    $(O)/ref-trin/jaricom.o \
    $(O)/ref-trin/jcapimin.o \
    $(O)/ref-trin/jcapistd.o \
    $(O)/ref-trin/jcarith.o \
    $(O)/ref-trin/jccoefct.o  \
    $(O)/ref-trin/jccolor.o \
    $(O)/ref-trin/jcdctmgr.o \
    $(O)/ref-trin/jchuff.o   \
    $(O)/ref-trin/jcinit.o \
    $(O)/ref-trin/jcmainct.o \
    $(O)/ref-trin/jcmarker.o \
    $(O)/ref-trin/jcmaster.o \
    $(O)/ref-trin/jcomapi.o \
    $(O)/ref-trin/jcparam.o \
    $(O)/ref-trin/jcprepct.o \
    $(O)/ref-trin/jcsample.o \
    $(O)/ref-trin/jctrans.o \
    $(O)/ref-trin/jdapimin.o \
    $(O)/ref-trin/jdapistd.o \
    $(O)/ref-trin/jdarith.o \
    $(O)/ref-trin/jdatadst.o \
    $(O)/ref-trin/jdatasrc.o \
    $(O)/ref-trin/jdcoefct.o \
    $(O)/ref-trin/jdcolor.o \
    $(O)/ref-trin/jddctmgr.o \
    $(O)/ref-trin/jdhuff.o \
    $(O)/ref-trin/jdinput.o \
    $(O)/ref-trin/jdmainct.o \
    $(O)/ref-trin/jdmarker.o \
    $(O)/ref-trin/jdmaster.o \
    $(O)/ref-trin/jdmerge.o \
    $(O)/ref-trin/jdpostct.o \
    $(O)/ref-trin/jdsample.o \
    $(O)/ref-trin/jdtrans.o \
    $(O)/ref-trin/jerror.o \
    $(O)/ref-trin/jfdctflt.o \
    $(O)/ref-trin/jfdctfst.o \
    $(O)/ref-trin/jfdctint.o \
    $(O)/ref-trin/jidctflt.o \
    $(O)/ref-trin/jidctfst.o \
    $(O)/ref-trin/jidctint.o \
    $(O)/ref-trin/jmemmgr.o \
    $(O)/ref-trin/jmemnobs.o \
    $(O)/ref-trin/jquant1.o \
    $(O)/ref-trin/jquant2.o \
    $(O)/ref-trin/jutils.o
endif

ifeq ($(ARCH),x86)
  YOBJ += \
    $(O)/client/snd_mixa.o \
    $(O)/client/matha.o \
    $(O)/client/snapvector.o \
    $(O)/client/ftola.o
endif
ifeq ($(ARCH),amd64)
  YOBJ += \
    $(O)/client/snapvector.o \
    $(O)/client/ftola.o
endif

ifeq ($(USE_VOIP),1)
ifeq ($(USE_INTERNAL_SPEEX),1)
YOBJ += \
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
YOBJ += \
  $(O)/client/adler32.o \
  $(O)/client/crc32.o \
  $(O)/client/inffast.o \
  $(O)/client/inflate.o \
  $(O)/client/inftrees.o \
  $(O)/client/zutil.o
endif

ifeq ($(HAVE_VM_COMPILED),true)
  ifeq ($(ARCH),x86)
    YOBJ += \
      $(O)/client/vm/x86.o
  endif
  ifeq ($(ARCH),amd64)
    ifeq ($(USE_OLD_VM64),1)
      YOBJ += \
        $(O)/client/vm/amd64.o \
        $(O)/client/vm/amd64_asm.o
    else
      YOBJ += \
        $(O)/client/vm/x86.o
    endif
  endif
  ifeq ($(ARCH),ppc)
    YOBJ += $(O)/client/vm/powerpc.o $(B)/client/vm_powerpc_asm.o
  endif
  ifeq ($(ARCH),ppc64)
    YOBJ += $(O)/client/vm/powerpc.o $(B)/client/vm_powerpc_asm.o
  endif
  ifeq ($(ARCH),sparc)
    YOBJ += $(O)/client/vm/sparc.o
  endif
endif

ifeq ($(PLATFORM),mingw32)
  YOBJ += \
    $(O)/client/sys/win/res.o \
    $(O)/client/sys/win/sys.o
else
  YOBJ += \
    $(O)/client/sys/unix/sys.o
endif

ifeq ($(PLATFORM),darwin)
  YOBJ += \
    $(O)/client/sys/osx/sys.o
endif

ifeq ($(USE_MUMBLE),1)
  YOBJ += \
    $(O)/client/libmumblelink.o
endif

ifneq ($(USE_RENDERER_DLOPEN),0)
# make renderer shared libraries (gross)
$(B)/$(CLIENTBIN)$(FULLBINEXT): $(YOBJ) $(LIBSDLMAIN)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CLIENT_CFLAGS) $(CFLAGS) $(CLIENT_LDFLAGS) $(LDFLAGS) \
		-o $@ $(YOBJ) \
		$(LIBSDLMAIN) $(CLIENT_LIBS) $(LIBS)

$(B)/ref1-$(SHLIBNAME): $(ROBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(ROBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(RENDERER_LIBS) $(LIBS)

$(B)/ref1-smp-$(SHLIBNAME): $(ROBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(ROBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(RENDERER_LIBS) $(LIBS)

$(B)/ref2-$(SHLIBNAME): $(R2OBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(R2OBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(RENDERER_LIBS) $(LIBS)

$(B)/ref2-smp-$(SHLIBNAME): $(R2OBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(R2OBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(RENDERER_LIBS) $(LIBS)

else
# static link renderer
$(B)/$(CLIENTBIN)$(FULLBINEXT): $(YOBJ) $(R2OBJ) $(LIBSDLMAIN) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CLIENT_CFLAGS) $(CFLAGS) $(CLIENT_LDFLAGS) $(LDFLAGS) \
		-o $@ $(YOBJ) $(R2OBJ) $(JPGOBJ) \
		$(LIBSDLMAIN) $(CLIENT_LIBS) $(RENDERER_LIBS) $(LIBS)

$(B)/$(CLIENTBIN)-smp$(FULLBINEXT): $(YOBJ) $(R2OBJ) $(LIBSDLMAIN) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CLIENT_CFLAGS) $(CFLAGS) $(CLIENT_LDFLAGS) $(LDFLAGS) $(THREAD_LDFLAGS) \
		-o $@ $(YOBJ) $(R2OBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(CLIENT_LIBS) $(RENDERER_LIBS) $(LIBS)
endif

ifneq ($(strip $(LIBSDLMAIN)),)
ifneq ($(strip $(LIBSDLMAINSRC)),)
$(LIBSDLMAIN) : $(LIBSDLMAINSRC)
	cp $< $@
	ranlib $@
endif
endif

#
# dedicated server
#

YDOBJ = \
  $(O)/ded/server/bot.o \
  $(O)/ded/server/client.o \
  $(O)/ded/server/ccmds.o \
  $(O)/ded/server/game.o \
  $(O)/ded/server/init.o \
  $(O)/ded/server/main.o \
  $(O)/ded/server/netchan.o \
  $(O)/ded/server/snapshot.o \
  $(O)/ded/server/world.o \
  \
  $(O)/ded/cm/load.o \
  $(O)/ded/cm/patch.o \
  $(O)/ded/cm/polylib.o \
  $(O)/ded/cm/test.o \
  $(O)/ded/cm/trace.o \
  $(O)/ded/bitmsg.o \
  $(O)/ded/cmd.o \
  $(O)/ded/common.o \
  $(O)/ded/cvar.o \
  $(O)/ded/files.o \
  $(O)/ded/huffman.o \
  $(O)/ded/md4.o \
  $(O)/ded/net/chan.o \
  $(O)/ded/net/ip.o \
  \
  $(O)/ded/maths.o \
  $(O)/ded/shared.o \
  $(O)/ded/utf.o \
  \
  $(O)/ded/unzip.o \
  $(O)/ded/ioapi.o \
  $(O)/ded/vm/interpreted.o \
  $(O)/ded/vm/vm.o \
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
  $(O)/ded/sys/null/client.o \
  $(O)/ded/sys/null/input.o \
  $(O)/ded/sys/null/snddma.o \
  \
  $(O)/ded/sys/conslog.o \
  $(O)/ded/sys/main.o

ifeq ($(ARCH),x86)
  YDOBJ += \
      $(O)/ded/matha.o \
      $(O)/ded/snapvector.o \
      $(O)/ded/ftola.o 
endif
ifeq ($(ARCH),amd64)
  YDOBJ += \
      $(O)/ded/snapvector.o \
      $(O)/ded/ftola.o 
endif

ifeq ($(USE_INTERNAL_ZLIB),1)
YDOBJ += \
  $(O)/ded/adler32.o \
  $(O)/ded/crc32.o \
  $(O)/ded/inffast.o \
  $(O)/ded/inflate.o \
  $(O)/ded/inftrees.o \
  $(O)/ded/zutil.o
endif

ifeq ($(HAVE_VM_COMPILED),true)
  ifeq ($(ARCH),x86)
    YDOBJ += \
      $(O)/ded/vm/x86.o
  endif
  ifeq ($(ARCH),amd64)
    ifeq ($(USE_OLD_VM64),1)
      YDOBJ += \
        $(O)/ded/vm/amd64.o \
        $(O)/ded/vm/amd64_asm.o
    else
      YDOBJ += \
        $(O)/ded/vm/x86.o
    endif
  endif
  ifeq ($(ARCH),ppc)
    YDOBJ += \
      $(O)/ded/vm/powerpc.o \
      $(O)/ded/vm/powerpc_asm.o
  endif
  ifeq ($(ARCH),ppc64)
    YDOBJ += \
      $(O)/ded/vm/powerpc.o \
      $(O)/ded/vm/powerpc_asm.o
  endif
  ifeq ($(ARCH),sparc)
    YDOBJ += $(O)/ded/vm/sparc.o
  endif
endif

ifeq ($(PLATFORM),mingw32)
  YDOBJ += \
    $(O)/ded/sys/win/res.o \
    $(O)/ded/sys/win/sys.o \
    $(O)/ded/sys/win/cons.o
else
  YDOBJ += \
    $(O)/ded/sys/unix/sys.o \
    $(O)/ded/sys/unix/cons.o
endif

ifeq ($(PLATFORM),darwin)
  YDOBJ += \
    $(O)/ded/sys/osx/sys.o
endif

$(B)/$(SERVERBIN)$(FULLBINEXT): $(YDOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(YDOBJ) $(LIBS)

#
# base cgame
#

CGOBJ_ = \
  $(O)/$(BASEGAME)/cgame/bg/misc.o \
  $(O)/$(BASEGAME)/cgame/bg/pmove.o \
  $(O)/$(BASEGAME)/cgame/bg/slidemove.o \
  $(O)/$(BASEGAME)/cgame/consolecmds.o \
  $(O)/$(BASEGAME)/cgame/draw.o \
  $(O)/$(BASEGAME)/cgame/drawtools.o \
  $(O)/$(BASEGAME)/cgame/effects.o \
  $(O)/$(BASEGAME)/cgame/ents.o \
  $(O)/$(BASEGAME)/cgame/event.o \
  $(O)/$(BASEGAME)/cgame/info.o \
  $(O)/$(BASEGAME)/cgame/localents.o \
  $(O)/$(BASEGAME)/cgame/main.o \
  $(O)/$(BASEGAME)/cgame/marks.o \
  $(O)/$(BASEGAME)/cgame/particles.o \
  $(O)/$(BASEGAME)/cgame/players.o \
  $(O)/$(BASEGAME)/cgame/playerstate.o \
  $(O)/$(BASEGAME)/cgame/predict.o \
  $(O)/$(BASEGAME)/cgame/scoreboard.o \
  $(O)/$(BASEGAME)/cgame/servercmds.o \
  $(O)/$(BASEGAME)/cgame/snapshot.o \
  $(O)/$(BASEGAME)/cgame/view.o \
  $(O)/$(BASEGAME)/cgame/weapons.o \
  \
  $(O)/$(BASEGAME)/common/maths.o \
  $(O)/$(BASEGAME)/common/shared.o \
  $(O)/$(BASEGAME)/common/utf.o \
  $(O)/$(BASEGAME)/common/vmlibc/vmlibc.o

CGOBJ = $(CGOBJ_) $(O)/$(BASEGAME)/cgame/syscalls.o
CGVMOBJ = $(CGOBJ_:%.o=%.asm)

$(B)/$(BASEGAME)/cgame-$(SHLIBNAME): $(CGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(CGOBJ)

$(B)/$(BASEGAME)/vm/cgame.qvm: $(CGVMOBJ) $(CGDIR)/syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(CGVMOBJ) $(CGDIR)/syscalls.asm

#
# base game
#

GOBJ_ = \
  $(O)/$(BASEGAME)/game/active.o \
  $(O)/$(BASEGAME)/game/ai/chat.o \
  $(O)/$(BASEGAME)/game/ai/cmd.o \
  $(O)/$(BASEGAME)/game/ai/dmnet.o \
  $(O)/$(BASEGAME)/game/ai/dmq3.o \
  $(O)/$(BASEGAME)/game/ai/main.o \
  $(O)/$(BASEGAME)/game/ai/team.o \
  $(O)/$(BASEGAME)/game/arenas.o \
  $(O)/$(BASEGAME)/game/bg/misc.o \
  $(O)/$(BASEGAME)/game/bg/pmove.o \
  $(O)/$(BASEGAME)/game/bg/slidemove.o \
  $(O)/$(BASEGAME)/game/bot.o \
  $(O)/$(BASEGAME)/game/client.o \
  $(O)/$(BASEGAME)/game/cmds.o \
  $(O)/$(BASEGAME)/game/combat.o \
  $(O)/$(BASEGAME)/game/items.o \
  $(O)/$(BASEGAME)/game/main.o \
  $(O)/$(BASEGAME)/game/mem.o \
  $(O)/$(BASEGAME)/game/misc.o \
  $(O)/$(BASEGAME)/game/missile.o \
  $(O)/$(BASEGAME)/game/mover.o \
  $(O)/$(BASEGAME)/game/session.o \
  $(O)/$(BASEGAME)/game/spawn.o \
  $(O)/$(BASEGAME)/game/svcmds.o \
  $(O)/$(BASEGAME)/game/target.o \
  $(O)/$(BASEGAME)/game/team.o \
  $(O)/$(BASEGAME)/game/trigger.o \
  $(O)/$(BASEGAME)/game/utils.o \
  $(O)/$(BASEGAME)/game/weapon.o \
  \
  $(O)/$(BASEGAME)/common/maths.o \
  $(O)/$(BASEGAME)/common/shared.o \
  $(O)/$(BASEGAME)/common/utf.o \
  $(O)/$(BASEGAME)/common/vmlibc/vmlibc.o

GOBJ = $(GOBJ_) $(O)/$(BASEGAME)/game/syscalls.o
GVMOBJ = $(GOBJ_:%.o=%.asm)

$(B)/$(BASEGAME)/game-$(SHLIBNAME): $(GOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(GOBJ)

$(B)/$(BASEGAME)/vm/game.qvm: $(GVMOBJ) $(GDIR)/syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(GVMOBJ) $(GDIR)/syscalls.asm

#
# base ui
#

UIOBJ_ = \
  $(O)/$(BASEGAME)/ui/bg/misc.o \
  $(O)/$(BASEGAME)/ui/addbots.o \
  $(O)/$(BASEGAME)/ui/atoms.o \
  $(O)/$(BASEGAME)/ui/cdkey.o \
  $(O)/$(BASEGAME)/ui/cinematics.o \
  $(O)/$(BASEGAME)/ui/confirm.o \
  $(O)/$(BASEGAME)/ui/connect.o \
  $(O)/$(BASEGAME)/ui/controls2.o \
  $(O)/$(BASEGAME)/ui/credits.o \
  $(O)/$(BASEGAME)/ui/demo2.o \
  $(O)/$(BASEGAME)/ui/display.o \
  $(O)/$(BASEGAME)/ui/gameinfo.o \
  $(O)/$(BASEGAME)/ui/ingame.o \
  $(O)/$(BASEGAME)/ui/loadconfig.o \
  $(O)/$(BASEGAME)/ui/main.o \
  $(O)/$(BASEGAME)/ui/menu.o \
  $(O)/$(BASEGAME)/ui/mfield.o \
  $(O)/$(BASEGAME)/ui/mods.o \
  $(O)/$(BASEGAME)/ui/network.o \
  $(O)/$(BASEGAME)/ui/options.o \
  $(O)/$(BASEGAME)/ui/playermodel.o \
  $(O)/$(BASEGAME)/ui/players.o \
  $(O)/$(BASEGAME)/ui/playersettings.o \
  $(O)/$(BASEGAME)/ui/preferences.o \
  $(O)/$(BASEGAME)/ui/qmenu.o \
  $(O)/$(BASEGAME)/ui/removebots.o \
  $(O)/$(BASEGAME)/ui/saveconfig.o \
  $(O)/$(BASEGAME)/ui/serverinfo.o \
  $(O)/$(BASEGAME)/ui/servers2.o \
  $(O)/$(BASEGAME)/ui/setup.o \
  $(O)/$(BASEGAME)/ui/sound.o \
  $(O)/$(BASEGAME)/ui/sparena.o \
  $(O)/$(BASEGAME)/ui/specifyserver.o \
  $(O)/$(BASEGAME)/ui/splevel.o \
  $(O)/$(BASEGAME)/ui/sppostgame.o \
  $(O)/$(BASEGAME)/ui/spskill.o \
  $(O)/$(BASEGAME)/ui/startserver.o \
  $(O)/$(BASEGAME)/ui/team.o \
  $(O)/$(BASEGAME)/ui/teamorders.o \
  $(O)/$(BASEGAME)/ui/video.o \
  \
  $(O)/$(BASEGAME)/common/maths.o \
  $(O)/$(BASEGAME)/common/shared.o \
  $(O)/$(BASEGAME)/common/utf.o \
  $(O)/$(BASEGAME)/common/vmlibc/vmlibc.o

UIOBJ = $(UIOBJ_) $(O)/$(BASEGAME)/ui/syscalls.o
UIVMOBJ = $(UIOBJ_:%.o=%.asm)

$(B)/$(BASEGAME)/ui-$(SHLIBNAME): $(UIOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(UIOBJ)

$(B)/$(BASEGAME)/vm/ui.qvm: $(UIVMOBJ) $(UIDIR)/syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(UIVMOBJ) $(UIDIR)/syscalls.asm

#
# client/server rules
#

$(O)/client/%.o: $(ASMDIR)/%.s
	$(DO_AS)
# k8 so inline assembler knows about SSE
$(O)/client/%.o: $(ASMDIR)/%.c
	$(DO_CC) -march=k8
$(O)/client/%.o: $(COMDIR)/%.c
	$(DO_CC)
$(O)/client/%.o: $(BLIBDIR)/%.c
	$(DO_BOT_CC)
$(O)/client/%.o: $(SPEEXDIR)/%.c
	$(DO_CC)
$(O)/client/%.o: $(ZDIR)/%.c
	$(DO_CC)
$(O)/client/%.o: $(CDIR)/%.c
	$(DO_CC)
$(O)/client/net/%.o: $(NETDIR)/%.c
	$(DO_CC)
$(O)/client/server/%.o: $(SDIR)/%.c
	$(DO_CC)
$(O)/client/snd/%.o: $(SNDDIR)/%.c
	$(DO_CC)
$(O)/client/sys/null/%.o: $(NDIR)/%.c
	$(DO_CC)
$(O)/client/sys/osx/%.o: $(OSXDIR)/%.c
	$(DO_CC)
$(O)/client/sys/osx/%.o: $(OSXDIR)/%.m
	$(DO_CC)
$(O)/client/sys/unix/%.o: $(UNIXDIR)/%.c
	$(DO_CC)
$(O)/client/sys/win/%.o: $(WINDIR)/%.c
	$(DO_CC)
$(O)/client/sys/win/%.o: $(WINDIR)/%.rc
	$(DO_WINDRES)
$(O)/client/sys/%.o: $(SYSDIR)/%.c
	$(DO_CC)
$(O)/client/sys/sdl/%.o: $(SDLDIR)/%.c
	$(DO_CC)
$(O)/ref-trinsmp/%.o: $(SDLDIR)/%.c
	$(DO_SMP_CC)

$(O)/ref-trin/%.o: $(COMDIR)/%.c
	$(DO_REF_CC)
$(O)/ref-trin/sys/sdl/%.o: $(SDLDIR)/%.c
	$(DO_REF_CC)
$(O)/ref-trin/%.o: $(JPDIR)/%.c
	$(DO_REF_CC)
$(O)/ref-trin/%.o: $(RDIR)/%.c
	$(DO_REF_CC)
$(O)/ref2/%.o: $(R2DIR)/%.c
	$(DO_REF_CC)

$(O)/ded/%.o: $(ASMDIR)/%.s
	$(DO_AS)
# k8 so inline assembler knows about SSE
$(O)/ded/%.o: $(ASMDIR)/%.c
	$(DO_CC) -march=k8
$(O)/ded/%.o: $(COMDIR)/%.c
	$(DO_DED_CC)
$(O)/ded/%.o: $(ZDIR)/%.c
	$(DO_DED_CC)
$(O)/ded/%.o: $(BLIBDIR)/%.c
	$(DO_BOT_CC)
$(O)/ded/net/%.o: $(NETDIR)/%.c
	$(DO_DED_CC)
$(O)/ded/server/%.o: $(SDIR)/%.c
	$(DO_DED_CC)
$(O)/ded/sys/null/%.o: $(NDIR)/%.c
	$(DO_DED_CC)
$(O)/ded/sys/osx/%.o: $(OSXDIR)/%.c
	$(DO_DED_CC)
$(O)/ded/sys/unix/%.o: $(UNIXDIR)/%.c
	$(DO_DED_CC)
$(O)/ded/sys/win/%.o: $(WINDIR)/%.c
	$(DO_DED_CC)
$(O)/ded/sys/win/%.o: $(WINDIR)/%.rc
	$(DO_WINDRES)
$(O)/ded/sys/%.o: $(SYSDIR)/%.c
	$(DO_DED_CC)
$(O)/ded/sys/sdl/%.o: $(SDLDIR)/%.c
	$(DO_DED_CC)
$(O)/ded/sys/osx/%.o: $(OSXDIR)/%.m
	$(DO_DED_CC)

#
# game module rules
#

$(O)/$(BASEGAME)/cgame/bg/%.o: $(BGDIR)/%.c
	$(DO_CGAME_CC)
$(O)/$(BASEGAME)/cgame/%.o: $(CGDIR)/%.c
	$(DO_CGAME_CC)
$(O)/$(BASEGAME)/cgame/bg/%.asm: $(BGDIR)/%.c $(Q3LCC)
	$(DO_CGAME_Q3LCC)
$(O)/$(BASEGAME)/cgame/%.asm: $(CGDIR)/%.c $(Q3LCC)
	$(DO_CGAME_Q3LCC)

$(O)/$(BASEGAME)/game/%.o: $(GDIR)/%.c
	$(DO_GAME_CC)
$(O)/$(BASEGAME)/game/ai/%.o: $(AIDIR)/%.c
	$(DO_GAME_CC)
$(O)/$(BASEGAME)/game/bg/%.o: $(BGDIR)/%.c
	$(DO_GAME_CC)
$(O)/$(BASEGAME)/game/%.asm: $(GDIR)/%.c $(Q3LCC)
	$(DO_GAME_Q3LCC)
$(O)/$(BASEGAME)/game/ai/%.asm: $(AIDIR)/%.c $(Q3LCC)
	$(DO_GAME_Q3LCC)
$(O)/$(BASEGAME)/game/bg/%.asm: $(BGDIR)/%.c $(Q3LCC)
	$(DO_GAME_Q3LCC)

$(O)/$(BASEGAME)/ui/bg/%.o: $(BGDIR)/%.c
	$(DO_UI_CC)
$(O)/$(BASEGAME)/ui/%.o: $(UIDIR)/%.c
	$(DO_UI_CC)
$(O)/$(BASEGAME)/ui/bg/%.asm: $(BGDIR)/%.c $(Q3LCC)
	$(DO_UI_Q3LCC)
$(O)/$(BASEGAME)/ui/%.asm: $(UIDIR)/%.c $(Q3LCC)
	$(DO_UI_Q3LCC)

$(O)/$(BASEGAME)/common/%.o: $(COMDIR)/%.c
	$(DO_SHLIB_CC)
$(O)/$(BASEGAME)/common/%.asm: $(COMDIR)/%.c $(Q3LCC)
	$(DO_Q3LCC)

#
# misc
#

OBJ = $(YOBJ) $(ROBJ) $(R2OBJ) $(YDOBJ) $(JPGOBJ) \
  $(GOBJ) $(CGOBJ) $(UIOBJ) \
  $(GVMOBJ) $(CGVMOBJ) $(UIVMOBJ)
TOOLSOBJ = $(LBURGOBJ) $(Q3CPPOBJ) $(RCCOBJ) $(Q3LCCOBJ) $(Q3ASMOBJ)


copyfiles: release
	@if [ ! -d $(COPYDIR)/$(BASEGAME) ]; then echo "You need to set COPYDIR to where your Quake3 data is!"; fi
ifneq ($(BUILD_GAME_SO),0)
  ifneq ($(BUILD_BASEGAME), 0)
		-$(MKDIR) -p -m 0755 $(COPYDIR)/$(BASEGAME)
  endif
endif

ifneq ($(BUILD_CLIENT),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(CLIENTBIN)$(FULLBINEXT) $(COPYBINDIR)/$(CLIENTBIN)$(FULLBINEXT)
  ifneq ($(USE_RENDERER_DLOPEN),0)
    ifneq ($(BUILD_REF1),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/ref1-$(SHLIBNAME) $(COPYBINDIR)/ref1-$(SHLIBNAME)
    endif
    ifneq ($(BUILD_REF2),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/ref2-$(SHLIBNAME) $(COPYBINDIR)/ref2-$(SHLIBNAME)
    endif
  endif
endif

# Don't copy the SMP until it's working together with SDL.
ifneq ($(BUILD_CLIENT_SMP),0)
  ifneq ($(USE_RENDERER_DLOPEN),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/ref1-smp-$(SHLIBNAME) $(COPYBINDIR)/ref1-smp-$(SHLIBNAME)
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
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(BASEGAME)/cgame-$(SHLIBNAME) \
					$(COPYDIR)/$(BASEGAME)/.
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(BASEGAME)/game-$(SHLIBNAME) \
					$(COPYDIR)/$(BASEGAME)/.
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(BASEGAME)/ui-$(SHLIBNAME) \
					$(COPYDIR)/$(BASEGAME)/.
endif

clean: clean-debug clean-release
ifeq ($(PLATFORM),mingw32)
	@$(MAKE) -C $(NSISDIR) clean
else
	@$(MAKE) -C $(LOKISETUPDIR) clean
endif

clean-debug:
	@$(MAKE) clean2 B=$(BIN_DIR) O=$(OBJ_DIR)/$(DP)

clean-release:
	@$(MAKE) clean2 B=$(BIN_DIR) O=$(OBJ_DIR)/$(RP)

clean2:
	@echo "clean $(B) & $(O)"
	@rm -f $(OBJ)
	@rm -f $(OBJ_D_FILES)
	@rm -f $(TARGETS)

cmdclean: cmdclean-debug cmdclean-release

cmdclean-debug:
	@$(MAKE) cmdclean2 B=$(BIN_DIR) O=$(OBJ_DIR)/$(DP)

cmdclean-release:
	@$(MAKE) cmdclean2 B=$(BIN_DIR) O=$(OBJ_DIR)/$(RP)

cmdclean2:
	@echo "CMD_CLEAN $(B)"
	@rm -f $(TOOLSOBJ)
	@rm -f $(TOOLSOBJ_D_FILES)
	@rm -f $(LBURG) $(DAGCHECK_C) $(RCC) $(Q3CPP) $(Q3LCC) $(Q3ASM)

distclean: clean cmdclean
	@rm -rf $(BIN_DIR)

installer: release
ifeq ($(PLATFORM),mingw32)
	@$(MAKE) VERSION=$(VERSION) -C $(NSISDIR) V=$(V) \
		USE_RENDERER_DLOPEN=$(USE_RENDERER_DLOPEN) \
		USE_CURL_DLOPEN=$(USE_CURL_DLOPEN) \
		USE_INTERNAL_SPEEX=$(USE_INTERNAL_SPEEX) \
		USE_INTERNAL_ZLIB=$(USE_INTERNAL_ZLIB) \
		USE_INTERNAL_JPEG=$(USE_INTERNAL_JPEG)
else
	@$(MAKE) VERSION=$(VERSION) -C $(LOKISETUPDIR) V=$(V)
endif

dist:
	git archive HEAD | xz > $(CLIENTBIN)-$(VERSION)-src.tar.xz

#
# dependencies
#

ifneq ($(B),)
  OBJ_D_FILES=$(filter %.d,$(OBJ:%.o=%.d))
  TOOLSOBJ_D_FILES=$(filter %.d,$(TOOLSOBJ:%.o=%.d))
  -include $(OBJ_D_FILES) $(TOOLSOBJ_D_FILES)
endif

.PHONY: all clean clean2 clean-debug clean-release copyfiles \
	debug default dist distclean installer makedirs \
	release targets \
	cmdclean cmdclean2 cmdclean-debug cmdclean-release \
	$(OBJ_D_FILES) $(TOOLSOBJ_D_FILES)
