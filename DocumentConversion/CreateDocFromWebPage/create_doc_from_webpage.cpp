/*
 * This sample demonstrates converting a web page into a PDF document.
 *
 * A web page consists of HTML, CSS, Javascript, and any associated assets. The page
 * contents can either be from local storage or via a URL.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "CreateDocFromWebPage Sample:" << std::endl;

    Library lib;

    std::string s_input_url = "https://www.datalogics.com";
    std::string s_output = "CreateDocFromWebPage-out.pdf";

    if (argc > 1)
        s_input_url = argv[1];
    if (argc > 2)
        s_output = argv[2];

    std::cout << "Input URL: " << s_input_url << ", writing to " << s_output << std::endl;

    // First, create an WebConvertParams to specify conversion parameters
    // for creating the document.
    WebConvertParams web_params;

    web_params.viewportSize = kWebToPDFViewportDesktop;
    web_params.pageOrientation = kWebToPDFOrientationPortrait;
    web_params.pageSize = kWebToPDFPageSizeLetter;
    web_params.margins.top = 0.5;
    web_params.margins.right = 0.5;
    web_params.margins.bottom = 0.5;
    web_params.margins.left = 0.5;
    web_params.imageCompression = kWebToPDFCompressionJPEG;
    web_params.downsamplingDPI = kWebToPDFDPI300;
    web_params.printBackground = true;
    web_params.timeoutSeconds = 300;

    web_params.progressCallback = [&](ASInt32 pageNum, ASInt32 totalPages, float progress) {
        std::cout << "\rProgress: " << static_cast<int>(progress * 100) << "%  " << std::flush;
        return true;
    }
    
    web_params.logCallback = [&](ASInt32 level, const char *message) {
        const char *levelStr = "INFO";
        switch (level) {
        case 0: levelStr = "ERROR";   break;
        case 1: levelStr = "WARNING"; break;
        case 2: levelStr = "INFO";    break;
        case 3: levelStr = "DEBUG";   break;
        }
        std::cout << "[WebToPDF " << levelStr << "] " << message << std::endl;
    }

    // Create the document from the URL.
    std::cout << "Creating a document from a URL..." << std::endl;
    Document doc(s_input_url, web_params, WebSourceType_URL);

    // Save the document.
    std::cout << "Saving the document..." << std::endl;
    doc.save(SaveFlags::Full, s_output);

    return 0;
}
