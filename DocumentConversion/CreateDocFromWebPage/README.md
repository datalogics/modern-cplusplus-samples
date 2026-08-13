# CreateDocFromWebPage

Converts a live web page or a local HTML file into a PDF document with the
Datalogics C++ APDFL API. The single positional argument may be a URL
(`http://`, `https://`, or `file://`) or a path to a local HTML file; the
sample detects which from the scheme prefix and routes to
`Document::from_web_url` or `Document::from_html_file` accordingly. Relative
asset references in a local file are resolved against that file's directory.

Rendering is performed by the WebToPDF plugin, which embeds Chromium (CEF) —
see [Platform support](#platform-support) below, because the plugin is not
available everywhere APDFL is.

Both entry points return a `WebConvertResult` designed for structured-binding
destructuring, pairing the new document with metadata about the conversion:

```cpp
auto [doc, info] = Document::from_web_url(source, params);
// info.get_page_count(), get_conversion_time_ms(), get_title(), get_source_url()
```

`get_source_url()` reports the URL that was actually rendered, so it reflects
any redirects the browser followed.

## What the sample demonstrates

- Configuring the conversion through `WebConvertParams`: viewport preset, page
  size and orientation, margins, CSS background printing, and a conversion
  timeout
- A progress callback that reports completion as a fraction; returning `false`
  from it cancels the conversion, which surfaces as
  `OperationCancelledException`
- A log callback that receives plugin diagnostics tagged with a `WebLogLevel`
- Catching `WebConvertError` and reporting its `Category` — the sample maps
  each category to a readable message, so a failure says *why* it failed
  rather than only that it did

The timeout follows the plugin's own convention: `0` means use the plugin
default of 300 seconds, `-1` means no limit, and a positive value is an
explicit number of seconds.

## Building and running

```sh
make          # or open CreateDocFromWebPage.vcxproj in Visual Studio
./create_doc_from_webpage                                   # https://www.datalogics.com -> CreateDocFromWebPage-out.pdf
./create_doc_from_webpage https://example.com out.pdf        # a specific URL
./create_doc_from_webpage page.html out.pdf                 # a local HTML file
./create_doc_from_webpage file:///abs/path/page.html out.pdf # an explicit file URL
```

Both arguments are optional: the first defaults to `https://www.datalogics.com`
and the second to `CreateDocFromWebPage-out.pdf`.

## Platform support

The WebToPDF plugin that performs the rendering is published for a narrower set
of platforms than APDFL itself:

| Platform | Architectures |
|----------|---------------|
| Windows  | x64, ARM64 |
| Linux    | x86_64, ARM64 |
| macOS    | Apple silicon (ARM64) only |

**Not supported:** macOS on Intel (x86_64), and 32-bit Windows (x86). On those
platforms the conversion throws `WebConvertError` with
`Category::PluginUnavailable`, which this sample reports as
`Conversion failed (Plugin unavailable)`. The code still compiles everywhere —
the limitation appears only when a conversion is attempted.

The same `PluginUnavailable` error is raised on a supported platform if the
plugin runtime is not installed: the conversion needs `DL210WebToPDF.ppi` and
its bundled CEF runtime present alongside the APDFL libraries. If you see this
error on Windows, Linux, or an Apple-silicon Mac, check for the plugin before
looking anywhere else.

## Notes

The plugin exposes only process-global state for a conversion's result and its
last-error detail, so concurrent calls to `from_web_url` and
`from_html_file` are serialized internally. Both are safe to call from multiple
threads — results and error messages will not be crossed up — but parallel
calls will not deliver parallel throughput. Parallelize across separate
processes if you need it.
