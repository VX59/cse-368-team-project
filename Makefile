CC = g++
CFLAGS = -fPIC -shared -Wall -g

BUILD_DIR = Scripts
SOURCE_DIR = Src
DETOUR_TARGET = $(BUILD_DIR)/ac_detour.so
DETOUR_SOURCE = $(SOURCE_DIR)/ac_detour.cpp $(SOURCE_DIR)/checkinput_hook.cpp

all: $(DETOUR_TARGET)

# For compiling the main detour shared-library
$(DETOUR_TARGET): $(DETOUR_SOURCE)
	$(CC) $(CFLAGS) $(DETOUR_SOURCE) -o $(DETOUR_TARGET)

clean:
	rm -f $(DETOUR_TARGET)
