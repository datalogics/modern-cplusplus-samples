/*
 * This sample demonstrates how to import an image into a PDF file.
 *
 * Copyright (c) 2007-2025, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "Import Images Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input_path  = Library::get_resource_directory() + "Sample_Input/ducky.jpg";
    std::string output_path = "ImageImport-out1.pdf";

    if (argc > 1) input_path  = argv[1];
    if (argc > 2) output_path = argv[2];

    std::cout << "Reading image file " << input_path
              << " and writing " << output_path << std::endl;

    try {
        Document doc;

        // Create the image and associate it with the document
        Image new_image(input_path, doc);

        // Create a PDF page one inch larger all around than the image.
        // The design width and height are in the Matrix a and d fields.
        Matrix img_matrix = new_image.get_matrix();
        Rect page_rect{0, 0, img_matrix.a + 144, img_matrix.d + 144};

        Page doc_page = doc.create_page(Document::before_first_page, page_rect);

        // Center the image on the page (72 pts = 1 inch margin)
        new_image.translate(72, 72);

        Content content = doc_page.get_content();
        content.add_element(new_image);
        doc_page.update_content();

        doc.save(SaveFlags::Full, output_path);
        std::cout << "Saved " << output_path << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
