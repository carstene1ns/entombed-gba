#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif

include $(DEVKITARM)/gba_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output, if this ends with _mb a multiboot image is generated
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET	:=	$(shell basename $(CURDIR))
BUILD		:=	build
SOURCES	:=	src src/gfx
DATA		:=	
INCLUDES	:=	include

GDB_DIRS   =    $cdir:$cwd:$(ADD_GDB_SOURCE_DIRS)

ifeq ($(MAKECMDGOALS),gdb)
DEBUG_SET   :=   gdb
endif

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
#ARCH	:=	-mthumb -mthumb-interwork 
ARCH	:=	-mthumb-interwork
ifneq ($(DEBUG_SET),gdb)
THIS_BUILD   :=   "Release Build"
#CFLAGS	:= 	-g -Wall -O2\
#		-mcpu=arm7tdmi -mtune=arm7tdmi\
#		-fomit-frame-pointer\
#		-ffast-math \
#		$(ARCH)

CFLAGS	:= -mlong-calls -Wall -O3 -fomit-frame-pointer -ffast-math \
           -save-temps -fverbose-asm $(ARCH)

else
THIS_BUILD   :=   "Debug Build"
#CFLAGS   :=   -g -Wall -O0 -save-temps \
#             -mcpu=arm7tdmi -mtune=arm7tdmi\
#             -ffast-math \
#             $(ARCH)
#CFLAGS	:= -gdwarf-2 -gstrict-dwarf -Wall -save-temps -fverbose-asm $(ARCH)
CFLAGS	:= -gdwarf-2 -Wall -save-temps -fverbose-asm $(ARCH)
endif

CFLAGS	+=	$(INCLUDE)


ifneq ($(DEBUG_SET),gdb)
CXXFLAGS	:=	$(CFLAGS) -fno-rtti -fno-exceptions	
else
CXXFLAGS	:=	$(CFLAGS)
endif

ASFLAGS	:=	$(ARCH)

ifneq ($(DEBUG_SET),gdb)
LDFLAGS   =   $(ARCH) -Wl,-Map,$(notdir $@).map
else
#LDFLAGS   =   -g $(ARCH) -Wl,-Map,$(notdir $@).map
LDFLAGS   =   -gdwarf-2 -gstrict-dwarf $(ARCH) -Wl,-Map,$(notdir $@).map
endif


#---------------------------------------------------------------------------------
# path to tools - this can be deleted if you set the path to the toolchain in windows
#---------------------------------------------------------------------------------
export PATH	:=	$(DEVKITARM)/bin:$(PATH)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
ifneq ($(DEBUG_SET),gdb)
LIBS	:= -lmm -lgba -ltonc
else
LIBS	:= -lmm -lgba -ltonc
endif

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBGBA) $(LIBTONC) $(DEVKITARM) ..

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
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

export OFILES	:= $(addsuffix .o,$(BINFILES)) $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD) \

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make "DEBUG_SET=$(DEBUG_SET)" --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

all	: buildtype $(BUILD)

buildtype:
		@echo "Build type: " $(THIS_BUILD)

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).gba
	
test:
	echo hello
	
#---------------------------------------------------------------------------------
startvba: 
	$(DEVKITPRO)/tools/vba/vba.exe -2 -Gtcp:44444 &

#---------------------------------------------------------------------------------
makeini:	
	echo "File $(TARGET).elf" > insight.ini
	echo "set remotelogfile log.log" >>insight.ini
	echo "target remote 127.0.0.1:44444" >>insight.ini
	echo "load $(TARGET).elf" >>insight.ini
	echo "b main" >>insight.ini
	echo "directory $(GDB_DIRS)">>insight.ini
	echo "c" >>insight.ini

#---------------------------------------------------------------------------------
startinsight:
	$(DEVKITPRO)/insight/bin/arm-eabi-insight.exe --command=insight.ini $(TARGET).elf

#---------------------------------------------------------------------------------	
 gdb: clean
	@make "DEBUG_SET=$(DEBUG_SET)" all
	make startvba makeini startinsight

#---------------------------------------------------------------------------------
rungdb: startvba makeini startinsight

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).gba	:	$(OUTPUT).elf

$(OUTPUT).elf	:	$(OFILES)

%.o   :   %.bin
	@echo   $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
