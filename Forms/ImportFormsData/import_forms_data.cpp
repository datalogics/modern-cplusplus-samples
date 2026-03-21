/*
 * The ImportFormsData sample demonstrates how to import forms data into XFA and AcroForms documents:
 *
 *  - Import data into a XFA (Dynamic or Static) document; acceptable types: XDP, XML, and XFD
 *  - Import data into an AcroForms document; acceptable types: XFDF, FDF, or XML
 *
 * Copyright (c) 2024-2025, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "ImportFormsData Sample:" << std::endl;

    Library lib(LibraryFlags::InitFormsExtension);

    if (!lib.is_forms_extension_available()) {
        std::cout << "Forms Plugins were not properly loaded!" << std::endl;
        return 1;
    }

    lib.set_allow_opening_xfa(true);

    std::cout << "Initialized the library." << std::endl;

    // XFA document
    std::string xfa_input      = Library::get_resource_directory() + "Sample_Input/DynamicXFA.pdf";
    std::string xfa_data_input = Library::get_resource_directory() + "Sample_Input/DynamicXFA_data.xdp";
    std::string xfa_output     = "ImportFormsDataXFA-out.pdf";

    if (argc > 1) xfa_output = argv[1];

    try {
        {
            Document doc(xfa_input);

            // Import the data; acceptable types include XDP, XML, and XFD
            bool result = doc.import_xfa_forms_data(xfa_data_input);

            if (result) {
                std::cout << "Forms data was imported!" << std::endl;
                doc.save(SaveFlags::Full | SaveFlags::Linearized, xfa_output);
            } else {
                std::cout << "Importing of Forms data failed!" << std::endl;
            }
        }

        // AcroForms document
        std::string acro_input      = Library::get_resource_directory() + "Sample_Input/AcroForm.pdf";
        std::string acro_data_input = Library::get_resource_directory() + "Sample_Input/AcroForm_data.xfdf";
        std::string acro_output     = "ImportFormsDataAcroForms-out.pdf";

        if (argc > 2) acro_output = argv[2];

        {
            Document doc(acro_input);

            // Import the data specifying XFDF type
            bool result = doc.import_acro_forms_data(acro_data_input, AcroFormImportType::XFDF);

            if (result) {
                std::cout << "Forms data was imported!" << std::endl;
                doc.save(SaveFlags::Full | SaveFlags::Linearized, acro_output);
            } else {
                std::cout << "Importing of Forms data failed!" << std::endl;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
