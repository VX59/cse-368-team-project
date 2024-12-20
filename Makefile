CC = g++
CFLAGS = -fpic -shared -Wall -g

BUILD_DIR = Bin
SOURCE_DIR = Src
DETOUR_TARGET = $(BUILD_DIR)/ac_detour.so
DETOUR_SOURCE = $(SOURCE_DIR)/hunter_agent.cpp $(SOURCE_DIR)/ac_detour.cpp $(SOURCE_DIR)/Environment_Interaction.cpp $(SOURCE_DIR)/checkinput_hook.cpp $(SOURCE_DIR)/Feature_Resolver.cpp $(SOURCE_DIR)/agents/math_helpers.cpp $(SOURCE_DIR)/agents/conditional/conditional_agent.cpp

all: $(DETOUR_TARGET)

# For compiling the main detour shared-library
$(DETOUR_TARGET): $(DETOUR_SOURCE)
	$(CC) $(CFLAGS) $(DETOUR_SOURCE) -o $(DETOUR_TARGET)

clean:
	rm -f $(DETOUR_TARGET)
