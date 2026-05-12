UNAME := $(shell uname)

CXX ?= $(if $(filter Darwin,$(UNAME)),clang++,g++)

TARGET = import_forms_data
SRC = import_forms_data.cpp

SDK_ROOT ?= ../../..

CXXFLAGS = -std=c++17 -I$(SDK_ROOT)/include
LDFLAGS = -L$(SDK_ROOT)/lib -ldatalogics_interface_api

SDK_LIB := $(abspath $(SDK_ROOT)/lib)

ifeq ($(UNAME),Darwin)
  LDFLAGS += -Wl,-rpath,$(SDK_LIB)
else
  LDFLAGS += -Wl,-rpath,$(SDK_LIB)
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
