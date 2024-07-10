.PHONY: all
.PHONY: clean
.PHONY: dump

# Custom definitions
OUT = ubi-helper
CC := gcc
CFLAGS := -Werror 

SOURCES = ubi-helper.c

# The dependency files
DEPEND_FILES_NO_DIR = $(notdir $(patsubst %.c,%.d,$(SOURCES)))
DEPEND_FILES = $(addprefix build/,$(DEPEND_FILES_NO_DIR))

# The obj files list
OBJ_FILES_NO_DIR = $(notdir $(patsubst %.c,%.o,$(SOURCES)))
OBJ_FILES = $(addprefix build/,$(OBJ_FILES_NO_DIR))

# ubi-helper -d super --lebsize=0x3E000 --inputfile=./test-master.ubifs
all: $(OUT)
	@echo "================ Running app ================"
	@./$(OUT) -d
	@echo

clean:
	rm -f build/*.o build/*.d
	rm -f $(OUT)
	
dump: 
	$(warning CC=$(CC))
	$(warning SOURCES=$(SOURCES))	
	$(warning OBJ_FILES=$(OBJ_FILES))		

include $(DEPEND_FILES)

build/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

build/%.d: %.c
	$(CC) -MM $(CFLAGS) $< > $@.$$$$;                      \
	sed 's,\($*\)\.o[ :]*,build/\1.o $@ : ,g' < $@.$$$$ > $@;     \
	rm -f $@.$$$$

$(OUT): $(OBJ_FILES)
	$(CC) -o $@ $^ 

