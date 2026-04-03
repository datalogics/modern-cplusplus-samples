/*
 * This sample shows how to convert a PDF document into a PDF/X compliant version of that file.
 *
 * PDF/X is used for graphics exchange when printing content. It is a version of the PDF format
 * that guarantees accuracy in colors used.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "PDFXConverter Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string s_input = Library::get_resource_directory() + "Sample_Input/sample.pdf";
    std::string s_output = "PDFXConverter-out.pdf";

    if (argc > 1)
        s_input = argv[1];
    if (argc > 2)
        s_output = argv[2];

    std::cout << "Input file: " << s_input << ". Writing to output " << s_output << std::endl;

    Document doc(s_input);

    // Make a conversion parameters object
    PDFXConvertParams pdfx_params;

    // Create a PDF/X compliant version of the document
    try {
        ConvertResult result = doc.clone_as_pdfx_document(PDFXConvertType::X4, pdfx_params);

        std::cout << "Successfully converted " << s_input << " to PDF/X." << std::endl;

        // Set the SaveFlags returned in the ConvertResult
        result.document.save(result.save_flags, s_output);
    } catch (const std::exception& ex) {
        std::cout << "ERROR: Could not convert " << s_input << " to PDF/X." << std::endl;
        std::cout << "Exception: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
