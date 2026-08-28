TARGET = human.sys
XDF = ../dist/wdrfrace.xdf

CROSS = m68k-xelf-
CC = $(CROSS)g++
AS = $(CROSS)as
LD = $(CROSS)g++

CFLAGS = -m68000 -O2 -g -MMD -MP -specs=x68knodos.specs
LDFLAGS = -Ttext=0x6800 -s -specs=x68knodos.specs

BGM_QUALITY ?= NDP_QUALITY_FAST
BGM_PROFILE ?= NDP_PROFILE_STREAM
PRIVATE_BGM ?= 0
CFLAGS += -DNDP_QUALITY=$(BGM_QUALITY)
CFLAGS += -DNDP_PROFILE=$(BGM_PROFILE)
ifeq ($(PRIVATE_BGM),1)
ifeq ($(wildcard bgmpriv.h),)
$(error PRIVATE_BGM=1 requires src/bgmpriv.h)
endif
CFLAGS += -DWDR_PRIVATE_BGM
endif

SOURCES := $(wildcard *.cpp)
OBJECTS := $(SOURCES:.cpp=.o)
DEPS := $(OBJECTS:.o=.d)
INTRO_DATA := introdat.h
COURSE_DATA := coursdat.h
DEMO_DATA := demodat.h
HERO_DATA := herodat.h
FONT_DATA := telefont.h
SE_MML := wdr_se.mml
SE_DATA := wdrse.h
SE_BINARY = $(BUILD_DIR)/wdrse.ndss
SE_TOOL := ndpstreamse.py

PYTHON ?= python3
CURL ?= curl

BUILD_DIR := build
XDF_ROOT := $(BUILD_DIR)/xdfroot
XDFTOOL := $(BUILD_DIR)/xdftool.py
XDFTOOL_URL := https://cdn.jsdelivr.net/gh/yunkya2/x68kmisc@08f647bad280dc8eac778f3f98358350d10e9471/xdftool/xdftool.py

.PHONY: all clean distclean check-tools check-xdf

all: check-tools $(TARGET) $(XDF)

check-tools:
	@command -v $(CC) >/dev/null 2>&1 || { \
		echo "Error: $(CC) was not found. Install elf2x68k and put m68k-xelf-* in PATH." >&2; \
		exit 1; \
	}
	@command -v $(PYTHON) >/dev/null 2>&1 || { \
		echo "Error: $(PYTHON) is not installed." >&2; \
		exit 1; \
	}
	@command -v $(CURL) >/dev/null 2>&1 || { \
		echo "Error: $(CURL) is not installed." >&2; \
		exit 1; \
	}

$(TARGET): $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^
	
%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

gmrace.o: $(INTRO_DATA)
gmcourse.o: $(COURSE_DATA)
gmdemo.o: $(DEMO_DATA)
gmtitle.o: $(HERO_DATA)
screen.o: $(FONT_DATA)
bgm.o: bgmdata.h ndp.h $(wildcard bgmpriv.h) $(wildcard bgmstrm*.h)
sound.o: $(SE_DATA) ndp.h

$(SE_DATA): $(SE_MML) $(SE_TOOL) | $(BUILD_DIR)
	$(PYTHON) $(SE_TOOL) $(SE_MML) --binary $(SE_BINARY) \
		--header $@ --symbol wdr_stream_se_data

$(FONT_DATA): mktele.py
	$(PYTHON) $< > $@.tmp
	mv $@.tmp $@

$(INTRO_DATA): mkintro.py
	$(PYTHON) $< > $@.tmp
	mv $@.tmp $@

$(COURSE_DATA): mkcourse.py
	$(PYTHON) $< > $@.tmp
	mv $@.tmp $@

$(DEMO_DATA): mktitle.py
	$(PYTHON) $< > $@.tmp
	mv $@.tmp $@

$(HERO_DATA): mkhero.py
	$(PYTHON) $<

$(BUILD_DIR):
	mkdir -p $@

$(XDFTOOL): | $(BUILD_DIR)
	$(CURL) -L --fail --show-error -o $@ $(XDFTOOL_URL)
	chmod +x $@

$(XDF): Makefile $(TARGET) $(XDFTOOL)
	mkdir -p $(dir $(XDF))
	rm -rf $(XDF_ROOT)
	mkdir -p $(XDF_ROOT)
	cp $(TARGET) $(XDF_ROOT)/human.sys
	cd $(XDF_ROOT) && \
		$(PYTHON) ../xdftool.py c ../../$(XDF) human.sys

check-xdf: $(XDF) $(XDFTOOL)
	$(PYTHON) $(XDFTOOL) t $(XDF)

clean:
	-rm -f *.o *.d *.elf* $(TARGET) $(XDF)
	-rm -rf $(XDF_ROOT)

distclean: clean
	-rm -rf $(BUILD_DIR)
	-rm -f $(INTRO_DATA) $(INTRO_DATA).tmp $(COURSE_DATA) $(COURSE_DATA).tmp
	-rm -f $(DEMO_DATA) $(DEMO_DATA).tmp
	-rm -f $(HERO_DATA) $(HERO_DATA).tmp titldat.h titldat.h.tmp
	-rm -f $(SE_DATA)

-include $(DEPS)


