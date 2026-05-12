/*
 * The ExportFormsData sample demonstrates how to export forms data from XFA and AcroForms documents:
 *
 *  - Export data from a XFA (Dynamic or Static) document; supported types: XDP, XML, or XFD
 *  - Export data from an AcroForms document; supported types: XFDF, FDF, or XML
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "ExportFormsData Sample:" << std::endl;

    Library lib(LibraryFlags::InitFormsExtension);

    if (!lib.is_forms_extension_available()) {
        std::cout << "Forms Plugins were not properly loaded!" << std::endl;
        return 1;
    }

    lib.set_allow_opening_xfa(true);

    std::cout << "Initialized the library." << std::endl;

    // XFA document
    std::string xfa_input   = Library::get_resource_directory() + "Sample_Input/DynamicXFA.pdf";
    std::string xfa_output  = "ExportFormsDataXFA.xdp";

    if (argc > 1) xfa_output = argv[1];

    try {
        {
            Document doc(xfa_input);

            // Export the data specifying XDP type
            bool result = doc.export_xfa_forms_data(xfa_output, XFAFormExportType::XDP);

            if (result) {
                std::cout << "Forms data was exported!" << std::endl;
            } else {
                std::cout << "Exporting of Forms data failed!" << std::endl;
            }
        }

        // AcroForms document
        std::string acro_input  = Library::get_resource_directory() + "Sample_Input/AcroForm.pdf";
        std::string acro_output = "ExportFormsDataAcroForms.xfdf";

        if (argc > 2) acro_output = argv[2];

        {
            Document doc(acro_input);

            // Export the data specifying XFDF type
            bool result = doc.export_acro_forms_data(acro_output, AcroFormExportType::XFDF);

            if (result) {
                std::cout << "Forms data was exported!" << std::endl;
            } else {
                std::cout << "Exporting of Forms data failed!" << std::endl;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
