#So that other compiling this on their machine can set things up and not
#have to modify this Makefile.
include makefile.local

SOURCES_PROJECT = main_pc_comm.c pc_serial.c read_thread.c keyboard.c

SOURCES = $(SOURCES_PROJECT)
OBJECTS = $(SOURCES:.c=.o)

#Where to put objects
OBJ_DIR = obj
OBJ_OBJECTS := $(addprefix $(OBJ_DIR)/, $(OBJECTS))

#Where to find sources
LOCAL_SRC_DIR = src
VPATH = $(LOCAL_SRC_DIR)


CFLAGS  =  -I. -IInclude -Iinclude -Iinc \
	-c -fno-common -O2 -g  \
	$(LOCAL_CFLAGS)
LFLAGS  =
LFLAGS_END = -lpthread
CPFLAGS = -Obinary
ODFLAGS = -S

all: $(EXE)
	@ echo ""
	@ echo ""

debug:
	@ echo "Sources:"  $(SOURCES)
	@ echo "Objects:"  $(OBJECTS)

clean:
	-rm -f $(EXE) $(OBJ_OBJECTS)

main.bin: main.elf
	@ echo "/* ***************************************************** */"
	@ echo "/* ...copying                                             */"
	@ echo "/* ***************************************************** */"
	$(CP) $(CPFLAGS) main.elf main.bin
	$(OD) $(ODFLAGS) main.elf > main.lst

$(EXE): $(OBJ_OBJECTS)
	@ echo "/* ***************************************************** */"
	@ echo "/* ...linking " $(EXE) " */"
	@ echo "/* ***************************************************** */"
	$(LD) $(LFLAGS) -o $(EXE) $(OBJ_OBJECTS) $(LFLAGS_END)
	@ echo ""
	@ echo ""

$(OBJ_DIR)/%.o: %.c
	@ echo "/* ***************************************************** */"
	@ echo "/* ...compiling " $(notdir $<) " */"
	@ echo "/* ***************************************************** */"
	$(CC) $(CFLAGS) $< -o $@
	@ echo ""

# $(OBJ_DIR)/%.o: %.s
# 	@ echo "/* ***************************************************** */"
# 	@ echo "/* ...compiling assembly " $(notdir $<) "*/"
# 	@ echo "/* ***************************************************** */"
# 	$(AS) $< -o $@

run: $(EXE)
	$(EXE) "/dev/ttyUSB0"

gdb: $(EXE)
	gdb $(EXE)

valgrind: $(EXE)
	valgrind $(EXE) "/dev/ttyUSB0"

