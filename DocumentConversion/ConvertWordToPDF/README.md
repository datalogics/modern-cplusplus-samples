# ConvertWordToPDF

Converts a Microsoft Word (`.docx`) document to PDF with the Datalogics
Office-to-PDF plug-in, through `Document::from_office_file` in the Datalogics
C++ APDFL API.

The sample creates a `Library`, sets an `OfficeConvertParams`, and converts:

```cpp
auto [doc, info] = Document::from_office_file(input, params);
// info.get_page_count(), info.get_diagnostics()
```

`from_office_file` returns an `OfficeConvertResult`: the converted `Document`,
which the sample saves, and an `OfficeConvertInfo` with the page count and the
per-asset diagnostics (substituted fonts, placeholder graphics), which can appear
even on a successful conversion. A document that does not convert throws
`OfficeConvertError`, and the sample reports its `Category` and the plug-in's
message.

## Building and running

```sh
make                                   # or open ConvertWordToPDF.vcxproj in Visual Studio
./convert_word_to_pdf                  # converts the bundled sample.docx to ConvertWordToPDF-out.pdf
./convert_word_to_pdf input.docx out.pdf   # convert a specific document
```

Ensure the SDK `lib/` directory is on your library search path so the APDFL
runtime is found (`Directory.Build.props` sets this up for Visual Studio; the
Makefile sets an RPATH). The plug-in, `DL210OfficeToPDF.ppi`, sits in the same
directory, where APDFL loads its plug-ins.

## The sample document

`sample.docx` is a Datalogics stock sample document (`DOCXLink.docx` from the
APDFL sample-input set). The sample accepts any `.docx` as an argument, so you
can point it at your own document. Additional Datalogics sample documents (for
example `attachment2.docx`) can be bundled here the same way if a richer
demonstration is wanted.

## Conversion options

The sample fixes the conversion time, so every run stamps the same dates, and
leaves comments out. A default `OfficeConvertParams` uses the system clock and
also leaves comments out. Call `set_comments` with
`OfficeCommentRendering::Margin` or `OfficeCommentRendering::Annotations` to
carry a reviewed document's comments into the PDF. The sample saves with
`SaveFlags::KeepModDate`, so the fixed time is kept as the modification date as
well.

## Platform support

The Office-to-PDF plug-in is published for a narrower set of platforms than
APDFL itself:

| Platform | Architectures |
|----------|---------------|
| Windows  | x64 |
| Linux    | x86_64, ARM64 |
| macOS    | Apple silicon (ARM64) only |

**Not supported:** Windows on ARM64, and macOS on Intel (x86_64). On those
platforms the conversion throws `OfficeConvertError` with
`Category::PluginUnavailable`, which this sample reports as
`Conversion failed (Plugin unavailable)`. The code still compiles everywhere.

The same error is raised on a supported platform if `DL210OfficeToPDF.ppi` is
missing from the SDK's `lib/` directory.

## Notes

The plug-in runs one conversion at a time, so calls to `from_office_file` from
several threads, each holding its own `Library`, wait for each other.
