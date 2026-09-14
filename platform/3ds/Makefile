# Based on devkitPro/3ds-examples application template at
# be2001fee08cf8ec7d3366e095e83f6f75da8942.
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

ifeq ($(FRLG_MODE0_REFERENCE_CANDIDATE),1)
ifneq ($(FRLG_NATIVE_GF)$(FRLG_NATIVE_TRACE),11)
$(error FRLG_MODE0_REFERENCE_CANDIDATE=1 requires FRLG_NATIVE_GF=1 and FRLG_NATIVE_TRACE=1)
endif
endif

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
# GRAPHICS is a list of directories containing graphics files
# GFXBUILD is the directory where converted graphics files will be placed
#   If set to $(BUILD), it will statically link in the converted
#   files as if they were data files.
#
# NO_SMDH: if set to anything, no SMDH file is generated.
# ROMFS is the directory which contains the RomFS, relative to the Makefile (Optional)
# APP_TITLE is the name of the app stored in the SMDH file (Optional)
# APP_DESCRIPTION is the description of the app stored in the SMDH file (Optional)
# APP_AUTHOR is the author of the app stored in the SMDH file (Optional)
# ICON is the filename of the icon (.png), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.png
#     - icon.png
#     - <libctru folder>/default_icon.png
#---------------------------------------------------------------------------------
TARGET		:=	frlg-3ds-p2c3
BUILD		:=	build
SOURCES		:=	source ../../compat/src ../../core/src
DATA		:=	data
INCLUDES	:=	include ../../compat/include ../../core/include
ifeq ($(FRLG_NATIVE_GF),1)
TARGET := frlg-3ds-native-gf
BUILD := build-native-gf
ifeq ($(FRLG_NATIVE_TRACE),1)
TARGET := frlg-3ds-native-gf-trace
BUILD := build-native-gf-trace
ifeq ($(FRLG_MODE0_REFERENCE_CANDIDATE),1)
TARGET := frlg-3ds-native-gf-reference-candidate
BUILD := build-native-gf-reference-candidate
endif
endif
SOURCES += ../../compat/upstream-full/src ../../compat/upstream-native/src
INCLUDES := $(INCLUDES) ../../compat/upstream-native/include
ifeq ($(FRLG_MODE0_REFERENCE_CANDIDATE),1)
SOURCES += ../../compat/reference-candidate ../../../../../references/zelda-tmc-3ds/port/ppu/src
INCLUDES := $(INCLUDES) ../../../../../references/zelda-tmc-3ds/port/ppu/include
endif
endif
ifeq ($(DEV_INPUT),1)
TARGET := frlg-3ds-p2c3-dev-input
BUILD := build-dev-input
SOURCES += ../../tests/support
INCLUDES += ../../tests/support
endif
GRAPHICS	:=	gfx
GFXBUILD	:=	$(BUILD)
#ROMFS		:=	romfs
#GFXBUILD	:=	$(ROMFS)/gfx

APP_TITLE		:=	FRLG 3DS P2c3
APP_DESCRIPTION	:=	Mode 0 tile background probe
APP_AUTHOR		:=	Liangzhen-Chen

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -Wall -O2 -mword-relocations \
			-ffunction-sections \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -D__3DS__
ifeq ($(FRLG_NATIVE_GF),1)
CFLAGS += -DFRLG_NATIVE_GF -DFIRERED -DENGLISH -DREVISION=0 -DMODERN=1 -DNDEBUG -fno-strict-aliasing -fdata-sections \
          -iquote $(TOPDIR)/../../compat/upstream-full/include \
          -iquote $(TOPDIR)/../../compat/upstream-native/include \
          -iquote $(TOPDIR)/../../external/pokefirered/include
ifeq ($(FRLG_NATIVE_TRACE),1)
CFLAGS += -DFRLG_NATIVE_STARTUP_TRACE
ifeq ($(FRLG_MODE0_REFERENCE_CANDIDATE),1)
CFLAGS += -DFRLG_GBA_MODE0_REFERENCE_CANDIDATE -DTMC_3DS
endif
endif
endif
ifeq ($(DEV_INPUT),1)
CFLAGS += -DFRLG_DEV_INPUT
endif

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
ifeq ($(FRLG_NATIVE_GF),1)
LDFLAGS += -Wl,--gc-sections
endif

LIBS	:= -lctru -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(CTRULIB)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))
ifeq ($(FRLG_NATIVE_GF),1)
export VPATH += $(CURDIR)/../../external/pokefirered/src
endif

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
ifeq ($(FRLG_NATIVE_GF),1)
CFILES += gpu_regs.c dma3_manager.c task.c malloc.c menu2.c window.c blit.c decompress.c trig.c random.c math_util.c palette.c blend_palette.c \
          play_time.c sound.c load_save.c text_printer.c agb_flash_1m.c
export NATIVE_STARTUP_OBJECTS := $(addprefix $(TOPDIR)/$(BUILD)/startup/,sprite.o intro.o title_screen.o new_menu_helpers.o title_assets.o native_bg.o text.o braille_text.o native_save.o)
export NATIVE_AUDIO_OBJECTS = $(addprefix $(TOPDIR)/$(BUILD)/audio/,$(shell cat $(TOPDIR)/$(BUILD)/audio/objects.txt 2>/dev/null))
endif
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
SHLISTFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.shlist)))
GFXFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.t3s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
ifeq ($(GFXBUILD),$(BUILD))
#---------------------------------------------------------------------------------
export T3XFILES :=  $(GFXFILES:.t3s=.t3x)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
export ROMFS_T3XFILES	:=	$(patsubst %.t3s, $(GFXBUILD)/%.t3x, $(GFXFILES))
export T3XHFILES		:=	$(patsubst %.t3s, $(BUILD)/%.h, $(GFXFILES))
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES_SOURCES 	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES)) \
			$(PICAFILES:.v.pica=.shbin.o) $(SHLISTFILES:.shlist=.shbin.o) \
			$(addsuffix .o,$(T3XFILES))

export OFILES = $(OFILES_BIN) $(OFILES_SOURCES) $(NATIVE_STARTUP_OBJECTS) $(NATIVE_AUDIO_OBJECTS)

export HFILES	:=	$(PICAFILES:.v.pica=_shbin.h) $(SHLISTFILES:.shlist=_shbin.h) \
			$(addsuffix .h,$(subst .,_,$(BINFILES))) \
			$(GFXFILES:.t3s=.h)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export _3DSXDEPS	:=	$(if $(NO_SMDH),,$(OUTPUT).smdh)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.png)
	ifneq (,$(findstring $(TARGET).png,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).png
	else
		ifneq (,$(findstring icon.png,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.png
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_SMDH)),)
	export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
endif

ifneq ($(ROMFS),)
	export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: all clean

#---------------------------------------------------------------------------------
all: $(BUILD) $(GFXBUILD) $(DEPSDIR) $(ROMFS_T3XFILES) $(T3XHFILES) $(if $(filter 1,$(FRLG_NATIVE_GF)),native-startup-objects)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

ifeq ($(FRLG_NATIVE_GF),1)
.PHONY: native-startup-objects
native-startup-objects: $(BUILD)/preproc $(BUILD)/gbagfx
	@FRLG_ENABLE_UPSTREAM_LOCAL=1 \
	  FRLG_NATIVE_PREPROC=$(CURDIR)/$(BUILD)/preproc \
	  FRLG_NATIVE_GBAGFX=$(CURDIR)/$(BUILD)/gbagfx \
	  FRLG_NATIVE_ASSET_ROOT=$(if $(FRLG_NATIVE_ASSET_ROOT),$(FRLG_NATIVE_ASSET_ROOT),$(CURDIR)/../../.tools/frlg-startup-assets) \
	  FRLG_NATIVE_OUTPUT_DIR=$(CURDIR)/$(BUILD)/startup \
	  sh $(CURDIR)/../../tests/upstream/compile-startup-local.sh
	@python3 $(CURDIR)/../../scripts/prepare-native-audio.py \
	  --upstream $(CURDIR)/../../external/pokefirered \
	  --output-dir $(CURDIR)/$(BUILD)/audio

$(BUILD)/preproc: $(wildcard ../../external/pokefirered/tools/preproc/*.cpp) | $(BUILD)
	c++ -std=c++11 -O2 -Wall -Wno-switch -Werror $^ -o $@

$(BUILD)/gbagfx: $(wildcard ../../external/pokefirered/tools/gbagfx/*.c) | $(BUILD)
	cc -Wall -Wextra -Werror -Wno-sign-compare -std=c11 -O3 -DPNG_SKIP_SETJMP_CHECK $^ -o $@ -lpng -lz
endif

$(BUILD):
	@mkdir -p $@

ifneq ($(GFXBUILD),$(BUILD))
$(GFXBUILD):
	@mkdir -p $@
endif

ifneq ($(DEPSDIR),$(BUILD))
$(DEPSDIR):
	@mkdir -p $@
endif

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(OUTPUT).smdh $(TARGET).elf $(GFXBUILD)

#---------------------------------------------------------------------------------
$(GFXBUILD)/%.t3x	$(BUILD)/%.h	:	%.t3s
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@tex3ds -i $< -H $(BUILD)/$*.h -d $(DEPSDIR)/$*.d -o $(GFXBUILD)/$*.t3x

#---------------------------------------------------------------------------------
else

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).3dsx	:	$(OUTPUT).elf $(_3DSXDEPS)

$(OFILES_SOURCES) : $(HFILES)

$(OUTPUT).elf	:	$(OFILES)

ifeq ($(FRLG_NATIVE_GF),1)
agb_flash_1m.o: CFLAGS += -include $(TOPDIR)/../../compat/upstream-full/include/global.h
agb_flash_1m.o: $(TOPDIR)/Makefile
endif

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
.PRECIOUS	:	%.t3x %.shbin
#---------------------------------------------------------------------------------
%.t3x.o	%_t3x.h :	%.t3x
#---------------------------------------------------------------------------------
	$(SILENTMSG) $(notdir $<)
	$(bin2o)

#---------------------------------------------------------------------------------
%.shbin.o %_shbin.h : %.shbin
#---------------------------------------------------------------------------------
	$(SILENTMSG) $(notdir $<)
	$(bin2o)

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
