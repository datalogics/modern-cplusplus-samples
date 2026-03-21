/*
 * This sample demonstrates how to find and describe annotations in an existing PDF document.
 *
 * Copyright (c) 2007-2024, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "Annotations Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/sample_annotations.pdf";

    if (argc > 1) input_path = argv[1];

    std::cout << "Input file: " << input_path << std::endl;

    Document doc(input_path);

    Page pg = doc.get_page(0);
    auto ann = pg.get_annotation(0);

    std::cout << ann->get_title() << std::endl;
    std::cout << ann->get_subtype() << std::endl;

    return 0;
}
