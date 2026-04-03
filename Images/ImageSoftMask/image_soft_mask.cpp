/*
 * This sample demonstrates working with masking in PDF documents. A soft mask allows
 * you to place an image on a page and define the level of transparency for that image.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "Image Soft Mask sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input_path  = Library::get_resource_directory() + "Sample_Input/ducky.jpg";
    std::string mask_path   = Library::get_resource_directory() + "Sample_Input/Mask.tif";
    std::string output_path = "ImageSoftMask-out.pdf";

    if (argc > 1) input_path  = argv[1];
    if (argc > 2) mask_path   = argv[2];
    if (argc > 3) output_path = argv[3];

    std::cout << "Input file: " << input_path
              << ", mask: " << mask_path
              << "; will write to " << output_path << std::endl;

    try {
        Document doc;
        Rect page_rect{0, 0, 612, 792};
        Page doc_page = doc.create_page(Document::before_first_page, page_rect);

        Image base_image(input_path);
        std::cout << "Created the image to mask." << std::endl;

        Image mask_image(mask_path);
        std::cout << "Created the image to use as mask." << std::endl;

        base_image.set_soft_mask(mask_image);
        std::cout << "Set the soft mask." << std::endl;

        Content content = doc_page.get_content();
        content.add_element(base_image);
        doc_page.update_content();
        std::cout << "Added element and updated content." << std::endl;

        doc.save(SaveFlags::Full, output_path);
        std::cout << "Saved " << output_path << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
