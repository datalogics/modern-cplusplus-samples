/*
 * This sample demonstrates converting a web page or local HTML file into
 * a PDF document using the modern C++ interface.
 *
 * The single positional argument can be either a URL (http://, https://,
 * or file://) or a path to a local HTML file. The sample auto-detects
 * which based on the scheme prefix and routes to Document::from_web_url
 * or Document::from_html_file accordingly.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <string>

using namespace datalogics_interface;

namespace {

// Returns true if `s` looks like a URL the conversion plugin handles
// natively (http/https/file).
bool looks_like_url(const std::string& s) {
    return s.rfind("http://",  0) == 0
        || s.rfind("https://", 0) == 0
        || s.rfind("file://",  0) == 0;
}

// Human-readable name for a WebConvertError::Category.
std::string category_name(const WebConvertError::Category c) {
    switch (c) {
    case WebConvertError::Category::InvalidArgument:   return "Invalid argument";
    case WebConvertError::Category::PluginUnavailable: return "Plugin unavailable";
    case WebConvertError::Category::CEFInitFailed:     return "CEF init failed";
    case WebConvertError::Category::RenderFailed:      return "Render failed";
    case WebConvertError::Category::Timeout:           return "Timeout";
    case WebConvertError::Category::OutputFailed:      return "Output write failed";
    case WebConvertError::Category::Unknown:           return "Unknown error";
    }
    return "Unknown error";
}

}  // namespace

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
        params.set_downsampling_dpi(WebDownsamplingDPI::DPI300);
        params.set_print_background(true);
        params.set_generate_tagged_pdf(true);    // produce an accessible (tagged) PDF
        params.set_timeout_seconds(60);          // override the 300s plugin default

        params.set_progress_callback(
            [](int /*page*/, int /*total*/, double fraction) {
                std::cout << "\rProgress: " << static_cast<int>(fraction * 100)
                          << "%  " << std::flush;
                return true;
            });

        params.set_log_callback(
            [](WebLogLevel level, const std::string& message) {
                std::string lvl = "INFO";
                switch (level) {
                case WebLogLevel::Error:   lvl = "ERROR";   break;
                case WebLogLevel::Warning: lvl = "WARNING"; break;
                case WebLogLevel::Info:    lvl = "INFO";    break;
                case WebLogLevel::Debug:   lvl = "DEBUG";   break;
                }
                std::cout << "[WebToPDF " << lvl << "] " << message << std::endl;
            });

        const bool is_url = looks_like_url(source);
        std::cout << "Converting " << (is_url ? "URL " : "HTML file ")
                  << source << " ..." << std::endl;

        auto [doc, info] = is_url
            ? Document::from_web_url(source, params)
            : Document::from_html_file(source, params);

        std::cout << std::endl;
        std::cout << "Wrote " << info.get_page_count() << " pages in "
                  << info.get_conversion_time_ms() << " ms" << std::endl;
        if (!info.get_title().empty()) {
            std::cout << "Title: " << info.get_title() << std::endl;
        }
        if (!info.get_source_url().empty() && info.get_source_url() != source) {
            std::cout << "Resolved URL: " << info.get_source_url() << std::endl;
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
                  << "Conversion failed (" << category_name(e.category())
                  << "): " << e.detail() << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
