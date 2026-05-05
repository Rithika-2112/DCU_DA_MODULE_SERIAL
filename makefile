# project name (generate executable with this name)
TARGET   = gurux.dlms.client.bin

CC       = /home/imx6ul/Toolchain/gcc-linaro-4.9-2014.11-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc

# compiling flags here
CFLAGS   = -std=gnu99 -Wall -I.

LINKER   = /home/imx6ul/Toolchain/gcc-linaro-4.9-2014.11-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc -o

# linking flags here
LFLAGS   = -L/home/rithika/Projects/DCU/GuruxDLMS.c-master/development/lib  -L/home/imx6ul/CC/redis-bin/lib

# change these to set the proper directories where each files should be

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin
INCDIR   = -Iinclude  -I/home/rithika/json/nxjson/inc/ -I/home/imx6ul/CC/redis-bin/include -I/../GuruxDLMS.c-master/development/include -I/../GuruxDLMS.c-master/development/GuruxDLMSClientExample/include -I/home/vishnu/Projects/REConnect/hc_file/ -I/home/vishnu/Projects/REConnect/net_logger/ 

# SOURCES  := $(wildcard $(SRCDIR)/*.c)
SOURCES := $(wildcard $(SRCDIR)/*.c)
# SOURCES := $(filter-out $(SRCDIR)/hc_heartbeat.c $(SRCDIR)/dcu_netlog.c, $(wildcard $(SRCDIR)/*.c))
EXTERNAL_SRC := /home/vishnu/Projects/REConnect/hc_file/hc_heartbeat.c /home/vishnu/Projects/REConnect/net_logger/dcu_netlog.c

INCLUDES := $(wildcard $(INCDIR)/*.h)
# OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
# EXTERNAL_OBJ := $(OBJDIR)/hc_heartbeat.o 

EXTERNAL_OBJ := \
    $(OBJDIR)/hc_heartbeat.o \
    $(OBJDIR)/dcu_netlog.o
rm       = rm -f


# $(BINDIR)/$(TARGET): $(OBJECTS)
$(BINDIR)/$(TARGET): $(OBJECTS) $(EXTERNAL_OBJ)
	@$(LINKER) $@ $(LFLAGS) $(OBJECTS) $(EXTERNAL_OBJ) -lgurux_dlms_c -lm -lhiredis -ldl -lpthread 
	@echo "Linking complete!"
	cp bin/gurux.dlms.client.bin .


$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

# $(EXTERNAL_OBJ): $(EXTERNAL_SRC)
# 	@$(CC) $(CFLAGS) -c $< -o $@
# 	@echo "Compiled external $< successfully!"

# Compile external files
$(OBJDIR)/hc_heartbeat.o : /home/vishnu/Projects/REConnect/hc_file/hc_heartbeat.c
	@$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@
	@echo "Compiled hc_heartbeat.c"

$(OBJDIR)/dcu_netlog.o : /home/vishnu/Projects/REConnect/net_logger/dcu_netlog.c
	@$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@
	@echo "Compiled dcu_netlog.c"

# .PHONEY: clean
# clean:
# 	@$(rm) $(OBJECTS)
# 	@echo "Cleanup complete!" 
# 	@echo $(OBJECTS)

.PHONEY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
	@echo "Executable removed!"

.PHONY: clean

clean:
	rm -rf $(OBJDIR)/*.o $(BINDIR)/$(TARGET)
	@echo "Executable removed!"

