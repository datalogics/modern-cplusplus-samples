# Datalogics C++ APDFL Samples

Sample applications demonstrating the Datalogics C++ APDFL API.

## Prerequisites

- C++17 compatible compiler:
  - **Windows**: Visual Studio 2019+ (MSVC v143)
  - **Linux**: GCC 7+ or Clang 5+
  - **macOS**: Xcode 11+ (Apple Clang)
- DatalogicsCppAPI SDK (this repo should be inside `DatalogicsCppAPI-0.1.1/`)

## Directory Structure

```
DatalogicsCppAPI-0.1.1/
├── include/                    ← SDK headers
├── lib/                        ← SDK libraries and resources
└── datalogics-cplusplus-samples/  ← this repo
    ├── Annotations/
    ├── ContentCreation/
    ├── ContentModification/
    ├── DocumentConversion/
    ├── DocumentOptimization/
    ├── Forms/
    ├── Images/
    ├── InformationExtraction/
    ├── OpticalCharacterRecognition/
    ├── Other/
    ├── Security/
    └── Text/
```

## Building a Sample

### Windows (Visual Studio)

Open the `.vcxproj` file for any sample in Visual Studio and build.

Ensure the SDK `lib/` directory is in your system PATH so DLLs are found at runtime.

### Linux / macOS (Make)

```bash
cd Text/ListWords
make
make run
```

The Makefile sets RPATH so the executable finds SDK libraries automatically.

## Samples

| Category | Samples |
|----------|---------|
| Annotations | 5 |
| ContentCreation | 16 |
| ContentModification | 14 |
| DocumentConversion | 10 |
| DocumentOptimization | 1 |
| Forms | 4 |
| Images | 14 |
| InformationExtraction | 6 |
| OpticalCharacterRecognition | 3 |
| Other | 2 |
| Security | 4 |
| Text | 14 |
| **Total** | **93** |
