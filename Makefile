#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
.SECONDARY:
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=devkitARM")
endif

GAME_TITLE     := ABISSO DS
GAME_SUBTITLE1 := porting su Nintendo DS
GAME_SUBTITLE2 := roguelike dell'abisso

include $(DEVKITARM)/ds_rules

#---------------------------------------------------------------------------------
TARGET   := abisso
BUILD    := build
SOURCES  := source
DATA     :=
INCLUDES := include
GRAPHICS := graphics

#---------------------------------------------------------------------------------
ARCH := -march=armv5te -mtune=arm946e-s -mthumb

CFLAGS   := -g -Wall -O2 -ffunction-sections -fdata-sections\
            $(ARCH)
CFLAGS   += $(INCLUDE) -DARM9
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS  := -g $(ARCH)
LDFLAGS  = -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS     := -lnds9
LIBDIRS  := $(LIBNDS)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT := $(CURDIR)/$(TARGET)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))\
                $(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir))

export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PNGFILES := $(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.png)))

#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
  export LD := $(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
  export LD := $(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES := $(PNGFILES:.png=.o) \
                 $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE := $(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir))\
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include)\
                  -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds

#---------------------------------------------------------------------------------
else

DEPENDS := $(OFILES:.o=.d)

#---------------------------------------------------------------------------------
$(OUTPUT).nds : $(OUTPUT).elf
$(OUTPUT).elf : $(OFILES)

#---------------------------------------------------------------------------------
%.s %.h : %.png %.grit
#---------------------------------------------------------------------------------
	grit $< -fts -o$*

-include $(DEPENDS)
#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
