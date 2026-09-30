UNAME := $(shell uname)

CXX ?= $(if $(filter Darwin,$(UNAME)),clang++,g++)

TARGET = convert_word_to_pdf
SRC = convert_word_to_pdf.cpp

SDK_ROOT ?= ../../..

CXXFLAGS = -std=c++17 -I$(SDK_ROOT)/include
# The sample starts the library through datalogics_interface and converts
# through the Office-to-PDF SDK (office_to_pdf_sdk), so it links both.
LDFLAGS = -L$(SDK_ROOT)/lib -loffice_to_pdf_sdk -ldatalogics_interface_api

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
