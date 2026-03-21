/*
 * This sample demonstrates converting a standard PDF document into a
 * PDF Archive, or PDF/A, compliant version of a PDF file.
 *
 * Copyright (c) 2007-2025, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "PDFAConverter Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string s_input = Library::get_resource_directory() + "Sample_Input/ducky.pdf";
    std::string s_output = "PDFAConverter-out.pdf";

    if (argc > 1)
        s_input = argv[1];
    if (argc > 2)
        s_output = argv[2];

    std::cout << "Converting " << s_input << ", output file is " << s_output << std::endl;

#if PLATFORM_HAS_PDFPROCESSOR_PLUGIN
    Document doc(s_input);

    // Make a conversion parameters object
    PDFAConvertParams pdfa_params;
    pdfa_params.set_abort_if_xfa_is_present(true);
    pdfa_params.set_ignore_font_errors(false);
    pdfa_params.set_no_validation_errors(false);
    pdfa_params.set_validate_implementation_limits(true);

    // Create a PDF/A compliant version of the document
    try {
        ConvertResult result = doc.clone_as_pdfa_document(PDFAConvertType::RGB3b, pdfa_params);

        std::cout << "Successfully converted " << s_input << " to PDF/A." << std::endl;

        // Save the result using the flags returned by the conversion
        result.document.save(result.save_flags, s_output);
    } catch (const std::exception& ex) {
        std::cout << "ERROR: Could not convert " << s_input << " to PDF/A." << std::endl;
        std::cout << "Exception: " << ex.what() << std::endl;
        return 1;
    }
#else
    std::cout << "PDF/A conversion is not available on this platform." << std::endl;
#endif

    return 0;
}
