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
