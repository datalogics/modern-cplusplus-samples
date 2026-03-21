/*
 * This sample demonstrates converting an XPS file into a PDF document.
 *
 * XML Paper Specification (XPS) is a standard document format that Microsoft created in 2006
 * as an alternative to the PDF format.
 *
 * Copyright (c) 2007-2025, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "CreateDocFromXPS Sample:" << std::endl;

    Library lib;

    std::string s_input = Library::get_resource_directory() + "Sample_Input/brownfox.xps";
    std::string s_output = "CreateDocFromXPS-out.pdf";

    if (argc > 1)
        s_input = argv[1];
    if (argc > 2)
        s_output = argv[2];

    std::cout << "Input file: " << s_input << ", writing to " << s_output << std::endl;

    // First, create an XPSConvertParams to specify conversion parameters
    // for creating the document.
    XPSConvertParams xps_params;

    // PDFL requires a .joboptions file to specify settings for XPS conversion.
    // A default .joboptions file is provided in the Resources directory of
    // the PDFL distribution. This file is used by default, but a custom file
    // can be used instead by calling xps_params.set_settings_file().
    std::cout << "Using settings file located at: " << xps_params.get_settings_file() << std::endl;

    // Create the document from the XPS file.
    std::cout << "Creating a document from an XPS file..." << std::endl;
    Document doc(s_input, xps_params);

    // Save the document.
    std::cout << "Saving the document..." << std::endl;
    doc.save(SaveFlags::Full, s_output);

    return 0;
}
