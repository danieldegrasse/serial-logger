###### CHANGE LOCATION TO YOUR CODEGEN TOOLS INSTALL DIR. UNIX PATH (no backslashes) #######
CODEGEN_INSTALL_DIR = /home/danieldegrasse/Downloads/gcc-arm-none-eabi-4_7-2012q4

CC = "$(CODEGEN_INSTALL_DIR)/bin/arm-none-eabi-gcc"
LNK = "$(CODEGEN_INSTALL_DIR)/bin/arm-none-eabi-gcc"

XDC_INSTALL_DIR := /home/danieldegrasse/ti/xdctools_3_32_00_06_core
TIRTOS_INSTALL_DIR := /home/danieldegrasse/ti/tirtos_tivac_2_16_00_08
TIDRIVERS_INSTALL_DIR := $(TIRTOS_INSTALL_DIR)/products/tidrivers_tivac_2_16_00_08
BIOS_INSTALL_DIR := $(TIRTOS_INSTALL_DIR)/products/bios_6_45_01_29
NDK_INSTALL_DIR := $(TIRTOS_INSTALL_DIR)/products/ndk_2_25_00_09
NS_INSTALL_DIR := $(TIRTOS_INSTALL_DIR)/products/ns_1_11_00_10
UIA_INSTALL_DIR := $(TIRTOS_INSTALL_DIR)/products/uia_2_00_05_50
TIVAWARE_INSTALL_DIR ?= $(TIRTOS_INSTALL_DIR)/products/TivaWare_C_Series-2.1.1.71b

TIRTOS_PACKAGES_DIR = $(TIRTOS_INSTALL_DIR)/packages
TIDRIVERS_PACKAGES_DIR = $(TIDRIVERS_INSTALL_DIR)/packages
BIOS_PACKAGES_DIR = $(BIOS_INSTALL_DIR)/packages
NDK_PACKAGES_DIR = $(NDK_INSTALL_DIR)/packages
NS_PACKAGES_DIR = $(NS_INSTALL_DIR)/packages
UIA_PACKAGES_DIR = $(UIA_INSTALL_DIR)/packages

XDCPATH = $(TIRTOS_PACKAGES_DIR);$(TIDRIVERS_PACKAGES_DIR);$(BIOS_PACKAGES_DIR);$(NDK_PACKAGES_DIR);$(NS_PACKAGES_DIR);$(UIA_PACKAGES_DIR);

CFLAGS = -I$(TIVAWARE_INSTALL_DIR) -I$(BIOS_PACKAGES_DIR)/ti/sysbios/posix -D_POSIX_SOURCE -D PART_TM4C123GH6PM -D gcc -D TIVAWARE -mcpu=cortex-m4 -march=armv7e-m -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -ffunction-sections -fdata-sections -g -gstrict-dwarf -Wall

LFLAGS = -Wl,-T,EK_TM4C123GXL.lds -Wl,-Map,$(NAME).map -Wl,-T,$(NAME)/linker.cmd -L$(TIVAWARE_INSTALL_DIR)/grlib/gcc -L$(TIVAWARE_INSTALL_DIR)/usblib/gcc -L$(TIVAWARE_INSTALL_DIR)/driverlib/gcc -lgr -lusb -ldriver -march=armv7e-m -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -nostartfiles -static -Wl,--gc-sections -L$(BIOS_PACKAGES_DIR)/gnu/targets/arm/libs/install-native/arm-none-eabi/lib/armv7e-m/fpu -lgcc -lc -lm -lrdimon

###################### SHOULD NOT MODIFY BELOW THIS LINE ############################
export XDCPATH

XDCTARGET = gnu.targets.arm.M4F
PLATFORM = ti.platforms.tiva:TM4C123GH6PM

ifeq ("$(SHELL)","sh.exe")
#For Windows
        RMDIR  = -rmdir /S /Q
        remove = -del $(subst /,\,$1)
        pwd    = cd
else
#For Linux
        RMDIR  = rm -rf
        remove = rm -f $(subst /,\,$1)
        pwd    = pwd
endif

###### recursive wildcard function #######
rwildcard=$(wildcard $1$2) $(foreach d, $(wildcard $1*),$(call rwildcard,$d/,$2))

###### returns all directories except one generated by configuro ######
DIRS = $(filter-out $(NAME)/, $(wildcard */))

###### returns all sources in example directory ######
SOURCES    = $(wildcard *.c)
CPPSOURCES = $(wildcard *.cpp)
ASM_FILES  = $(wildcard *.asm)

###### recursively find all sources in every subdirectory ######
SOURCES    += $(foreach d, $(DIRS),$(call rwildcard,$(d),*.c))
CPPSOURCES += $(foreach d, $(DIRS),$(call rwildcard,$(d),*.cpp))
ASM_FILES  += $(foreach d, $(DIRS),$(call rwildcard,$(d),*.asm))

###### get list of source directories to add to compiler include paths ######
INC_DIRS = $(foreach d, $(sort $(dir $(abspath $(SOURCES) $(CPPSOURCES) $(AMS_FILES)))), -I$(d))


###### return list of .obj's from all .c's ######
OBJECTS  = $(patsubst %.c, %.obj, $(SOURCES))
OBJECTS += $(patsubst %.cpp, %.obj, $(CPPSOURCES))
OBJECTS += $(patsubst %.asm, %.obj, $(ASM_FILES))

###### return config file name #######
NAME = $(strip $(patsubst %.cfg, %, $(wildcard *.cfg)))

###### return example name #######
PROG = sd_logger

###### Extend compiler options here ######

CFLAGS += $(INC_DIRS) 

###### Extend linker options here ######
LFLAGS += 

.PRECIOUS: %/compiler.opt %/linker.cmd

all: $(PROG).out

%/compiler.opt: %/linker.cmd;

%.obj : %.c
%.obj : %.cpp
%.obj : %.asm

%/linker.cmd: %.cfg
	@ echo Running Configuro...
	@ $(XDC_INSTALL_DIR)/xs xdc.tools.configuro -c "$(CODEGEN_INSTALL_DIR)" -t $(XDCTARGET) -p $(PLATFORM) --compileOptions "$(CFLAGS)" $(NAME).cfg

%.obj: %.c $(NAME)/compiler.opt
	@ echo Building $@
	@ $(CC)  $(CFLAGS) $< -c @$(NAME)/compiler.opt -o $@

%.obj: %.cpp $(NAME)/compiler.opt
	@ echo Building $@
	@ $(CC)  $(CFLAGS) $< -c @$(NAME)/compiler.opt -o $@

%.obj: %.asm $(NAME)/compiler.opt
	@ echo Building $@
	@ $(CC)  $(CFLAGS) $< -c @$(NAME)/compiler.opt -o $@

$(PROG).out: $(OBJECTS) $(NAME)/linker.cmd
	@ echo linking...
	@ $(LNK)  $(OBJECTS)  $(LFLAGS) -o $(PROG).out

clean:
	@ echo Cleaning...
	@ $(call remove, $(OBJECTS))
	@ $(call remove, $(PROG).out)
	@ $(call remove, $(NAME).map)
	@ $(RMDIR) $(NAME)

debug: $(PROG).out
	# Start gdb
	$(CODEGEN_INSTALL_DIR)/bin/arm-none-eabi-gdb --command gdb.command

debugserver:
	# Run openocd
	/usr/bin/openocd -f /usr/share/openocd/scripts/board/ek-tm4c123gxl.cfg

.PHONY: debug debugserver
	
