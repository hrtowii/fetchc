# Compiler
CC = clang

# Compiler flags
CFLAGS = -O3

# Frameworks
FRAMEWORKS = -framework IOKit -framework CoreFoundation

# Source file
SRC = main.c

# Output executable
OUT = main

# Default target
all: $(OUT)

# Link and create the executable
$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(FRAMEWORKS) -o $(OUT) $(SRC)

# Clean target
clean:
	rm -f $(OUT)
