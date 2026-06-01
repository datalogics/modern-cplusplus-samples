# Modern C++ API for Web-to-PDF Conversion — Design Spec

**Status:** Draft for review
**Date:** 2026-06-01
**Target release:** v1 of the modern interface's web-to-PDF support
**Replaces:** prototype call site at `DocumentConversion/CreateDocFromWebPage/create_doc_from_webpage.cpp`

## 1. Summary

This document specifies a new public API in `namespace datalogics_interface` for converting web pages and local HTML files to PDF. The API sits atop the existing APDFL `WebToPDF` plugin (which uses Chromium Embedded Framework). It enables idiomatic modern-C++ usage and unblocks .NET and Java extension through the existing DLE projection layer.

The design preserves the feature set of the prototype call site while correcting departures from established modern-interface conventions, introducing structured error reporting, and providing a return shape that carries per-conversion metadata alongside the produced `Document` without polluting `Document` with feature-specific state.

## 2. Goals

- Idiomatic modern-C++ surface: RAII, exceptions, value semantics, `enum class`, `std::function` callbacks, `std::string`. No C-layer types (`ASInt32`, `const char*`, `void* clientData`) leak through.
- Clean projection to .NET and Java through existing DLE conventions, with no novel marshaling patterns required.
- Faithful coverage of the prototype's feature scope: viewport, page geometry, margins, image compression, downsampling, background printing, timeout, progress and log callbacks.
- Structured access to per-conversion metadata (`pageCount`, `conversionTimeMs`, `title`) via the factory's return value, not via `Document` state.
- Structured error reporting (category, underlying plugin code, textual detail) instead of opaque runtime errors.
- Consistency with established modern-interface patterns: opaque pImpl params types, `detail::*Access` friend-accessors, `std::function`-to-functor-base bridge for callbacks.

## 3. Non-Goals (deferred to a later release)

- Authentication: custom HTTP headers, cookies, user-agent override, proxy configuration.
- Asynchronous conversion (`std::future<...>`-returning entry points). Callers wanting async wrap the synchronous factory in `std::async` or the equivalent in their host language.
- HTML-from-`std::string` and HTML-from-stream input modes.
- Promoting `WebConvertError` to a shared `ConversionError` base for other conversion APIs. The exception type is shaped so this can be done retroactively without source-breaking changes.
- A general `OperationCancelledException` adoption sweep across other long-running APIs.

## 4. Public API Surface

### 4.1 Entry points (additions to `document.hpp`)

```cpp
class DL_API Document {
public:
    // ...existing constructors and methods...

    /// Render a live web page (http/https) and produce a Document.
    /// Throws WebConvertError on fetch/render failure.
    /// Throws OperationCancelledException if the progress callback returns false.
    /// @copydoc CDocument::FromWebUrl
    static WebConvertResult from_web_url(const std::string& url,
                                         const WebConvertParams& params);

    /// Render a local HTML file. Relative asset references are resolved
    /// against the file's directory.
    /// Throws WebConvertError on read/render failure.
    /// Throws OperationCancelledException if the progress callback returns false.
    /// @copydoc CDocument::FromHtmlFile
    static WebConvertResult from_html_file(const std::string& path,
                                           const WebConvertParams& params);
};
```

Both entry points are `static` factory methods. They take the input as `std::string` and a `const WebConvertParams&`, and return a `WebConvertResult` by value. The factory does not retain a reference to `params` after returning.

**Rationale for static factories rather than constructor overloads.** The closest existing precedent is `Document(const std::string& xps_file, XPSConvertParams& params)`, which uses a constructor and disambiguates by the underlying file's content. For web-to-PDF, both inputs are `std::string`, and a constructor overload would be ambiguous. Static factories make the caller's intent explicit, project naturally to .NET (`Document.FromWebUrl(...)`, `Document.FromHtmlFile(...)`) and Java (`Document.fromWebUrl(...)`, `Document.fromHtmlFile(...)`), and avoid any "is this a URL or a path?" inference logic.

### 4.2 `WebConvertParams` (new header `web_convert_params.hpp`)

```cpp
#pragma once

#include <datalogics_interface/export.h>
#include <functional>
#include <memory>
#include <string>

namespace datalogics_interface {

enum class WebViewportPreset : int {
    Desktop = 0,    ///< Wide desktop layout (e.g. 1280px-class)
    Tablet  = 1,    ///< Tablet layout
    Mobile  = 2,    ///< Narrow mobile layout
    Custom  = 3,    ///< Use the explicit width/height set on params
};

enum class WebPageOrientation : int {
    Portrait  = 0,
    Landscape = 1,
};

enum class WebPageSize : int {
    Letter = 0,
    Legal  = 1,
    A4     = 2,
    A3     = 3,
    Custom = 4,     ///< Use the explicit width/height (inches) set on params
};

enum class WebImageCompression : int {
    JPEG     = 0,
    Flate    = 1,
    JPEG2000 = 2,
};

enum class WebLogLevel : int {
    Error   = 0,
    Warning = 1,
    Info    = 2,
    Debug   = 3,
};

namespace detail { class WebConvertParamsAccess; }

/// Parameters controlling Document::from_web_url and Document::from_html_file.
/// Plain value type. Safe to construct and pass by reference.
/// @copydoc CWebToPDFConvertParams
class DL_API WebConvertParams {
public:
    WebConvertParams();
    ~WebConvertParams();

    WebConvertParams(const WebConvertParams&);
    WebConvertParams& operator=(const WebConvertParams&);
    WebConvertParams(WebConvertParams&&) noexcept;
    WebConvertParams& operator=(WebConvertParams&&) noexcept;

    // ── Viewport ──
    WebViewportPreset get_viewport_preset() const;
    void set_viewport_preset(WebViewportPreset preset);
    int  get_viewport_width_px() const;          ///< Used when preset == Custom.
    void set_viewport_width_px(int px);
    int  get_viewport_height_px() const;
    void set_viewport_height_px(int px);

    // ── Page geometry ──
    WebPageSize        get_page_size() const;
    void               set_page_size(WebPageSize size);
    double             get_page_width_inches() const;   ///< Used when size == Custom.
    void               set_page_width_inches(double w);
    double             get_page_height_inches() const;
    void               set_page_height_inches(double h);
    WebPageOrientation get_page_orientation() const;
    void               set_page_orientation(WebPageOrientation o);

    // ── Margins (inches) ──
    double get_margin_top_inches() const;
    double get_margin_right_inches() const;
    double get_margin_bottom_inches() const;
    double get_margin_left_inches() const;
    void   set_margins_inches(double top, double right, double bottom, double left);

    // ── Image handling ──
    WebImageCompression get_image_compression() const;
    void                set_image_compression(WebImageCompression c);
    int                 get_downsampling_dpi() const;   ///< 0 = no downsampling.
    void                set_downsampling_dpi(int dpi);

    // ── Behavior ──
    bool get_print_background() const;
    void set_print_background(bool value);
    int  get_timeout_seconds() const;                   ///< 0 = no timeout.
    void set_timeout_seconds(int seconds);

    // ── Callbacks (see Section 6) ──
    using ProgressCallback = std::function<bool(int page, int total_pages, double fraction)>;
    using LogCallback      = std::function<void(WebLogLevel level, const std::string& message)>;
    void set_progress_callback(ProgressCallback cb);
    void set_log_callback(LogCallback cb);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    friend class detail::WebConvertParamsAccess;
};

}  // namespace datalogics_interface
```

### 4.3 `WebConvertInfo` and `WebConvertResult`

```cpp
namespace detail { class WebConvertInfoAccess; }

/// Metadata about a successful web/HTML conversion.
/// @copydoc CWebConvertInfo
class DL_API WebConvertInfo {
public:
    WebConvertInfo();
    ~WebConvertInfo();
    WebConvertInfo(const WebConvertInfo&);
    WebConvertInfo& operator=(const WebConvertInfo&);
    WebConvertInfo(WebConvertInfo&&) noexcept;
    WebConvertInfo& operator=(WebConvertInfo&&) noexcept;

    int         get_page_count() const;             ///< pages written; 0 if unknown
    int         get_conversion_time_ms() const;     ///< 0 if unknown
    std::string get_title() const;                  ///< empty if the page had no <title>

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    friend class detail::WebConvertInfoAccess;
};

/// Result of a successful web/HTML conversion.
/// Designed for structured-binding destructuring at the call site.
struct DL_API WebConvertResult {
    Document       document;
    WebConvertInfo info;
};
```

### 4.4 Exception types

```cpp
/// Thrown by web-to-PDF conversion when the underlying plugin reports a
/// non-cancellation failure. Carries the C-layer result code and textual
/// detail in addition to the human-readable what() string.
class DL_API WebConvertError : public std::runtime_error {
public:
    enum class Category : int {
        InvalidArgument,    ///< Malformed URL, empty path, file not found
        PluginUnavailable,  ///< WebToPDF plugin / HFT not located
        Network,            ///< DNS, TLS, HTTP 4xx/5xx
        Timeout,            ///< timeout_seconds exceeded
        RenderFailed,       ///< CEF/page-script error
        OutputFailed,       ///< Could not produce the PDF
        Unknown,
    };

    WebConvertError(Category category,
                    int plugin_result_code,
                    const std::string& detail);

    Category    category() const noexcept;
    int         plugin_result_code() const noexcept;  ///< WebToPDFResultCode value
    const std::string& detail() const noexcept;       ///< from WebToPDFGetLastError
};

/// Thrown when a user-supplied progress callback returns false to cancel
/// a long-running operation. Distinct from WebConvertError so that
/// non-coupling catch handlers can quietly handle cancellation without
/// also being aware of the web-conversion subsystem.
class DL_API OperationCancelledException : public std::runtime_error {
public:
    OperationCancelledException();
    explicit OperationCancelledException(const std::string& message);
};
```

## 5. Convention deltas from the prototype

The prototype at `DocumentConversion/CreateDocFromWebPage/create_doc_from_webpage.cpp` works as a sketch but departs from established modern-interface patterns in several places. The deltas below resolve those.

| Prototype | This design | Rationale |
|---|---|---|
| `WebConvertParams` struct with public fields | Opaque pImpl class with `get_`/`set_` methods | Matches `PDFAConvertParams`, `XPSConvertParams`. ABI-stable. Cleanly marshalable across DLE. |
| `kWebToPDFViewportDesktop`, `kWebToPDFOrientationPortrait`, … | `WebViewportPreset::Desktop`, `WebPageOrientation::Portrait`, … | Established `enum class` convention; no `kFoo` constants anywhere else in the modern interface. |
| `ASInt32` in progress callback | `int` | Hides C-layer type from the public surface. |
| `const char*` in log callback | `const std::string&` | C `char*` does not marshal cleanly across JNI/PInvoke; `std::string` is the predictable conversion path. |
| `kWebToPDFDPI300` enum constants | `int dpi` (`0` means "no downsampling") | Arbitrary DPI, not a fixed menu. |
| Numeric `level` with magic-number switch | `WebLogLevel` enum class | Eliminates the switch-on-magic-number in caller code. |
| `params.margins.top = 0.5; …` four-statement assignment | `set_margins_inches(top, right, bottom, left)` | Single call; obvious tuple semantics; no extra nested public type for DLE to project. |
| Viewport: preset enum only | Preset enum **or** explicit width/height via `Custom` | Real browsers render at pixel dimensions; a preset-only menu is too restrictive long-term. |
| `WebSourceType` enum to discriminate URL vs file | Two static factories: `from_web_url` and `from_html_file` | Caller intent is explicit; no error-prone enum/string-mismatch combinations; cleaner .NET/Java projection. |
| `Document` returned, no metadata path | `WebConvertResult { Document; WebConvertInfo }` returned | Carries `pageCount`/`conversionTimeMs`/`title` from `WebToPDFGetConversionInfo` without polluting `Document`. Structured-binding ergonomics keep the simple-case call site terse. |
| Single `std::runtime_error` if anything fails | `WebConvertError` with `Category`/`plugin_result_code`/`detail`, plus separate `OperationCancelledException` for cancellation | Preserves the C layer's categorical code and `WebToPDFGetLastError` text; allows callers to branch programmatically. |

## 6. Callbacks

The two callbacks use the established `std::function` → private-bridge → `C*`-base pattern from `word_finder.cpp` (`WordProcBridge`) and `document.cpp` (`StdFunctionEnumProc`).

### 6.1 Progress callback

```cpp
using ProgressCallback = std::function<bool(int page, int total_pages, double fraction)>;
```

| Parameter | Meaning |
|---|---|
| `page` | 1-based index of the page currently being emitted; `0` before the first page is known. |
| `total_pages` | Best estimate of the final page count; `-1` if not yet known. Web rendering frequently cannot predict this until late in the layout phase. |
| `fraction` | Progress in `[0.0, 1.0]`. Monotonically non-decreasing. May be coarse, driven by underlying plugin phases. |
| return value | `true` to continue rendering; `false` to cancel. Matches `WordFinder::enum_words` and `Document::for_each_indirect_object` convention. |

When the callback returns `false`, the bridge translates that to the C-layer abort signal. After the C layer returns control, the modern-interface code throws `OperationCancelledException`. No partial `Document` is returned.

### 6.2 Log callback

```cpp
using LogCallback = std::function<void(WebLogLevel level, const std::string& message)>;
```

Returns `void` (no per-line cancellation). Level is `enum class WebLogLevel`. Message is `std::string` (always non-null, possibly empty).

### 6.3 Bridge ownership

Each bridge owns its `std::function` by value (`std::move`d in by the factory before kicking off the synchronous render). This is the safer of the two existing patterns in the codebase. The bridges are stack-local inside the factory and never outlive the synchronous call, so capturing references in user lambdas is safe.

For DLE: .NET delegates and Java functional-interface impls are kept alive across the JNI/PInvoke boundary by the DLE shim holding a GC root for the duration of the call. The by-value `std::function` semantics fit that exactly.

## 7. Cancellation and error semantics

```cpp
try {
    auto [doc, info] = Document::from_web_url(url, params);
    doc.save(SaveFlags::Full, "out.pdf");
} catch (const OperationCancelledException&) {
    // User pressed Cancel — quiet handling.
} catch (const WebConvertError& e) {
    log("Web conversion failed: [", static_cast<int>(e.category()),
        " / plugin code ", e.plugin_result_code(), "] ",
        e.detail());
} catch (const std::exception& e) {
    log("Unexpected: ", e.what());
}
```

`WebConvertError::what()` is composed as `"[Category] detail"` so legacy `catch (const std::exception&)` handlers still get readable text. Typed catches let callers branch on `category()` (programmatic handling) or surface `plugin_result_code()` (support diagnostics).

`OperationCancelledException` is deliberately not a subtype of `WebConvertError`. It is a control-flow signal usable by any cancellable operation in the modern interface, present and future. Callers that want to "quietly handle cancellation everywhere" can write one `catch` for it without coupling to the web-conversion subsystem.

## 8. Lifetime, ownership, threading

- `WebConvertParams` and `WebConvertInfo` are **value types** (fully copyable and movable), consistent with `PDFAConvertParams` and `XPSConvertParams`. Safe to construct on the stack and discard after the call.
- The `Document` inside `WebConvertResult` is the same `Document` everyone else already uses; same destructor, same move semantics, nothing new to learn.
- The two callbacks stored inside the params are owned by value. They live as long as the params object. The bridges that wrap them are stack-local inside the factory.
- A `WebConvertParams` instance is **not** thread-safe for concurrent mutation. Standard C++ convention.
- The factories themselves are callable from any thread. Each conversion is single-threaded internally from the caller's POV (the C plugin's threading details are an implementation matter).
- **The progress and log callbacks may be invoked on any thread the underlying plugin chooses.** The C plugin's documentation states that log messages are emitted on the plugin's worker thread. GUI callers in .NET/Java must marshal back to their UI thread; the modern API documents this explicitly.

## 9. Implementation layering

### 9.1 Layer summary

Adding this feature requires changes in two layers, because the underlying `C*` interface layer has no existing web/HTML wrapper class.

```
Modern interface (NEW)
  WebConvertParams, WebConvertInfo, WebConvertResult,
  WebConvertError, OperationCancelledException,
  Document::from_web_url, Document::from_html_file
        │
        │  detail::*Access friend-accessors + private bridges
        ▼
C++ Interface layer (NEW)
  CWebToPDFConvertParams, CWebConvertInfo, CWebConvertError,
  CWebProgressProc (functor base), CWebLogProc (functor base),
  CDocument::FromWebUrl, CDocument::FromHtmlFile
        │
        │  HFT lookup + function-pointer dispatch via WebToPDFCalls.h
        ▼
APDFL plugin layer (EXISTING)
  WebToPDFInitialize, WebToPDFTerminate,
  WebToPDFConvertURL, WebToPDFConvertHTMLFile,
  WebToPDFGetConversionInfo, WebToPDFGetLastError,
  CEF (Chromium Embedded Framework) under the hood
```

### 9.2 Layer 1 — new `C*` interface classes

Files added under `src/Interface/`:

- `CWebProgressProc.h/.cpp` — abstract base with `virtual ASBool operator()(ASInt32 page, ASInt32 total, float fraction)`.
- `CWebLogProc.h/.cpp` — abstract base with `virtual void operator()(int level, const char* message)`.
- `CWebToPDFConvertParams.h/.cpp` — pImpl over `WebToPDFParamsRec`. Initializes via `WebToPDFInitParams`, exposes get/set, and stores references to a `CWebProgressProc` and `CWebLogProc` for the static C trampolines to dispatch to via `clientData`.
- `CWebConvertInfo.h/.cpp` — pImpl over `WebToPDFConversionInfoRec`. Populated by `CDocument::FromWebUrl` / `FromHtmlFile` immediately after a successful convert.
- `CWebConvertError.h/.cpp` — exception type carrying the `WebToPDFResultCode` plus the textual message obtained from `WebToPDFGetLastError`.
- `CDocument` additions: `static CDocument FromWebUrl(const std::string& url, CWebToPDFConvertParams& params, CWebConvertInfo& info_out)` and the parallel `FromHtmlFile`. Each calls the plugin's convert entry point, then `WebToPDFGetConversionInfo` to populate `info_out`. On a non-success result, throws `CWebConvertError` after pulling the message out of `WebToPDFGetLastError`.

### 9.3 Layer 2 — modern interface

Files added under `api/include/datalogics_interface/` and `api/src/`:

- `web_convert_params.hpp` and `web_convert_params.cpp` — the public surface listed in §4.2 / §4.3 / §4.4, plus the private `Impl` carrying a `CWebToPDFConvertParams` and the stored `std::function` callbacks.
- Additions to `document.hpp` and `document.cpp` — the two static factories. The `.cpp` defines two anonymous-namespace bridge classes:

```cpp
// inside document.cpp's anonymous namespace
class WebProgressBridge : public CWebProgressProc {
public:
    using Callback = WebConvertParams::ProgressCallback;
    explicit WebProgressBridge(Callback cb) : cb_(std::move(cb)) {}
    ASBool operator()(ASInt32 page, ASInt32 total, float fraction) override {
        if (!cb_) return 1;
        return cb_(static_cast<int>(page),
                   static_cast<int>(total),
                   static_cast<double>(fraction)) ? 1 : 0;
    }
private:
    Callback cb_;
};

class WebLogBridge : public CWebLogProc {
public:
    using Callback = WebConvertParams::LogCallback;
    explicit WebLogBridge(Callback cb) : cb_(std::move(cb)) {}
    void operator()(int level, const char* message) override {
        if (!cb_) return;
        cb_(static_cast<WebLogLevel>(level),
            message ? std::string(message) : std::string{});
    }
private:
    Callback cb_;
};
```

The factory body:

```cpp
WebConvertResult Document::from_web_url(const std::string& url,
                                        const WebConvertParams& params) {
    WebProgressBridge prog(detail::WebConvertParamsAccess::progress_callback(params));
    WebLogBridge      log (detail::WebConvertParamsAccess::log_callback(params));

    // Copy the internal C params into a local mutable instance so that the
    // factory's `const WebConvertParams&` promise is preserved while we still
    // need to inject the bridge pointers for this call.
    CWebToPDFConvertParams cparams =
        detail::WebConvertParamsAccess::clone_internal(params);
    cparams.SetProgressProc(&prog);
    cparams.SetLogProc(&log);

    CWebConvertInfo cinfo;
    try {
        CDocument cdoc = CDocument::FromWebUrl(url, cparams, cinfo);
        WebConvertResult result;
        result.document = detail::DocumentAccess::create(std::move(cdoc));
        result.info     = detail::WebConvertInfoAccess::create(std::move(cinfo));
        return result;
    } catch (const CWebConvertError& e) {
        throw_translated(e);  // throws WebConvertError or OperationCancelledException
    }
}
```

`throw_translated` maps the C-layer result code to a modern-interface category and re-throws as `WebConvertError`, except when the code indicates user cancellation, in which case it throws `OperationCancelledException`.

### 9.4 Plugin lifecycle

The C plugin requires `WebToPDFInitialize()` before the first conversion call and `WebToPDFTerminate()` at shutdown, and `gWebToPDFHFT` must be populated from `ASExtensionMgrGetHFT`. This is process-global state.

The modern interface manages this transparently:
- HFT lookup and `WebToPDFInitialize` are performed lazily on the first `from_web_url` or `from_html_file` call, guarded by a mutex.
- `WebToPDFTerminate` is performed when the last `Library` instance is destructed, in the same teardown path as other plugin termination.
- All of this lives behind `detail::` accessors; it is not part of the public API.

If the plugin or its HFT cannot be located, the first factory call throws `WebConvertError` with `Category::PluginUnavailable` and a descriptive `detail()`. The modern API does not require callers to call `Initialize`/`Terminate` themselves.

## 10. DLE / .NET / Java projection

The modern types map cleanly onto existing DLE conventions:

| Modern C++ | .NET projection | Java projection |
|---|---|---|
| `WebConvertParams` (pImpl class with get/set) | `WebConvertParams` class with properties | `WebConvertParams` class with `getX()` / `setX()` |
| `WebConvertInfo` (pImpl, getters only) | `WebConvertInfo` class with read-only properties | `WebConvertInfo` class with `getX()` |
| `WebConvertResult { Document; WebConvertInfo }` | Tuple-deconstructable record | Immutable POJO with `document()` / `info()` |
| `enum class WebViewportPreset`, etc. | `enum WebViewportPreset` | `enum WebViewportPreset` |
| `ProgressCallback = std::function<bool(int,int,double)>` | `Func<int,int,double,bool>` delegate | `WebProgressCallback` functional interface |
| `LogCallback = std::function<void(WebLogLevel, std::string)>` | `Action<WebLogLevel,string>` delegate | `WebLogCallback` functional interface |
| `WebConvertError` (`std::runtime_error` subclass) | `WebConvertException` with `Category`/`PluginResultCode`/`Detail` properties | Checked `WebConvertException` with the same getters |
| `OperationCancelledException` | Marshals to `OperationCanceledException` (built-in) | Marshals to `CancellationException` (or a custom checked equivalent) |
| `Document::from_web_url(url, params)` | `Document.FromWebUrl(url, params)` static | `Document.fromWebUrl(url, params)` static |
| `Document::from_html_file(path, params)` | `Document.FromHtmlFile(path, params)` static | `Document.fromHtmlFile(path, params)` static |

Call-site projections:

```csharp
// .NET
var (doc, info) = Document.FromWebUrl("https://example.com", parameters);
Console.WriteLine($"Wrote {info.PageCount} pages in {info.ConversionTimeMs} ms");
doc.Save(SaveFlags.Full, "out.pdf");
```

```java
// Java
WebConvertResult result = Document.fromWebUrl("https://example.com", parameters);
System.out.println("Wrote " + result.info().getPageCount() + " pages");
result.document().save(SaveFlags.Full, "out.pdf");
```

No type in this design requires a marshaling convention the DLE doesn't already support for other modern-interface types.

## 11. Refactored sample call-site

The new sample replaces the prototype:

```cpp
/*
 * This sample demonstrates converting a web page or local HTML file into a
 * PDF document using the modern C++ interface.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "CreateDocFromWebPage Sample:" << std::endl;

    try {
        Library lib;

        std::string source = "https://www.datalogics.com";
        std::string output = "CreateDocFromWebPage-out.pdf";
        if (argc > 1) source = argv[1];
        if (argc > 2) output = argv[2];

        WebConvertParams params;
        params.set_viewport_preset(WebViewportPreset::Desktop);
        params.set_page_size(WebPageSize::Letter);
        params.set_page_orientation(WebPageOrientation::Portrait);
        params.set_margins_inches(0.5, 0.5, 0.5, 0.5);
        params.set_image_compression(WebImageCompression::JPEG);
        params.set_downsampling_dpi(300);
        params.set_print_background(true);
        params.set_timeout_seconds(300);

        params.set_progress_callback(
            [](int /*page*/, int /*total*/, double fraction) {
                std::cout << "\rProgress: " << static_cast<int>(fraction * 100)
                          << "%  " << std::flush;
                return true;
            });

        params.set_log_callback(
            [](WebLogLevel level, const std::string& message) {
                const char* lvl = "INFO";
                switch (level) {
                case WebLogLevel::Error:   lvl = "ERROR";   break;
                case WebLogLevel::Warning: lvl = "WARNING"; break;
                case WebLogLevel::Info:    lvl = "INFO";    break;
                case WebLogLevel::Debug:   lvl = "DEBUG";   break;
                }
                std::cout << "[WebToPDF " << lvl << "] " << message << std::endl;
            });

        std::cout << "Converting " << source << " ..." << std::endl;
        auto [doc, info] = Document::from_web_url(source, params);
        std::cout << std::endl;
        std::cout << "Wrote " << info.get_page_count() << " pages in "
                  << info.get_conversion_time_ms() << " ms" << std::endl;
        if (!info.get_title().empty()) {
            std::cout << "Title: " << info.get_title() << std::endl;
        }

        doc.save(SaveFlags::Full, output);
        std::cout << "Saved to " << output << std::endl;
    }
    catch (const OperationCancelledException&) {
        std::cout << std::endl << "Cancelled." << std::endl;
        return 1;
    }
    catch (const WebConvertError& e) {
        std::cerr << std::endl
                  << "Conversion failed (plugin code " << e.plugin_result_code()
                  << "): " << e.detail() << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
```

## 12. Open decisions for stakeholder input

These choices are reasonable as proposed but reviewers may want to weigh in.

- **Margin shape.** Currently a single 4-arg `set_margins_inches(top, right, bottom, left)`. Alternatives: introduce a `Margins` value type, or four individual setters (`set_margin_top_inches`, …). The current shape is the most concise and adds no new public type to project.
- **Units.** All linear dimensions are in inches. Alternatives: points (1/72 inch) or millimeters. Inches matches the prototype and is the most common unit in US-based print workflows.
- **`WebConvertResult` as a `struct`.** Chosen for structured-binding ergonomics (`auto [doc, info] = …`). Could instead be a `class` with getters. The struct is fine: members are concrete value types, no invariants to enforce.
- **Stub-valued `WebConvertInfo` in v1.** v1 ships the surface; if the implementation chooses not to populate the info fields in the first cut, the getters return safe sentinels (`0`, `0`, `""`). Flipping from stub to real values is strictly additive and will not break callers.
- **CEF-specific options** (e.g. JavaScript enable/disable, headless/headful mode). Deferred to v2 unless a stakeholder commits to needing them on day one.

## 13. Migration

The prototype call site at `DocumentConversion/CreateDocFromWebPage/create_doc_from_webpage.cpp` will be replaced wholesale with the sample shown in §11 once Layer 2 lands. There are no production callers of the prototype to migrate.

## 14. References

- C-style sample using the underlying plugin: `apdfl-cplusplus-samples/DocumentConversion/ConvertWebToPDF/ConvertWebToPDF.cpp`
- Prototype to be superseded: `modern-cplusplus-samples/DocumentConversion/CreateDocFromWebPage/create_doc_from_webpage.cpp`
- Convention exemplars (params types): `api/include/datalogics_interface/pdfa_convert_params.hpp`, `xps_convert_params.hpp`
- Convention exemplars (callback bridges): `api/src/word_finder.cpp` (`WordProcBridge`), `api/src/document.cpp` (`StdFunctionEnumProc`)
- Plugin C API: `WebToPDFCalls.h` (provides `WebToPDFConvertURL`, `WebToPDFConvertHTMLFile`, `WebToPDFGetConversionInfo`, `WebToPDFGetLastError`, `WebToPDFParamsRec`, `WebToPDFConversionInfoRec`)
