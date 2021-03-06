#So that other compiling this on their machine can set things up and not
#have to modify this Makefile.
include makefile.local

SOURCES_PROJECT = main_pc_comm.c pc_serial.c read_thread.c packet_handling_thread.c keyboard.c generic_packet.c gp_receive.c gp_proj_thermal.c gp_proj_universal.c gp_proj_analog.c gp_proj_sonar.c gp_proj_motor.c gp_proj_rs485_sb.c create_image.c palettes.c create_image_rgb.c status_updates.c cmd_handling_readline.c

SOURCES = $(SOURCES_PROJECT)
OBJECTS = $(SOURCES:.c=.o)

#Where to put objects
OBJ_DIR = obj
OBJ_OBJECTS := $(addprefix $(OBJ_DIR)/, $(OBJECTS))

#Where to find sources
LOCAL_SRC_DIR = src
VPATH = $(LOCAL_SRC_DIR) $(GENERIC_PACKET_SRC_DIR)


CFLAGS  =  -I. -IInclude -Iinclude -Iinc \
	-I$(GENERIC_PACKET_INC_DIR) \
	-c -fno-common -O2 -g  \
	$(LOCAL_CFLAGS)
LFLAGS  =
LFLAGS_END = -lm -lpthread -lnetpbm -lreadline
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

