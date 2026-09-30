# ConvertWordToPDF

Converts a Microsoft Word (`.docx`) document to PDF with the Datalogics
Office-to-PDF SDK's modern C++ interface (`office_to_pdf/converter.hpp`).

The SDK converts on the Adobe PDF Library its caller has started on the
calling thread, and never starts or stops it itself. So the sample creates a
`Library`, as every sample here does, calls `office_to_pdf::ConvertWordToPdf`
while it is alive, then reports the returned status and any per-asset
diagnostics (substituted fonts, placeholder graphics — which can appear even
on a successful conversion).

## Building and running

```sh
make                                   # or open ConvertWordToPDF.vcxproj in Visual Studio
./convert_word_to_pdf                  # converts the bundled sample.docx to ConvertWordToPDF-out.pdf
./convert_word_to_pdf input.docx out.pdf   # convert a specific document
```

Ensure the SDK `lib/` directory is on your library search path so the SDK and
APDFL runtime are found (`Directory.Build.props` sets this up for Visual Studio;
the Makefile sets an RPATH).

## The sample document

`sample.docx` is a Datalogics stock sample document (`DOCXLink.docx` from the
APDFL sample-input set). The sample accepts any `.docx` as an argument, so you
can point it at your own document. Additional Datalogics sample documents (for
example `attachment2.docx`) can be bundled here the same way if a richer
demonstration is wanted.

## Conversion options

The sample pins a fixed creation/modification date so repeated runs are
byte-reproducible and omits Word comments. Call
`ConvertWordToPdf(input, output)` for the SDK defaults (system clock, comments
omitted), or set `ConversionOptions::comments` to `Margin` or `Annotations` to
carry a reviewed document's comments into the PDF.
