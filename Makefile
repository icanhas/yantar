# If you require a different configuration from the defaults below, create a
# new file named "Makeconfig" in the same directory as this file and define
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

ifeq ($(COMPILE_ARCH),i386)
  COMPILE_ARCH=x86
endif
ifeq ($(COMPILE_ARCH),x86_64)
  COMPILE_ARCH=amd64
endif
ifeq ($(COMPILE_ARCH),x64)
  COMPILE_ARCH=amd64
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
ifndef BUILD_MISSIONPACK
  BUILD_MISSIONPACK=0
endif
ifndef BUILD_RENDERER_GL2
  BUILD_RENDERER_GL2=1
endif
ifndef BUILD_RENDERER_GL1
  BUILD_RENDERER_GL1=0
endif

ifneq ($(PLATFORM),darwin)
  BUILD_CLIENT_SMP=0
endif

#########

-include Makeconfig

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
	VERSION=0.1.4
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
COPYDIR="/usr/local/games/yantar"
endif

ifndef COPYBINDIR
COPYBINDIR=$(COPYDIR)
endif

ifndef MOUNT_DIR
MOUNT_DIR=src
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

#############################################################################

DP=debug-$(PLATFORM)-$(ARCH)
RP=release-$(PLATFORM)-$(ARCH)
BD=$(BIN_DIR)
BR=$(BIN_DIR)
CDIR=$(MOUNT_DIR)/client
SNDDIR=$(CDIR)/snd
SDIR=$(MOUNT_DIR)/server
RDIR=$(MOUNT_DIR)/renderer
R2DIR=$(MOUNT_DIR)/renderergl2
COMDIR=$(MOUNT_DIR)/qcommon
NETDIR=$(COMDIR)/net
ASMDIR=$(MOUNT_DIR)/asm
SYSDIR=$(MOUNT_DIR)/sys
SDLDIR=$(SYSDIR)/sdl
NDIR=$(SYSDIR)/null
UNIXDIR=$(SYSDIR)/unix
OSXDIR=$(SYSDIR)/osx
WINDIR=$(SYSDIR)/win
GDIR=$(MOUNT_DIR)/game
AIDIR=$(GDIR)/ai
BGDIR=$(GDIR)/bg
CGDIR=$(MOUNT_DIR)/cgame
BLIBDIR=$(MOUNT_DIR)/botlib
UIDIR=$(MOUNT_DIR)/ui
Q3UIDIR=$(MOUNT_DIR)/q3_ui
JPDIR=$(MOUNT_DIR)/jpeg-8c
SPEEXDIR=$(MOUNT_DIR)/libspeex
ZDIR=$(MOUNT_DIR)/zlib
Q3ASMDIR=$(MOUNT_DIR)/cmd/asm
LBURGDIR=$(MOUNT_DIR)/cmd/lcc/lburg
Q3CPPDIR=$(MOUNT_DIR)/cmd/lcc/cpp
Q3LCCETCDIR=$(MOUNT_DIR)/cmd/lcc/etc
Q3LCCSRCDIR=$(MOUNT_DIR)/cmd/lcc/src
LOKISETUPDIR=misc/setup
NSISDIR=misc/nsis
SDLHDIR=$(MOUNT_DIR)/SDL12
LIBSDIR=$(MOUNT_DIR)/libs
INCLUDES=-Iinclude

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
    ifneq ($(BUILD_RENDERER_GL1),0)
      TARGETS += $(B)/ref-gl1-$(SHLIBNAME)
        ifneq ($(BUILD_CLIENT_SMP),0)
          TARGETS += $(B)/ref-gl1-smp-$(SHLIBNAME)
        endif
    endif
    ifneq ($(BUILD_RENDERER_GL2), 0)
      TARGETS += $(B)/ref-gl2-$(SHLIBNAME)
      ifneq ($(BUILD_CLIENT_SMP),0)
        TARGETS += $(B)/ref-gl2-smp-$(SHLIBNAME)
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
    $(B)/$(BASEGAME)/qagame-$(SHLIBNAME) \
    $(B)/$(BASEGAME)/ui-$(SHLIBNAME)
  ifneq ($(BUILD_MISSIONPACK),0)
    TARGETS += \
    $(B)/$(MISSIONPACK)/cgame-$(SHLIBNAME) \
    $(B)/$(MISSIONPACK)/qagame-$(SHLIBNAME) \
    $(B)/$(MISSIONPACK)/ui-$(SHLIBNAME)
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
  DO_QVM_DEP=cat $(@:%.o=%.d) | sed -e 's/\.o/\.asm/g' >> $(@:%.o=%.d)
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

define DO_SHLIB_CC_MISSIONPACK
$(echo_cmd) "SHLIB_CC_MISSIONPACK $<"
$(Q)$(CC) $(MISSIONPACK_CFLAGS) $(SHLIBCFLAGS) $(CFLAGS) $(INCLUDES) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_GAME_CC_MISSIONPACK
$(echo_cmd) "GAME_CC_MISSIONPACK $<"
$(Q)$(CC) $(MISSIONPACK_CFLAGS) -DQAGAME $(SHLIBCFLAGS) $(CFLAGS) $(INCLUDES) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_CGAME_CC_MISSIONPACK
$(echo_cmd) "CGAME_CC_MISSIONPACK $<"
$(Q)$(CC) $(MISSIONPACK_CFLAGS) -DCGAME $(SHLIBCFLAGS) $(CFLAGS) $(INCLUDES) $(OPTIMIZEVM) -o $@ -c $<
$(Q)$(DO_QVM_DEP)
endef

define DO_UI_CC_MISSIONPACK
$(echo_cmd) "UI_CC_MISSIONPACK $<"
$(Q)$(CC) $(MISSIONPACK_CFLAGS) -DUI $(SHLIBCFLAGS) $(CFLAGS) $(INCLUDES) $(OPTIMIZEVM) -o $@ -c $<
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


#############################################################################
# MAIN TARGETS
#############################################################################

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
		$(O)/renderer/sys/sdl \
		$(O)/renderergl2/sys/sdl \
		$(O)/renderersmp/sys/sdl \
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
		$(O)/$(BASEGAME)/qcommon/vmlibc \
		$(O)/$(BASEGAME)/vm \
		$(O)/$(MISSIONPACK)/cgame/bg \
		$(O)/$(MISSIONPACK)/game/ai \
		$(O)/$(MISSIONPACK)/game/bg \
		$(O)/$(MISSIONPACK)/ui/bg \
		$(O)/$(MISSIONPACK)/qcommon/vmlibc \
		$(O)/$(MISSIONPACK)/vm \
		$(O)/cmd/asm \
		$(O)/cmd/etc \
		$(O)/cmd/rcc \
		$(O)/cmd/cpp \
		$(O)/cmd/lburg \
		$(B)/$(BASEGAME)/vm \
		$(B)/$(MISSIONPACK)/vm \
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
$(Q)$(CC) $(TOOLS_CFLAGS)  $(INCLUDES) -o $@ -c $<
endef

define DO_TOOLS_CC_DAGCHECK
$(echo_cmd) "TOOLS_CC_DAGCHECK $<"
$(Q)$(CC) $(TOOLS_CFLAGS) -Wno-unused -o $@ -c $<
endef

LBURG       = $(B)/cmd/lburg$(BINEXT)
DAGCHECK_C  = $(B)/cmd/rcc/dagcheck.c
Q3RCC       = $(B)/cmd/q3rcc$(BINEXT)
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

Q3RCCOBJ = \
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

$(Q3RCC): $(Q3RCCOBJ)
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

$(Q3LCC): $(Q3LCCOBJ) $(Q3RCC) $(Q3CPP)
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

define DO_Q3LCC_MISSIONPACK
$(echo_cmd) "Q3LCC_MISSIONPACK $<"
$(Q)$(Q3LCC) $(MISSIONPACK_CFLAGS) $(INCLUDES) -o $@ $<
endef

define DO_CGAME_Q3LCC_MISSIONPACK
$(echo_cmd) "CGAME_Q3LCC_MISSIONPACK $<"
$(Q)$(Q3LCC) $(MISSIONPACK_CFLAGS) -DCGAME $(INCLUDES) -o $@ $<
endef

define DO_GAME_Q3LCC_MISSIONPACK
$(echo_cmd) "GAME_Q3LCC_MISSIONPACK $<"
$(Q)$(Q3LCC) $(MISSIONPACK_CFLAGS) -DQAGAME $(INCLUDES) -o $@ $<
endef

define DO_UI_Q3LCC_MISSIONPACK
$(echo_cmd) "UI_Q3LCC_MISSIONPACK $<"
$(Q)$(Q3LCC) $(MISSIONPACK_CFLAGS) -DUI $(INCLUDES) -o $@ $<
endef


Q3ASMOBJ = \
  $(O)/cmd/asm/q3asm.o \
  $(O)/cmd/asm/cmdlib.o

$(O)/cmd/asm/%.o: $(Q3ASMDIR)/%.c
	$(DO_TOOLS_CC)

$(Q3ASM): $(Q3ASMOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(TOOLS_CFLAGS) $(TOOLS_LDFLAGS) -o $@ $^ $(TOOLS_LIBS)


#############################################################################
# CLIENT/SERVER
#############################################################################

Q3OBJ = \
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

Q3R2OBJ = \
  $(O)/renderer/image_bmp.o \
  $(O)/renderer/image_jpg.o \
  $(O)/renderer/image_png.o \
  $(O)/renderer/image_tga.o \
  $(O)/renderer/noise.o \
  $(O)/renderergl2/animation.o \
  $(O)/renderergl2/backend.o \
  $(O)/renderergl2/bsp.o \
  $(O)/renderergl2/cmds.o \
  $(O)/renderergl2/curve.o \
  $(O)/renderergl2/extensions.o \
  $(O)/renderergl2/extramath.o \
  $(O)/renderergl2/fbo.o \
  $(O)/renderergl2/flares.o \
  $(O)/renderergl2/font.o \
  $(O)/renderergl2/glsl.o \
  $(O)/renderergl2/image.o \
  $(O)/renderergl2/init.o \
  $(O)/renderergl2/light.o \
  $(O)/renderergl2/main.o \
  $(O)/renderergl2/marks.o \
  $(O)/renderergl2/material.o \
  $(O)/renderergl2/material_parse.o \
  $(O)/renderergl2/mesh.o \
  $(O)/renderergl2/model.o \
  $(O)/renderergl2/model_iqm.o \
  $(O)/renderergl2/postprocess.o \
  $(O)/renderergl2/scene.o \
  $(O)/renderergl2/shade.o \
  $(O)/renderergl2/shade_calc.o \
  $(O)/renderergl2/shadows.o \
  $(O)/renderergl2/sky.o \
  $(O)/renderergl2/surface.o \
  $(O)/renderergl2/vbo.o \
  $(O)/renderergl2/world.o \
  \
  $(O)/renderer/sys/sdl/gamma.o \
  $(O)/renderer/sys/sdl/glimp.o

Q3ROBJ = \
  $(O)/renderer/animation.o \
  $(O)/renderer/backend.o \
  $(O)/renderer/bsp.o \
  $(O)/renderer/cmds.o \
  $(O)/renderer/curve.o \
  $(O)/renderer/flares.o \
  $(O)/renderer/font.o \
  $(O)/renderer/image.o \
  $(O)/renderer/image_bmp.o \
  $(O)/renderer/image_jpg.o \
  $(O)/renderer/image_png.o \
  $(O)/renderer/image_tga.o \
  $(O)/renderer/init.o \
  $(O)/renderer/light.o \
  $(O)/renderer/main.o \
  $(O)/renderer/marks.o \
  $(O)/renderer/mesh.o \
  $(O)/renderer/model.o \
  $(O)/renderer/model_iqm.o \
  $(O)/renderer/noise.o \
  $(O)/renderer/scene.o \
  $(O)/renderer/shade.o \
  $(O)/renderer/shade_calc.o \
  $(O)/renderer/shader.o \
  $(O)/renderer/shadows.o \
  $(O)/renderer/sky.o \
  $(O)/renderer/surface.o \
  $(O)/renderer/world.o \
  \
  $(O)/renderer/sys/sdl/gamma.o \
  $(O)/renderer/sys/sdl/glimp.o

ifneq ($(USE_RENDERER_DLOPEN), 0)
  Q3ROBJ += \
    $(O)/renderer/shared.o \
    $(O)/renderer/puff.o \
    $(O)/renderer/maths.o \
    $(O)/renderer/subs.o

  Q3R2OBJ += \
    $(O)/renderer/shared.o \
    $(O)/renderer/puff.o \
    $(O)/renderer/maths.o \
    $(O)/renderer/subs.o
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

ifeq ($(ARCH),x86)
  Q3OBJ += \
    $(O)/client/snd_mixa.o \
    $(O)/client/matha.o \
    $(O)/client/snapvector.o \
    $(O)/client/ftola.o
endif
ifeq ($(ARCH),amd64)
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
  ifeq ($(ARCH),x86)
    Q3OBJ += \
      $(O)/client/vm/x86.o
  endif
  ifeq ($(ARCH),amd64)
    ifeq ($(USE_OLD_VM64),1)
      Q3OBJ += \
        $(O)/client/vm/amd64.o \
        $(O)/client/vm/amd64_asm.o
    else
      Q3OBJ += \
        $(O)/client/vm/x86.o
    endif
  endif
  ifeq ($(ARCH),ppc)
    Q3OBJ += $(O)/client/vm/powerpc.o $(B)/client/vm_powerpc_asm.o
  endif
  ifeq ($(ARCH),ppc64)
    Q3OBJ += $(O)/client/vm/powerpc.o $(B)/client/vm_powerpc_asm.o
  endif
  ifeq ($(ARCH),sparc)
    Q3OBJ += $(O)/client/vm/sparc.o
  endif
endif

ifeq ($(PLATFORM),mingw32)
  Q3OBJ += \
    $(O)/client/sys/win/res.o \
    $(O)/client/sys/win/sys.o
else
  Q3OBJ += \
    $(O)/client/unix/sys.o
endif

ifeq ($(PLATFORM),darwin)
  Q3OBJ += \
    $(O)/client/osx/sys.o
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

$(B)/ref-gl1-$(SHLIBNAME): $(Q3ROBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3ROBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(RENDERER_LIBS) $(LIBS)

$(B)/ref-gl1-smp-$(SHLIBNAME): $(Q3ROBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3ROBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(RENDERER_LIBS) $(LIBS)

$(B)/ref-gl2-$(SHLIBNAME): $(Q3R2OBJ) $(JPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3R2OBJ) $(JPGOBJ) \
		$(THREAD_LIBS) $(LIBSDLMAIN) $(RENDERER_LIBS) $(LIBS)

$(B)/ref-gl2-smp-$(SHLIBNAME): $(Q3R2OBJ) $(JPGOBJ)
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
  Q3DOBJ += \
      $(O)/ded/matha.o \
      $(O)/ded/snapvector.o \
      $(O)/ded/ftola.o 
endif
ifeq ($(ARCH),amd64)
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
  ifeq ($(ARCH),x86)
    Q3DOBJ += \
      $(O)/ded/vm/x86.o
  endif
  ifeq ($(ARCH),amd64)
    ifeq ($(USE_OLD_VM64),1)
      Q3DOBJ += \
        $(O)/ded/vm/amd64.o \
        $(O)/ded/vm/amd64_asm.o
    else
      Q3DOBJ += \
        $(O)/ded/vm/x86.o
    endif
  endif
  ifeq ($(ARCH),ppc)
    Q3DOBJ += \
      $(O)/ded/vm/powerpc.o \
      $(O)/ded/vm/powerpc_asm.o
  endif
  ifeq ($(ARCH),ppc64)
    Q3DOBJ += \
      $(O)/ded/vm/powerpc.o \
      $(O)/ded/vm/powerpc_asm.o
  endif
  ifeq ($(ARCH),sparc)
    Q3DOBJ += $(O)/ded/vm/sparc.o
  endif
endif

ifeq ($(PLATFORM),mingw32)
  Q3DOBJ += \
    $(O)/ded/sys/win/res.o \
    $(O)/ded/sys/win/sys.o \
    $(O)/ded/sys/win/cons.o
else
  Q3DOBJ += \
    $(O)/ded/sys/unix/sys.o \
    $(O)/ded/sys/unix/cons.o
endif

ifeq ($(PLATFORM),darwin)
  Q3DOBJ += \
    $(O)/ded/sys/osx/sys.o
endif

$(B)/$(SERVERBIN)$(FULLBINEXT): $(Q3DOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(Q3DOBJ) $(LIBS)



#############################################################################
## BASEQ3 CGAME
#############################################################################

Q3CGOBJ_ = \
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
  $(O)/$(BASEGAME)/qcommon/maths.o \
  $(O)/$(BASEGAME)/qcommon/shared.o \
  $(O)/$(BASEGAME)/qcommon/utf.o \
  $(O)/$(BASEGAME)/qcommon/vmlibc/vmlibc.o

Q3CGOBJ = $(Q3CGOBJ_) $(O)/$(BASEGAME)/cgame/syscalls.o
Q3CGVMOBJ = $(Q3CGOBJ_:%.o=%.asm)

$(B)/$(BASEGAME)/cgame-$(SHLIBNAME): $(Q3CGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3CGOBJ)

$(B)/$(BASEGAME)/vm/cgame.qvm: $(Q3CGVMOBJ) $(CGDIR)/syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(Q3CGVMOBJ) $(CGDIR)/syscalls.asm

#############################################################################
## MISSIONPACK CGAME
#############################################################################

MPCGOBJ_ = \
  $(O)/$(MISSIONPACK)/cgame/bg/misc.o \
  $(O)/$(MISSIONPACK)/cgame/bg/pmove.o \
  $(O)/$(MISSIONPACK)/cgame/bg/slidemove.o \
  $(O)/$(MISSIONPACK)/cgame/consolecmds.o \
  $(O)/$(MISSIONPACK)/cgame/draw.o \
  $(O)/$(MISSIONPACK)/cgame/drawtools.o \
  $(O)/$(MISSIONPACK)/cgame/effects.o \
  $(O)/$(MISSIONPACK)/cgame/ents.o \
  $(O)/$(MISSIONPACK)/cgame/event.o \
  $(O)/$(MISSIONPACK)/cgame/info.o \
  $(O)/$(MISSIONPACK)/cgame/localents.o \
  $(O)/$(MISSIONPACK)/cgame/main.o \
  $(O)/$(MISSIONPACK)/cgame/marks.o \
  $(O)/$(MISSIONPACK)/cgame/newdraw.o \
  $(O)/$(MISSIONPACK)/cgame/particles.o \
  $(O)/$(MISSIONPACK)/cgame/players.o \
  $(O)/$(MISSIONPACK)/cgame/playerstate.o \
  $(O)/$(MISSIONPACK)/cgame/predict.o \
  $(O)/$(MISSIONPACK)/cgame/scoreboard.o \
  $(O)/$(MISSIONPACK)/cgame/servercmds.o \
  $(O)/$(MISSIONPACK)/cgame/snapshot.o \
  $(O)/$(MISSIONPACK)/cgame/view.o \
  $(O)/$(MISSIONPACK)/cgame/weapons.o \
  $(O)/$(MISSIONPACK)/ui/shared.o \
  \
  $(O)/$(MISSIONPACK)/qcommon/maths.o \
  $(O)/$(MISSIONPACK)/qcommon/shared.o \
  $(O)/$(MISSIONPACK)/qcommon/utf.o \
  $(O)/$(MISSIONPACK)/qcommon/vmlibc/vmlibc.o

MPCGOBJ = $(MPCGOBJ_) $(O)/$(MISSIONPACK)/cgame/syscalls.o
MPCGVMOBJ = $(MPCGOBJ_:%.o=%.asm)

$(B)/$(MISSIONPACK)/cgame-$(SHLIBNAME): $(MPCGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(MPCGOBJ)

$(B)/$(MISSIONPACK)/vm/cgame.qvm: $(MPCGVMOBJ) $(CGDIR)/syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(MPCGVMOBJ) $(CGDIR)/syscalls.asm



#############################################################################
## BASEQ3 GAME
#############################################################################

Q3GOBJ_ = \
  $(O)/$(BASEGAME)/game/active.o \
  $(O)/$(BASEGAME)/game/ai/chat.o \
  $(O)/$(BASEGAME)/game/ai/cmd.o \
  $(O)/$(BASEGAME)/game/ai/dmnet.o \
  $(O)/$(BASEGAME)/game/ai/dmq3.o \
  $(O)/$(BASEGAME)/game/ai/main.o \
  $(O)/$(BASEGAME)/game/ai/team.o \
  $(O)/$(BASEGAME)/game/ai/vcmd.o \
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
  $(O)/$(BASEGAME)/qcommon/maths.o \
  $(O)/$(BASEGAME)/qcommon/shared.o \
  $(O)/$(BASEGAME)/qcommon/utf.o \
  $(O)/$(BASEGAME)/qcommon/vmlibc/vmlibc.o

Q3GOBJ = $(Q3GOBJ_) $(O)/$(BASEGAME)/game/syscalls.o
Q3GVMOBJ = $(Q3GOBJ_:%.o=%.asm)

$(B)/$(BASEGAME)/qagame-$(SHLIBNAME): $(Q3GOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3GOBJ)

$(B)/$(BASEGAME)/vm/qagame.qvm: $(Q3GVMOBJ) $(GDIR)/syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(Q3GVMOBJ) $(GDIR)/syscalls.asm

#############################################################################
## MISSIONPACK GAME
#############################################################################

MPGOBJ_ = \
  $(O)/$(MISSIONPACK)/game/active.o \
  $(O)/$(MISSIONPACK)/game/ai/chat.o \
  $(O)/$(MISSIONPACK)/game/ai/cmd.o \
  $(O)/$(MISSIONPACK)/game/ai/dmnet.o \
  $(O)/$(MISSIONPACK)/game/ai/dmq3.o \
  $(O)/$(MISSIONPACK)/game/ai/main.o \
  $(O)/$(MISSIONPACK)/game/ai/team.o \
  $(O)/$(MISSIONPACK)/game/ai/vcmd.o \
  $(O)/$(MISSIONPACK)/game/arenas.o \
  $(O)/$(MISSIONPACK)/game/bg/misc.o \
  $(O)/$(MISSIONPACK)/game/bg/pmove.o \
  $(O)/$(MISSIONPACK)/game/bg/slidemove.o \
  $(O)/$(MISSIONPACK)/game/bot.o \
  $(O)/$(MISSIONPACK)/game/client.o \
  $(O)/$(MISSIONPACK)/game/cmds.o \
  $(O)/$(MISSIONPACK)/game/combat.o \
  $(O)/$(MISSIONPACK)/game/items.o \
  $(O)/$(MISSIONPACK)/game/main.o \
  $(O)/$(MISSIONPACK)/game/mem.o \
  $(O)/$(MISSIONPACK)/game/misc.o \
  $(O)/$(MISSIONPACK)/game/missile.o \
  $(O)/$(MISSIONPACK)/game/mover.o \
  $(O)/$(MISSIONPACK)/game/session.o \
  $(O)/$(MISSIONPACK)/game/spawn.o \
  $(O)/$(MISSIONPACK)/game/svcmds.o \
  $(O)/$(MISSIONPACK)/game/target.o \
  $(O)/$(MISSIONPACK)/game/team.o \
  $(O)/$(MISSIONPACK)/game/trigger.o \
  $(O)/$(MISSIONPACK)/game/utils.o \
  $(O)/$(MISSIONPACK)/game/weapon.o \
  \
  $(O)/$(MISSIONPACK)/qcommon/maths.o \
  $(O)/$(MISSIONPACK)/qcommon/shared.o \
  $(O)/$(MISSIONPACK)/qcommon/utf.o \
  $(O)/$(MISSIONPACK)/qcommon/vmlibc/vmlibc.o

MPGOBJ = $(MPGOBJ_) $(O)/$(MISSIONPACK)/game/syscalls.o
MPGVMOBJ = $(MPGOBJ_:%.o=%.asm)

$(B)/$(MISSIONPACK)/qagame-$(SHLIBNAME): $(MPGOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(MPGOBJ)

$(B)/$(MISSIONPACK)/vm/qagame.qvm: $(MPGVMOBJ) $(GDIR)/syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(MPGVMOBJ) $(GDIR)/syscalls.asm



#############################################################################
## BASEQ3 UI
#############################################################################

Q3UIOBJ_ = \
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
  $(O)/$(BASEGAME)/qcommon/maths.o \
  $(O)/$(BASEGAME)/qcommon/shared.o \
  $(O)/$(BASEGAME)/qcommon/utf.o \
  $(O)/$(BASEGAME)/qcommon/vmlibc/vmlibc.o

Q3UIOBJ = $(Q3UIOBJ_) $(O)/$(MISSIONPACK)/ui/syscalls.o
Q3UIVMOBJ = $(Q3UIOBJ_:%.o=%.asm)

$(B)/$(BASEGAME)/ui-$(SHLIBNAME): $(Q3UIOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(Q3UIOBJ)

$(B)/$(BASEGAME)/vm/ui.qvm: $(Q3UIVMOBJ) $(UIDIR)/syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(Q3UIVMOBJ) $(UIDIR)/syscalls.asm

#############################################################################
## MISSIONPACK UI
#############################################################################

MPUIOBJ_ = \
  $(O)/$(MISSIONPACK)/ui/atoms.o \
  $(O)/$(MISSIONPACK)/ui/gameinfo.o \
  $(O)/$(MISSIONPACK)/ui/main.o \
  $(O)/$(MISSIONPACK)/ui/players.o \
  $(O)/$(MISSIONPACK)/ui/shared.o \
  \
  $(O)/$(MISSIONPACK)/ui/bg/misc.o \
  \
  $(O)/$(MISSIONPACK)/qcommon/maths.o \
  $(O)/$(MISSIONPACK)/qcommon/shared.o \
  $(O)/$(MISSIONPACK)/qcommon/utf.o \
  $(O)/$(MISSIONPACK)/qcommon/vmlibc/vmlibc.o

MPUIOBJ = $(MPUIOBJ_) $(O)/$(MISSIONPACK)/ui/syscalls.o
MPUIVMOBJ = $(MPUIOBJ_:%.o=%.asm)

$(B)/$(MISSIONPACK)/ui-$(SHLIBNAME): $(MPUIOBJ)
	$(echo_cmd) "LD $@"
	$(Q)$(CC) $(CFLAGS) $(SHLIBLDFLAGS) -o $@ $(MPUIOBJ)

$(B)/$(MISSIONPACK)/vm/ui.qvm: $(MPUIVMOBJ) $(UIDIR)/syscalls.asm $(Q3ASM)
	$(echo_cmd) "Q3ASM $@"
	$(Q)$(Q3ASM) -o $@ $(MPUIVMOBJ) $(UIDIR)/syscalls.asm



#############################################################################
## CLIENT/SERVER RULES
#############################################################################

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
$(O)/renderersmp/%.o: $(SDLDIR)/%.c
	$(DO_SMP_CC)

$(O)/renderer/%.o: $(COMDIR)/%.c
	$(DO_REF_CC)
$(O)/renderer/sys/sdl/%.o: $(SDLDIR)/%.c
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


#############################################################################
## GAME MODULE RULES
#############################################################################

$(O)/$(BASEGAME)/cgame/bg/%.o: $(BGDIR)/%.c
	$(DO_CGAME_CC)
$(O)/$(BASEGAME)/cgame/%.o: $(CGDIR)/%.c
	$(DO_CGAME_CC)
$(O)/$(BASEGAME)/cgame/bg/%.asm: $(BGDIR)/%.c $(Q3LCC)
	$(DO_CGAME_Q3LCC)
$(O)/$(BASEGAME)/cgame/%.asm: $(CGDIR)/%.c $(Q3LCC)
	$(DO_CGAME_Q3LCC)
$(O)/$(MISSIONPACK)/cgame/bg/%.o: $(BGDIR)/%.c
	$(DO_CGAME_CC_MISSIONPACK)
$(O)/$(MISSIONPACK)/cgame/%.o: $(CGDIR)/%.c
	$(DO_CGAME_CC_MISSIONPACK)
$(O)/$(MISSIONPACK)/cgame/bg/%.asm: $(BGDIR)/%.c $(Q3LCC)
	$(DO_CGAME_Q3LCC_MISSIONPACK)
$(O)/$(MISSIONPACK)/cgame/%.asm: $(CGDIR)/%.c $(Q3LCC)
	$(DO_CGAME_Q3LCC_MISSIONPACK)

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
$(O)/$(MISSIONPACK)/game/%.o: $(GDIR)/%.c
	$(DO_GAME_CC_MISSIONPACK)
$(O)/$(MISSIONPACK)/game/ai/%.o: $(AIDIR)/%.c
	$(DO_GAME_CC_MISSIONPACK)
$(O)/$(MISSIONPACK)/game/bg/%.o: $(BGDIR)/%.c
	$(DO_GAME_CC_MISSIONPACK)
$(O)/$(MISSIONPACK)/game/%.asm: $(GDIR)/%.c $(Q3LCC)
	$(DO_GAME_Q3LCC_MISSIONPACK)
$(O)/$(MISSIONPACK)/game/ai/%.asm: $(AIDIR)/%.c $(Q3LCC)
	$(DO_GAME_Q3LCC_MISSIONPACK)
$(O)/$(MISSIONPACK)/game/bg/%.asm: $(BGDIR)/%.c $(Q3LCC)
	$(DO_GAME_Q3LCC_MISSIONPACK)

$(O)/$(BASEGAME)/ui/bg/%.o: $(BGDIR)/%.c
	$(DO_UI_CC)
$(O)/$(BASEGAME)/ui/%.o: $(Q3UIDIR)/%.c
	$(DO_UI_CC)
$(O)/$(BASEGAME)/ui/bg/%.asm: $(BGDIR)/%.c $(Q3LCC)
	$(DO_UI_Q3LCC)
$(O)/$(BASEGAME)/ui/%.asm: $(Q3UIDIR)/%.c $(Q3LCC)
	$(DO_UI_Q3LCC)
$(O)/$(MISSIONPACK)/ui/bg/%.o: $(BGDIR)/%.c
	$(DO_UI_CC_MISSIONPACK)
$(O)/$(MISSIONPACK)/ui/%.o: $(UIDIR)/%.c
	$(DO_UI_CC_MISSIONPACK)
$(O)/$(MISSIONPACK)/ui/bg/%.asm: $(BGDIR)/%.c $(Q3LCC)
	$(DO_UI_Q3LCC_MISSIONPACK)
$(O)/$(MISSIONPACK)/ui/%.asm: $(UIDIR)/%.c $(Q3LCC)
	$(DO_UI_Q3LCC_MISSIONPACK)

$(O)/$(BASEGAME)/qcommon/%.o: $(COMDIR)/%.c
	$(DO_SHLIB_CC)
$(O)/$(BASEGAME)/qcommon/%.asm: $(COMDIR)/%.c $(Q3LCC)
	$(DO_Q3LCC)
$(O)/$(MISSIONPACK)/qcommon/%.o: $(COMDIR)/%.c
	$(DO_SHLIB_CC_MISSIONPACK)
$(O)/$(MISSIONPACK)/qcommon/%.asm: $(COMDIR)/%.c $(Q3LCC)
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
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/ref-gl1-$(SHLIBNAME) $(COPYBINDIR)/ref-gl1-$(SHLIBNAME)
    endif
    ifneq ($(BUILD_RENDERER_GL2),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/ref-gl2-$(SHLIBNAME) $(COPYBINDIR)/ref-gl2-$(SHLIBNAME)
    endif
  endif
endif

# Don't copy the SMP until it's working together with SDL.
ifneq ($(BUILD_CLIENT_SMP),0)
  ifneq ($(USE_RENDERER_DLOPEN),0)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/ref-gl1-smp-$(SHLIBNAME) $(COPYBINDIR)/ref-gl1-smp-$(SHLIBNAME)
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
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(BASEGAME)/qagame-$(SHLIBNAME) \
					$(COPYDIR)/$(BASEGAME)/.
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(BASEGAME)/ui-$(SHLIBNAME) \
					$(COPYDIR)/$(BASEGAME)/.
  ifneq ($(BUILD_MISSIONPACK),0)
	-$(MKDIR) -p -m 0755 $(COPYDIR)/$(MISSIONPACK)
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(MISSIONPACK)/cgame-$(SHLIBNAME) \
					$(COPYDIR)/$(MISSIONPACK)/.
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(MISSIONPACK)/qagame-$(SHLIBNAME) \
					$(COPYDIR)/$(MISSIONPACK)/.
	$(INSTALL) $(STRIP_FLAG) -m 0755 $(BR)/$(MISSIONPACK)/ui-$(SHLIBNAME) \
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
	@rm -f $(LBURG) $(DAGCHECK_C) $(Q3RCC) $(Q3CPP) $(Q3LCC) $(Q3ASM)

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
	cmdclean cmdclean2 cmdclean-debug cmdclean-release \
	$(OBJ_D_FILES) $(TOOLSOBJ_D_FILES)
