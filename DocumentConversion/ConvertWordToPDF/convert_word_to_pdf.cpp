/*
 * ConvertWordToPDF
 *
 * Converts a Microsoft Word (.docx) document to PDF with the Office-to-PDF
 * plug-in, through Document::from_office_file.
 *
 * With no arguments it converts the bundled sample.docx to
 * ConvertWordToPDF-out.pdf in the current directory; pass an input .docx and an
 * output path to convert a specific file.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <string>

using namespace datalogics_interface;

namespace {

std::string category_name(const OfficeConvertError::Category c)
{
    switch (c) {
    case OfficeConvertError::Category::InputNotFound:     return "Input not found";
    case OfficeConvertError::Category::InvalidInput:      return "Invalid input";
    case OfficeConvertError::Category::InputProtected:    return "Input is password-protected";
    case OfficeConvertError::Category::ConversionFailed:  return "Conversion error";
    case OfficeConvertError::Category::PluginUnavailable: return "Plugin unavailable";
    case OfficeConvertError::Category::Unknown:           return "Unknown error";
    }
    return "Unknown error";
}

}  // namespace

int main(int argc, char* argv[])
{
    std::cout << "ConvertWordToPDF Sample:" << std::endl;

    try {
        Library lib;

        std::string input = "sample.docx";
        std::string output = "ConvertWordToPDF-out.pdf";
        if (argc > 1)
            input = argv[1];
        if (argc > 2)
            output = argv[2];

        // A fixed conversion time, so every run stamps the same dates, and
        // comments left out. A default OfficeConvertParams uses the system
        // clock and also leaves comments out.
        OfficeConvertParams params;
        params.set_conversion_time(OfficeConversionTime{2025, 1, 1, 0, 0, 0});
        params.set_comments(OfficeCommentRendering::Omit);

        std::cout << "Converting " << input << " -> " << output << std::endl;
        auto [doc, info] = Document::from_office_file(input, params);

        std::cout << "  " << info.get_page_count() << " page(s)" << std::endl;
        // Diagnostics describe how assets were rendered, such as a substituted
        // font, and can be present on success.
        for (const auto& d : info.get_diagnostics()) {
            std::cout << "  [kind " << d.get_code() << "] " << d.get_asset()
                      << ": " << d.get_message() << std::endl;
        }

        // KeepModDate keeps the fixed time as the modification date too.
        doc.save(SaveFlags::Full | SaveFlags::KeepModDate, output);
        std::cout << "Saved to " << output << std::endl;
    }
    catch (const OfficeConvertError& e) {
        std::cerr << std::endl << "Conversion failed (" << category_name(e.category())
                  << "): " << e.detail() << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
