/*
 * This sample program searches through the PDF file that you select and identifies drawings,
 * diagrams and photographs from input streams.
 *
 * A stream is a string of bytes of any length. This program demonstrates constructing
 * an Image object from a file stream.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <fstream>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "ImageFromStream Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string jpeg_input  = Library::get_resource_directory() + "Sample_Input/ducky.jpg";
    std::string doc_output  = "ImageFromStream-out.pdf";

    if (argc > 1) jpeg_input = argv[1];
    if (argc > 2) doc_output = argv[2];

    std::cout << "Using jpeg input " << jpeg_input
              << ". Writing to output " << doc_output << std::endl;

    try {
        // Open a JPEG file as an input stream and create an Image from it.
        std::ifstream jpeg_stream(jpeg_input, std::ios::binary);
        if (!jpeg_stream) {
            std::cerr << "Cannot open input file: " << jpeg_input << std::endl;
            return 1;
        }

        // Create a new Document and add a page to it.
        Document doc;
        doc.create_page(Document::before_first_page, Rect{0, 0, 612, 792});

        // Create the Image from the stream, associated with the document to optimize
        // data usage within the document.
        Image pdfl_image(jpeg_stream, doc);

        // Save the image directly to a PNG file as well
        pdfl_image.save("ImageFromStream-out.png", ImageType::PNG);
        std::cout << "Saved ImageFromStream-out.png" << std::endl;

        // Place the image on the first page
        Page pg = doc.get_page(0);
        Content content = pg.get_content();
        content.add_element(pdfl_image);
        pg.update_content();

        doc.save(SaveFlags::Full, doc_output);
        std::cout << "Saved " << doc_output << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
