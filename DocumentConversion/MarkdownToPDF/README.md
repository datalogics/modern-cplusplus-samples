# MarkdownToPDF

Converts a practical Markdown subset directly to a newly created, tagged PDF
document with the Datalogics C++ APDFL API. The sample parses Markdown itself,
lays text out with real font metrics (`Font::measure_text_width`), and builds
the logical structure tree (`StructTreeRoot` / `StructElement` /
`Container::add_marked_content_ref`) while it draws, so the output is a
tagged, accessible PDF — no HTML, no browser, no third-party Markdown or PDF
library.

## Supported Markdown

- ATX (`#` … `######`) and setext headings
- Paragraphs with `**bold**`, `*italic*`, `` `inline code` ``,
  `~~strikethrough~~`, and CommonMark-style emphasis rules (no intraword `_`)
- Links: inline `[label](url)`, reference `[label][id]`, autolinks
  `<https://…>`, bare URLs, and `mailto:` autolinks — each drawn in blue with
  a working link annotation (`LinkAnnotation` + `URIAction`)
- Ordered, unordered, and task lists (`- [x]`) with indentation levels,
  tagged `L` → `LI` → `Lbl`/`LBody`
- Fenced code blocks (``` or `~~~`) with a language title row, gray
  background, preserved indentation, and hard wrapping
- Blockquotes, horizontal rules, and pipe tables with per-column alignment,
  tagged `Table` → `TR` → `TH`/`TD`
- An "HTML-lite" subset (`<h1>`–`<h6>`, `<a>`, `<strong>`, `<em>`, `<code>`,
  `<del>`, `<br>`, block containers, HTML entities) normalized to Markdown
  before parsing; remaining tags are stripped unless
  `--include-unrendered-html` is given
- Image syntax is intentionally rendered as an `[Image omitted: …]` note;
  the sample stays local-file only

CJK text wraps per character and falls back to an installed font that covers
the script (`Font::is_text_representable`); Cyrillic and Greek get the same
treatment.

## Building and running

```sh
make          # or open MarkdownToPDF.vcxproj in Visual Studio
./markdown_to_pdf                         # converts sample.md to MarkdownToPDF-out.pdf
./markdown_to_pdf input.md output.pdf     # convert a specific file
./markdown_to_pdf --help                  # all options
./markdown_to_pdf --self-test             # parser self-tests, no PDF written
./markdown_to_pdf --list-font-families    # font families APDFL can use here
```

Useful options: `--page-size` (Letter, Legal, Ledger, A3, A4, A5, Tabloid, or
`WIDTHxHEIGHT` in points), `--orientation`, `--margin`, `--font-family`,
`--heading-font-family`, `--code-font-family`, `--cjk-font-family`,
`--fallback-fonts`, `--title`, `--lang`, `--overwrite`, `--verbose`.

The `samples/` folder contains documents that exercise pagination,
multilingual text, HTML-lite input, and the image-omission behavior; see
`samples/README.md`.

Set `APDFL_LICENSE_KEY` to provide a Datalogics APDFL activation key before
the library initializes.

## Differences from the .NET sample

This is a rewrite of the `apdfl-csharp-dotnet-samples` sample of the same
name using the Modern C++ interface. The Modern C++ API assigns marked-content
IDs and maintains the structure parent tree automatically
(`StructElement::add_marked_content_ref`), so the tagging code is
substantially simpler than the .NET version. Batch directory conversion
(`--recursive`) is not carried over; convert files one at a time.
