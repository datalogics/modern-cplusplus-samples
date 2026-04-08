/*
 * The ConvertXFAToAcroForms sample demonstrates how to convert XFA into AcroForms.
 * Converts XFA (Dynamic or Static) fields to AcroForms fields and removes XFA fields.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "ConvertXFAToAcroForms Sample:" << std::endl;

    Library lib(LibraryFlags::InitFormsExtension);

    if (!lib.is_forms_extension_available()) {
        std::cout << "Forms Plugins were not properly loaded!" << std::endl;
        return 1;
    }

    lib.set_allow_opening_xfa(true);

    std::cout << "Initialized the library." << std::endl;

    std::string input_path  = Library::get_resource_directory() + "Sample_Input/DynamicXFA.pdf";
    std::string output_path = "ConvertXFAToAcroForms-out.pdf";

    if (argc > 1) input_path  = argv[1];
    if (argc > 2) output_path = argv[2];

    try {
        Document doc(input_path);

        int pages_output = doc.convert_xfa_to_acroform();

        std::cout << "XFA document was converted into an AcroForms document with "
                  << pages_output << " pages." << std::endl;

        doc.save(SaveFlags::Full | SaveFlags::Linearized, output_path);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
