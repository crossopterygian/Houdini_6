### 1. General Configuration
### ==========================================================================

### Establish the operating system name
UNAME = $(shell uname)

### Executable name
EXE = h.exe

### Installation dir definitions
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

### Built-in benchmark for pgo-builds
ifeq ($(PTIME),)
	PTIME=1000
endif
PGOBENCH = ./$(EXE) bench 32 6 $(PTIME) default time

### Object files
OBJS =

ifneq ($(VERSION),Dev)
	OBJS += egtb_nalimov.o egtb_nalimovprobe.o 
endif

versmelt = yes

ifeq ($(versmelt),yes)
	OBJS += versmelting.o
else
	OBJS += zoeken.o evaluatie.o \
		bord.o egtb_kpk.o eval_eindspel.o eval_materiaal.o eval_pionstruct.o \
		eval_pst.o main.o stelling.o uci.o uci_opties.o uci_tijd.o \
		util_bench.o util_tools.o zet_generatie.o zet_hash.o \
		zet_keuze.o zoek_smp.o syzygy/tbprobe.o \
		egtb_syzygyprobe.o licentie.o 
endif

COBJS = zlib/adler32.o zlib/crc32.o zlib/inffast.o zlib/inflate.o \
	zlib/inftrees.o zlib/uncompr.o zlib/zutil.o \
	polarssl/aes.o polarssl/md5.o polarssl/padlock.o

### 2.1. General and architecture defaults
### ==========================================================================

optimize = yes
debug = no
bits = 32
prefetch = no
popcnt = no
sse = no
pext = no

### 2.2 Architecture specific
### ==========================================================================

ifeq ($(ARCH),general-32)
	arch = any
endif

ifeq ($(ARCH),x86-32-old)
	arch = i386
endif

ifeq ($(ARCH),x86-32)
	arch = i386
	prefetch = yes
	sse = yes
endif

ifeq ($(ARCH),x86-32-modern)
	arch = i386
	prefetch = yes
	popcnt = yes
	sse = yes
endif

ifeq ($(ARCH),general-64)
	arch = any
	bits = 64
endif

ifeq ($(ARCH),x86-64)
	arch = x86_64
	bits = 64
	prefetch = yes
	sse = yes
endif

ifeq ($(ARCH),x86-64-modern)
	arch = x86_64
	bits = 64
	prefetch = yes
	popcnt = yes
	sse = yes
endif

ifeq ($(ARCH),native)
	arch = native
	bits = 64
	prefetch = yes
	popcnt = yes
	sse = yes
endif

ifeq ($(ARCH),x86-64-bmi2)
	arch = x86_64
	bits = 64
	prefetch = yes
	popcnt = yes
	sse = yes
	pext = yes
endif

ifeq ($(ARCH),armv7)
	arch = armv7
	prefetch = yes
endif

ifeq ($(ARCH),ppc-32)
	arch = ppc
endif

ifeq ($(ARCH),ppc-64)
	arch = ppc64
	bits = 64
endif

### 2.3 Compile versions
### ==========================================================================

ifneq ($(VERSION),Dev)
	CXXFLAGS += -DUSE_LICENSE
endif

ifeq ($(VERSION),Pro)
	CXXFLAGS += -DPREMIUM
endif

ifeq ($(VERSION),Std)
	CXXFLAGS += -DPRO_LICENSE
endif

family = none

ifeq ($(FAMILY),ChessBase)
	family = ChessBase
	CXXFLAGS += -DCHESSBASE
	ifeq ($(bits),64)
		LDFLAGS += -L. -l:libprot64.a -lole32
	else
		LDFLAGS += -L. -l:libprot32.a -lole32
	endif
endif

ifeq ($(FAMILY),Aquarium)
	family = ChessOK
	CXXFLAGS += -DAQUARIUM
endif

ifeq ($(FAMILY),ChessKing)
	family = ChessOK
	CXXFLAGS += -DCHESSKING
endif

ifeq ($(FAMILY),ChessAssistant)
	family = ChessOK
	CXXFLAGS += -DCHESS_ASSISTANT
endif


### 3.1 Selecting compiler (default = gcc)
### ==========================================================================

CXXFLAGS += -Wall -Wcast-qual -fno-exceptions $(EXTRACXXFLAGS)
DEPENDFLAGS += -std=c++11
LDFLAGS += $(EXTRALDFLAGS)
CC = gcc

ifeq ($(COMP),)
	COMP=gcc
endif

ifeq ($(COMP),gcc)
	comp=gcc
	CXX=g++
	CXXFLAGS += -pedantic -Wextra -Wshadow -m$(bits)
	ifneq ($(UNAME),Darwin)
		LDFLAGS += -Wl,--no-as-needed
	endif
endif

ifeq ($(COMP),mingw)
	comp=mingw

	ifeq ($(UNAME),Linux)
		ifeq ($(bits),64)
			ifeq ($(shell which x86_64-w64-mingw32-c++-posix),)
				CXX=x86_64-w64-mingw32-c++
			else
				CXX=x86_64-w64-mingw32-c++-posix
			endif
		else
			ifeq ($(shell which i686-w64-mingw32-c++-posix),)
				CXX=i686-w64-mingw32-c++
			else
				CXX=i686-w64-mingw32-c++-posix
			endif
		endif
	else
		CXX=g++
	endif

	CXXFLAGS += -Wextra -Wshadow
	LDFLAGS += -static
endif

ifeq ($(COMP),icc)
	comp=icc
	CXX=icpc
	CXXFLAGS += -diag-disable 1476,10120 -Wcheck -Wabi -Wdeprecated -strict-ansi
endif

ifeq ($(COMP),clang)
	comp=clang
	CXX=clang++
	CXXFLAGS += -pedantic -Wextra -Wshadow -m$(bits)
	LDFLAGS += -m$(bits)
	ifeq ($(UNAME),Darwin)
		CXXFLAGS += -stdlib=libc++
		DEPENDFLAGS += -stdlib=libc++
	endif
endif

ifeq ($(comp),icc)
	profile_prepare = icc-profile-prepare
	profile_make = icc-profile-make
	profile_use = icc-profile-use
	profile_clean = icc-profile-clean
else
	profile_prepare = gcc-profile-prepare
	profile_make = gcc-profile-make
	profile_use = gcc-profile-use
	profile_clean = gcc-profile-clean
endif

ifeq ($(UNAME),Darwin)
	CXXFLAGS += -arch $(arch) -mmacosx-version-min=10.9
	LDFLAGS += -arch $(arch) -mmacosx-version-min=10.9
endif

### Travis CI script uses COMPILER to overwrite CXX
ifdef COMPILER
	COMPCXX=$(COMPILER)
endif

### Allow overwriting CXX from command line
ifdef COMPCXX
	CXX=$(COMPCXX)
endif

### On mingw use Windows threads, otherwise POSIX
ifneq ($(comp),mingw)
	# On Android Bionic's C library comes with its own pthread implementation bundled in
	ifneq ($(arch),armv7)
		# Haiku has pthreads in its libroot, so only link it in on other platforms
		ifneq ($(UNAME),Haiku)
			LDFLAGS += -lpthread
		endif
	endif
endif

### 3.2 Debugging
### ==========================================================================
ifeq ($(debug),no)
	CXXFLAGS += -DNDEBUG
else
	CXXFLAGS += -g
endif

### 3.3 Optimization
### ==========================================================================
ifeq ($(optimize),yes)
	CXXFLAGS += -O3

	ifeq ($(comp),gcc)
		ifeq ($(UNAME),Darwin)
			ifeq ($(arch),i386)
				CXXFLAGS += -mdynamic-no-pic
			endif
			ifeq ($(arch),x86_64)
				CXXFLAGS += -mdynamic-no-pic
			endif
		endif

		ifeq ($(arch),armv7)
			CXXFLAGS += -fno-gcse -mthumb -march=armv7-a -mfloat-abi=softfp
		endif
	endif

	ifeq ($(arch),native)
		CXXFLAGS += -march=native -mtune=native
	endif

	ifeq ($(comp),icc)
		ifeq ($(UNAME),Darwin)
			CXXFLAGS += -mdynamic-no-pic
		endif
	endif

	ifeq ($(comp),clang)
		ifeq ($(UNAME),Darwin)
			ifeq ($(pext),no)
				CXXFLAGS += -flto
				LDFLAGS += $(CXXFLAGS)
			endif
			ifeq ($(arch),i386)
				CXXFLAGS += -mdynamic-no-pic
			endif
			ifeq ($(arch),x86_64)
				CXXFLAGS += -mdynamic-no-pic
			endif
		endif
	endif
endif

### 3.4 Bits
### ==========================================================================
ifeq ($(bits),64)
	CXXFLAGS += -DIS_64BIT
endif

### 3.5 prefetch
### ==========================================================================
ifeq ($(prefetch),yes)
	ifeq ($(sse),yes)
		CXXFLAGS += -msse
		DEPENDFLAGS += -msse
	endif
else
	CXXFLAGS += -DNO_PREFETCH
endif

### 3.6 popcnt
### ==========================================================================
ifeq ($(popcnt),yes)
	ifeq ($(comp),icc)
		CXXFLAGS += -msse3 -DUSE_POPCNT
	else
	ifeq ($(bits),64)
		CXXFLAGS += -msse3 -mpopcnt -DUSE_POPCNT
	else
		CXXFLAGS += -mpopcnt -DUSE_POPCNT
	endif
	endif
endif

### 3.7 pext
### ==========================================================================
ifeq ($(pext),yes)
	CXXFLAGS += -DUSE_PEXT
	ifeq ($(comp),$(filter $(comp),gcc clang mingw))
		CXXFLAGS += -mbmi -mbmi2
	endif
endif

### 3.8 Link Time Optimization, it works since gcc 4.5 but not on mingw under Windows.
### This is a mix of compile and link time options because the lto link phase
### needs access to the optimization flags.
### ==========================================================================
ifeq ($(comp),gcc)
	ifeq ($(optimize),yes)
	ifeq ($(debug),no)
		CXXFLAGS += -flto
		LDFLAGS += $(CXXFLAGS)
	endif
	endif
endif

ifeq ($(comp),mingw)
	ifeq ($(UNAME),Linux)
	ifeq ($(optimize),yes)
	ifeq ($(debug),no)
		CXXFLAGS += -flto
		LDFLAGS += $(CXXFLAGS)
	endif
	endif
	endif
endif

### 3.9 Android 5 can only run position independent executables. Note that this
### breaks Android 4.0 and earlier.
### ==========================================================================
ifeq ($(arch),armv7)
	CXXFLAGS += -fPIE
	LDFLAGS += -fPIE -pie
endif

### 3.10 Differentiate c++ and c compiles
### ==========================================================================
CFLAGS := $(CXXFLAGS)
CXXFLAGS += -fno-rtti -std=c++11

### 4. Public targets
### ==========================================================================

help:
	@echo ""
	@echo "To compile stockfish, type: "
	@echo ""
	@echo "make target ARCH=arch [COMP=compiler] [COMPCXX=cxx]"
	@echo ""
	@echo "Supported targets:"
	@echo ""
	@echo "build                   > Standard build"
	@echo "profile-build           > PGO build"
	@echo "strip                   > Strip executable"
	@echo "install                 > Install executable"
	@echo "clean                   > Clean up"
	@echo ""
	@echo "Supported archs:"
	@echo ""
	@echo "x86-64                  > x86 64-bit"
	@echo "x86-64-modern           > x86 64-bit with popcnt support"
	@echo "x86-64-bmi2             > x86 64-bit with pext support"
	@echo "x86-32                  > x86 32-bit with SSE support"
	@echo "x86-32-old              > x86 32-bit fall back for old hardware"
	@echo "ppc-64                  > PPC 64-bit"
	@echo "ppc-32                  > PPC 32-bit"
	@echo "armv7                   > ARMv7 32-bit"
	@echo "general-64              > unspecified 64-bit"
	@echo "general-32              > unspecified 32-bit"
	@echo ""
	@echo "Supported compilers:"
	@echo ""
	@echo "gcc                     > Gnu compiler (default)"
	@echo "mingw                   > Gnu compiler with MinGW under Windows"
	@echo "clang                   > LLVM Clang compiler"
	@echo "icc                     > Intel compiler"
	@echo ""
	@echo "Simple examples. If you don't know what to do, you likely want to run: "
	@echo ""
	@echo "make build ARCH=x86-64    (This is for 64-bit systems)"
	@echo "make build ARCH=x86-32    (This is for 32-bit systems)"
	@echo ""
	@echo "Advanced examples, for experienced users: "
	@echo ""
	@echo "make build ARCH=x86-64 COMP=clang"
	@echo "make profile-build ARCH=x86-64-modern COMP=gcc COMPCXX=g++-4.8"
	@echo ""


.PHONY: build profile-build
build:
	$(MAKE) ARCH=$(ARCH) COMP=$(COMP) config-sanity
	$(MAKE) ARCH=$(ARCH) COMP=$(COMP) all

profile-build:
	$(MAKE) ARCH=$(ARCH) COMP=$(COMP) config-sanity
	@echo ""
	@echo "Step 0/4. Preparing for profile build."
	$(MAKE) ARCH=$(ARCH) COMP=$(COMP) $(profile_prepare)
	@echo ""
	@echo "Step 1/4. Building executable for benchmark ..."
	$(MAKE) -B ARCH=$(ARCH) COMP=$(COMP) $(profile_make)
	@echo ""
	@echo "Step 2a/4. Inject Tables and FileSize ..."
ifeq ($(family),ChessBase)
	./InjectTables-H6-ChessBase.exe $(EXE)
else ifeq ($(family)$(VERSION),ChessOKPro)
	./InjectTables-H6-ChessOKPro.exe $(EXE)
else ifeq ($(family)$(VERSION),ChessOKStd)
	./InjectTables-H6-ChessOKPro.exe $(EXE)
else ifeq ($(family),ChessOK)
	./InjectTables-H6-ChessOK.exe $(EXE)
else ifeq ($(VERSION),Pro)
	./InjectTables-H6Pro.exe $(EXE)
else ifeq ($(VERSION),Std)
	./InjectTables-H6Pro.exe $(EXE)
else ifneq ($(VERSION),Dev)
	./InjectTables-H6.exe $(EXE)
endif
ifneq ($(VERSION),Dev)
	./$(EXE) z? >nul 2>aes.dat
	./InjectFileSize-H6.exe $(EXE) aes.dat
endif
	@echo "Step 2b/4. Running benchmark for pgo-build ..."
	$(PGOBENCH) >nul
	@echo ""
	@echo "Step 3/4. Building final executable ..."
	$(MAKE) -B ARCH=$(ARCH) COMP=$(COMP) $(profile_use)
	@echo ""
	@echo "Step 4/4. Deleting profile data ..."
	$(MAKE) ARCH=$(ARCH) COMP=$(COMP) $(profile_clean)
	$(MAKE) strip

strip:
	strip $(EXE)

install:
	-mkdir -p -m 755 $(BINDIR)
	-cp $(EXE) $(BINDIR)
	-strip $(BINDIR)/$(EXE)

clean:
	$(RM) $(EXE) $(EXE).exe *.o .depend *.gcda ./syzygy/*.o ./syzygy/*.gcda \
	zlib/*.o zlib/*.gcda polarssl/*.o polarssl/*.gcda

default:
	help

### Section 5. Private targets
### ==========================================================================

all: $(EXE) .depend

config-sanity:
	@echo ""
	@echo "Config:"
	@echo "debug: '$(debug)'"
	@echo "optimize: '$(optimize)'"
	@echo "arch: '$(arch)'"
	@echo "bits: '$(bits)'"
	@echo "prefetch: '$(prefetch)'"
	@echo "popcnt: '$(popcnt)'"
	@echo "sse: '$(sse)'"
	@echo "pext: '$(pext)'"
	@echo ""
	@echo "Compiler:"
	@echo "CXX: $(CXX)"
	@echo "CXXFLAGS: $(CXXFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo ""
	@echo "Testing config sanity. If this fails, try 'make help' ..."
	@echo ""
	@test "$(debug)" = "yes" || test "$(debug)" = "no"
	@test "$(optimize)" = "yes" || test "$(optimize)" = "no"
	@test "$(arch)" = "any" || test "$(arch)" = "x86_64" || test "$(arch)" = "i386" || \
	 test "$(arch)" = "ppc64" || test "$(arch)" = "ppc" || test "$(arch)" = "armv7" || test "$(arch)" = "native"
	@test "$(bits)" = "32" || test "$(bits)" = "64"
	@test "$(prefetch)" = "yes" || test "$(prefetch)" = "no"
	@test "$(popcnt)" = "yes" || test "$(popcnt)" = "no"
	@test "$(sse)" = "yes" || test "$(sse)" = "no"
	@test "$(pext)" = "yes" || test "$(pext)" = "no"
	@test "$(comp)" = "gcc" || test "$(comp)" = "icc" || test "$(comp)" = "mingw" || test "$(comp)" = "clang"

$(EXE): $(OBJS) $(COBJS)
	$(CXX) -o $@ $(OBJS) $(COBJS) $(LDFLAGS)

$(COBJS): %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

gcc-profile-prepare:
	$(MAKE) ARCH=$(ARCH) COMP=$(COMP) gcc-profile-clean

gcc-profile-make:
	$(MAKE) ARCH=$(ARCH) COMP=$(COMP) \
	EXTRACXXFLAGS='-fprofile-generate' \
	EXTRALDFLAGS='-lgcov' \
	all

gcc-profile-use:
	$(MAKE) ARCH=$(ARCH) COMP=$(COMP) \
	EXTRACXXFLAGS='-fprofile-use -fno-peel-loops -fno-tracer -fprofile-correction' \
	EXTRALDFLAGS='-lgcov -Wl,-Map,h.map' \
	all

gcc-profile-clean:
	@rm -rf *.gcda syzygy/*.gcda zlib/*.gcda polarssl/*.gcda
	@rm -rf *.o syzygy/*.o zlib/*.o polarssl/*.o

icc-profile-prepare:
	$(MAKE) ARCH=$(ARCH) COMP=$(COMP) icc-profile-clean
	@mkdir profdir

icc-profile-make:
	$(MAKE) ARCH=$(ARCH) COMP=$(COMP) \
	EXTRACXXFLAGS='-prof-gen=srcpos -prof_dir ./profdir' \
	all

icc-profile-use:
	$(MAKE) ARCH=$(ARCH) COMP=$(COMP) \
	EXTRACXXFLAGS='-prof_use -prof_dir ./profdir' \
	all

icc-profile-clean:
	@rm -rf profdir bench.txt

.depend:
	-@$(CXX) $(DEPENDFLAGS) -MM $(OBJS:.o=.cpp) $(COBJS:.o=.c) > $@ 2> /dev/null

-include .depend
