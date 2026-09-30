/*
 * ConvertWordToPDF
 *
 * Converts a Microsoft Word (.docx) document to PDF using the Datalogics
 * Office-to-PDF SDK's modern C++ interface (office_to_pdf/converter.hpp).
 *
 * The SDK converts on the Adobe PDF Library its caller has started on the
 * calling thread, and never starts or stops it itself. So this sample creates
 * a Library, as every sample here does, and keeps it alive for the
 * conversion.
 *
 * With no arguments it converts the bundled sample.docx to
 * ConvertWordToPDF-out.pdf in the current directory; pass an input .docx and an
 * output path to convert a specific file.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <office_to_pdf/converter.hpp>

#include <iostream>
#include <string>

using namespace office_to_pdf;

namespace {

// Human-readable text for the conversion outcome, so the status line reads
// clearly in the sample's output.
const char* status_text(ConversionStatus status)
{
    switch (status) {
    case ConversionStatus::Success:                return "success";
    case ConversionStatus::InputNotFound:          return "input not found";
    case ConversionStatus::InvalidInput:           return "invalid input (not a .docx)";
    case ConversionStatus::InputProtected:         return "input is password-protected";
    case ConversionStatus::DestinationNotWritable: return "destination not writable";
    case ConversionStatus::ConversionError:        return "conversion error";
    }
    return "unknown status";
}

// Print any per-asset diagnostics the conversion reported. These describe
// notable resolutions (substituted fonts, placeholder graphics) and may be
// present even on success -- they report rendered-asset quality, not failure.
void report_diagnostics(const ConversionResult& result)
{
    if (result.diagnostics.empty())
        return;
    std::cout << "  " << result.diagnostics.size() << " diagnostic(s):" << std::endl;
    for (const auto& diagnostic : result.diagnostics) {
        std::cout << "    [kind " << static_cast<int>(diagnostic.kind) << "] "
                  << diagnostic.asset << ": " << diagnostic.message << std::endl;
    }
}

}  // namespace

int main(int argc, char* argv[])
{
    std::cout << "ConvertWordToPDF Sample:" << std::endl;

    try {
        // Start the Adobe PDF Library on this thread. The conversion runs on it.
        datalogics_interface::Library lib;

        std::string input = "sample.docx";
        std::string output = "ConvertWordToPDF-out.pdf";
        if (argc > 1)
            input = argv[1];
        if (argc > 2)
            output = argv[2];

        // Deterministic conversion options: pin the produced PDF's creation and
        // modification dates so repeated runs are byte-reproducible, and omit
        // Word comments. Calling ConvertWordToPdf(input, output) instead uses
        // the SDK defaults (system clock, comments omitted).
        ConversionOptions options;
        options.conversion_time = ConversionTimestamp{2025, 1, 1, 0, 0, 0};
        options.comments = CommentRendering::Omit;

        std::cout << "Converting " << input << " -> " << output << std::endl;

        const ConversionResult result = ConvertWordToPdf(input, output, options);

        std::cout << "  status:  " << status_text(result.status) << std::endl;
        if (!result.message.empty())
            std::cout << "  message: " << result.message << std::endl;
        report_diagnostics(result);

        if (result.status != ConversionStatus::Success)
            return 1;

        std::cout << "Saved to " << output << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
