ARCHTUPLE=arm-none-eabi-
DEVICE=STM32F0-Discovery

MFLAGS=-mcpu=cortex-m0
CPPFLAGS=-Og -DUSE_STDPERIPH_DRIVER
GCCFLAGS=-ffunction-sections -fdata-sections -fdiagnostics-color -g

WARNFLAGS+=

SPACE :=
SPACE +=
COMMA := ,

LIBRARIES+=$(wildcard $(FWDIR)/*.a)
wlprefix=-Wl,$(subst $(SPACE),$(COMMA),$1)
LNK_FLAGS=--gc-sections --start-group $(strip $(LIBRARIES) )-lc -lm -lgcc -lstdc++ -lsupc++ --end-group

ASMFLAGS=$(MFLAGS) $(CPPFLAGS) $(WARNFLAGS) $(GCCFLAGS) $(WARNFLAGS)
CFLAGS=$(MFLAGS) $(CPPFLAGS) $(WARNFLAGS) $(GCCFLAGS) --std=gnu11
CXXFLAGS=$(MFLAGS) $(CPPFLAGS) $(WARNFLAGS) $(GCCFLAGS) --std=gnu++17
LDFLAGS=$(MFLAGS) $(WARNFLAGS) -specs=nano.specs
SIZEFLAGS=-d --common
NUMFMTFLAGS=--to=iec --format %.2f --suffix=B

AR:=$(ARCHTUPLE)ar
# using arm-none-eabi-as generates a listing by default. This produces a super verbose output.
# Using gcc accomplishes the same thing without the extra output
AS:=$(ARCHTUPLE)gcc
CC:=$(ARCHTUPLE)gcc
CXX:=$(ARCHTUPLE)g++
LD:=$(ARCHTUPLE)g++
OBJCOPY:=$(ARCHTUPLE)objcopy
SIZETOOL:=$(ARCHTUPLE)size
READELF:=$(ARCHTUPLE)readelf
STRIP:=$(ARCHTUPLE)strip

ifneq (, $(shell command -v gnumfmt 2> /dev/null))
	SIZES_NUMFMT:=| gnumfmt --field=-4 --header $(NUMFMTFLAGS)
else
ifneq (, $(shell command -v numfmt 2> /dev/null))
	SIZES_NUMFMT:=| numfmt --field=-4 --header $(NUMFMTFLAGS)
else
	SIZES_NUMFMT:=
endif
endif

ifneq (, $(shell command -v sed 2> /dev/null))
SIZES_SED:=| sed -e 's/  dec/total/'
else
SIZES_SED:=
endif

rwildcard=$(foreach d,$(filter-out $3,$(wildcard $1*)),$(call rwildcard,$d/,$2,$3)$(filter $(subst *,%,$2),$d))

# Colors
NO_COLOR=$(shell printf "%b" "\033[0m")
OK_COLOR=$(shell printf "%b" "\033[32;01m")
ERROR_COLOR=$(shell printf "%b" "\033[31;01m")
WARN_COLOR=$(shell printf "%b" "\033[33;01m")
STEP_COLOR=$(shell printf "%b" "\033[37;01m")
OK_STRING=$(OK_COLOR)[OK]$(NO_COLOR)
DONE_STRING=$(OK_COLOR)[DONE]$(NO_COLOR)
ERROR_STRING=$(ERROR_COLOR)[ERRORS]$(NO_COLOR)
WARN_STRING=$(WARN_COLOR)[WARNINGS]$(NO_COLOR)
ECHO=/bin/printf "%s\n"
echo=@$(ECHO) "$2$1$(NO_COLOR)"
echon=@/bin/printf "%s" "$2$1$(NO_COLOR)"

define test_output
@if test $(BUILD_VERBOSE) -eq $(or $4,1); then printf "%s\n" "$2"; fi;
@output="$$($2 2>&1)"; exit=$$?;           \
if test 0 -ne $$exit; then                 \
  printf "%s%s\n" "$1" "$(ERROR_STRING)";  \
  printf "%s\n" "$$output";                \
  exit $$exit;                             \
elif test -n "$$output"; then              \
  printf "%s%s\n" "$1" "$(WARN_STRING)";   \
  printf "%s\n" "$$output";                \
else                                       \
  printf "%s%s\n" "$1" "$3";               \
fi;
endef

# Makefile Verbosity
ifeq ("$(origin VERBOSE)", "command line")
BUILD_VERBOSE = $(VERBOSE)
endif
ifeq ("$(origin V)", "command line")
BUILD_VERBOSE = $(V)
endif

ifndef BUILD_VERBOSE
BUILD_VERBOSE = 0
endif

# R is reduced (default messages) - build verbose = 0
# V is verbose messages - verbosity = 1
# VV is super verbose - verbosity = 2
ifeq ($(BUILD_VERBOSE), 0)
R = @echo
D = @
VV = @
endif
ifeq ($(BUILD_VERBOSE), 1)
R = @echo
D =
VV = @
endif
ifeq ($(BUILD_VERBOSE), 2)
R =
D =
VV =
endif

INCLUDE=$(foreach dir,$(INCDIR) $(EXTRA_INCDIR),-iquote"$(dir)")

ASMSRC=$(foreach asmext,$(ASMEXTS),$(call rwildcard, $(SRCDIR),*.$(asmext), $1))
ASMOBJ=$(addprefix $(BINDIR)/,$(patsubst $(SRCDIR)/%,%.o,$(call ASMSRC,$1)))
CSRC=$(foreach cext,$(CEXTS),$(call rwildcard, $(SRCDIR),*.$(cext), $1))
COBJ=$(addprefix $(BINDIR)/,$(patsubst $(SRCDIR)/%,%.o,$(call CSRC, $1)))
CXXSRC=$(foreach cxxext,$(CXXEXTS),$(call rwildcard, $(SRCDIR),*.$(cxxext), $1))
CXXOBJ=$(addprefix $(BINDIR)/,$(patsubst $(SRCDIR)/%,%.o,$(call CXXSRC,$1)))

# support for copying objects in src directory to bin directory
OBJSRC=$(foreach objext,$(OBJEXTS),$(call rwildcard, $(SRCDIR),*.$(objext), $1))
OBJOBJ=$(addprefix $(BINDIR)/,$(patsubst $(SRCDIR)/%,%,$(call OBJSRC,$1)))

GETALLOBJ=$(sort $(call ASMOBJ,$1) $(call COBJ,$1) $(call CXXOBJ,$1) $(call OBJOBJ,$1))

ARCHIVE_TEXT_LIST=$(subst $(SPACE),$(COMMA),$(notdir $(basename $(LIBRARIES))))

MONOLITH_BIN:=$(BINDIR)/monolith.bin
MONOLITH_HEX:=$(basename $(MONOLITH_BIN)).hex
MONOLITH_ELF:=$(basename $(MONOLITH_BIN)).elf

.PHONY: all clean quick

quick: $(MONOLITH_BIN) $(MONOLITH_HEX)

all: clean $(MONOLITH_BIN) $(MONOLITH_HEX)

clean:
	@echo Cleaning project
	-$Drm -rf $(BINDIR)

ELF_DEPS=$(call GETALLOBJ,$(EXCLUDE_SRCDIRS))

$(MONOLITH_BIN): $(MONOLITH_ELF)
	$(VV)mkdir -p $(dir $@)
	$(call test_output,Creating $@ for $(DEVICE) ,$(OBJCOPY) $< -O binary -S $@,$(DONE_STRING))

$(MONOLITH_HEX): $(MONOLITH_ELF)
	$(VV)mkdir -p $(dir $@)
	$(call test_output,Creating $@ for $(DEVICE) ,$(OBJCOPY) $< -O ihex $@,$(DONE_STRING))

$(MONOLITH_ELF): $(ELF_DEPS) $(LIBRARIES)
	$(VV)mkdir -p $(dir $@)
	$(call test_output,Linking project with $(ARCHIVE_TEXT_LIST) ,$(LD) $(LDFLAGS) $(ELF_DEPS) $(LDTIMEOBJ) $(call wlprefix,-T$(ROOT)/LinkerScript.ld $(LNK_FLAGS)) -o $@,$(OK_STRING))
	@echo Section sizes:
	-$(VV)$(SIZETOOL) $(SIZEFLAGS) $@ $(SIZES_SED) $(SIZES_NUMFMT)

define asm_rule
$(BINDIR)/%.$1.o: $(SRCDIR)/%.$1
	$(VV)mkdir -p $$(dir $$@)
	$$(call test_output,Compiling $$< ,$(AS) -c $(ASMFLAGS) -o $$@ $$<,$(OK_STRING))
endef
$(foreach asmext,$(ASMEXTS),$(eval $(call asm_rule,$(asmext))))

define c_rule
$(BINDIR)/%.$1.o: $(SRCDIR)/%.$1
	$(VV)mkdir -p $$(dir $$@)
	$$(call test_output,Compiling $$< ,$(CC) -c $(INCLUDE) -iquote"$(INCDIR)/$$(dir $$*)" $(CFLAGS) $(EXTRA_CFLAGS) -o $$@ $$<,$(OK_STRING))
endef
$(foreach cext,$(CEXTS),$(eval $(call c_rule,$(cext))))

define cxx_rule
$(BINDIR)/%.$1.o: $(SRCDIR)/%.$1
	$(VV)mkdir -p $$(dir $$@)
	$$(call test_output,Compiling $$< ,$(CXX) -c $(INCLUDE) -iquote"$(INCDIR)/$$(dir $$*)" $(CXXFLAGS) $(EXTRA_CXXFLAGS) -o $$@ $$<,$(OK_STRING))
endef
$(foreach cxxext,$(CXXEXTS),$(eval $(call cxx_rule,$(cxxext))))

define obj_rule
$(BINDIR)/%.$1: $(SRCDIR)/%.$1
	$(VV)mkdir -p $$(dir $$@)
	$$(call tesT_output,Copying $$< ,cp $< $@,$(OK_STRING))
endef
$(foreach objext,$(OBJEXTS),$(eval $$(call obj_rul,$(cxxext))))

upload: $(MONOLITH_ELF) $(MONOLITH_BIN) $(MONOLITH_HEX)
	openocd -f board/stm32f0discovery.cfg -c "program $(MONOLITH_ELF) verify reset exit"

# these rules are for build-compile-commands, which just print out sysroot information
cc-sysroot:
	@echo | $(CC) -c -x c $(CFLAGS) $(EXTRA_CFLAGS) --verbose -o /dev/null -
cxx-sysroot:
	@echo | $(CXX) -c -x c++ $(CXXFLAGS) $(EXTRA_CXXFLAGS) --verbose -o /dev/null -
