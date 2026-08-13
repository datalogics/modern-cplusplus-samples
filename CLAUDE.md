# CLAUDE.md

This file provides guidance to Claude Code when working with code in this repository.

## Project Overview

C++ sample applications demonstrating the Datalogics C++ APDFL API. Each sample is a standalone `main()` program in its own directory, organized by category (Text, Images, ContentCreation, etc.).

## Building

Each sample has its own Makefile. Build from the sample directory:

```bash
cd Text/ListWords
make        # build
make run    # build and run
make clean  # remove binary
```

The SDK root is expected at `../../..` (i.e., the repo lives inside `DatalogicsCppAPI-0.1.1/datalogics-cplusplus-samples/`). Override with `SDK_ROOT=path make`.

## Source Conventions

- All samples use `#include <datalogics_interface/datalogics_interface.hpp>` (umbrella header)
- All samples use `using namespace datalogics_interface;` after includes
- Error handling: wrap main logic in `try/catch (const std::exception& e)`
- Input files come from `Library::get_resource_directory() + "Sample_Input/..."` with optional `argv` overrides
- Output files are written to the current directory

## Commit Message Format

Use imperative mood with a category prefix when touching a specific area:

```
fix: correct API usage in AddGlyphs sample
feat: add Text category samples (14 samples)
```

## Directory Structure

```
_Common/             — shared headers (extract_text.hpp)
Annotations/         — 5 samples
ContentCreation/     — 16 samples
ContentModification/ — 14 samples
DocumentConversion/  — 10 samples
DocumentOptimization/— 1 sample
Forms/               — 4 samples
Images/              — 14 samples
InformationExtraction/ — 6 samples
OpticalCharacterRecognition/ — 3 samples
Other/               — 2 samples
Security/            — 4 samples
Text/                — 14 samples
```

Each sample directory contains a `.cpp` source file and a `Makefile`.
