CC := g++
RM := rm -f
CFLAGS := $(shell pkg-config --cflags sdl2 zlib)
LIBS := $(shell pkg-config --libs sdl2 zlib)

TARGET = ch8emu

all: $(TARGET)

%.o: %.cpp
	$(CC) -c -o $@ $^ $(CFLAGS)

$(TARGET): chip8.o main.o 
	$(CC) -o $(TARGET) chip8.o main.o $(LIBS)

.PHONY: clean

clean:
	$(RM) $(TARGET) *.o