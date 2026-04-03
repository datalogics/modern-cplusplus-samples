/*
 * The FlattenForms sample demonstrates how to flatten XFA into AcroForms:
 *
 *  - Flatten XFA (Dynamic or Static) to regular page content, which converts and expands XFA
 *    fields to regular PDF content and removes the XFA fields.
 *  - Flatten AcroForms to regular page content, which converts AcroForm fields to regular
 *    page content and removes the AcroForm fields.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "FlattenForms Sample:" << std::endl;

    Library lib(LibraryFlags::InitFormsExtension);

    if (!lib.is_forms_extension_available()) {
        std::cout << "Forms Plugins were not properly loaded!" << std::endl;
        return 1;
    }

    // Must be set to true to prevent default legacy behavior of PDFL
    lib.set_allow_opening_xfa(true);

    std::cout << "Initialized the library." << std::endl;

    std::string xfa_input    = Library::get_resource_directory() + "Sample_Input/DynamicXFA.pdf";
    std::string xfa_output   = "FlattenXFA-out.pdf";
    std::string acro_input   = Library::get_resource_directory() + "Sample_Input/AcroForm.pdf";
    std::string acro_output  = "FlattenAcroForms-out.pdf";

    if (argc > 1) xfa_input   = argv[1];
    if (argc > 2) xfa_output  = argv[2];

    try {
        // Flatten XFA document
        {
            Document doc(xfa_input);

            int pages_output = doc.flatten_xfa_form_fields();

            std::cout << "XFA document was expanded into " << pages_output
                      << " Flattened pages." << std::endl;

            doc.save(SaveFlags::Full | SaveFlags::Linearized, xfa_output);
        }

        // Flatten AcroForms document
        {
            Document doc(acro_input);

            doc.flatten_acro_form_fields();

            std::cout << "AcroForms document was Flattened." << std::endl;

            doc.save(SaveFlags::Full | SaveFlags::Linearized, acro_output);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
