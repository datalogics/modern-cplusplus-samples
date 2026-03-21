/*
 * This sample demonstrates drawing a list of grayscale separations from a PDF file to
 * a multi-page TIFF file.
 *
 * Copyright (c) 2007-2025, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "GetSeparatedImages Sample:" << std::endl;

    std::string input_path  = Library::get_resource_directory() + "Sample_Input/ducky.pdf";
    std::string output_path = "GetSeparatedImages-out.tiff";

    if (argc > 1) input_path  = argv[1];
    if (argc > 2) output_path = argv[2];

    std::cout << "Input file: " << input_path
              << ", will write to " << output_path << std::endl;

    try {
        Library lib;

        Document doc(input_path);
        Page pg = doc.get_page(0);

        // Get all inks that are present on the page
        std::vector<Ink> inks = pg.list_inks();

        // Build SeparationColorSpace for each ink
        std::vector<std::shared_ptr<SeparationColorSpace>> colorants;
        for (Ink& ink : inks) {
            colorants.push_back(std::make_shared<SeparationColorSpace>(pg, ink));
        }

        PageImageParams pip;
        pip.set_page_draw_flags(DrawFlags::UseAnnotFaces);
        pip.set_horizontal_resolution(300);
        pip.set_vertical_resolution(300);

        // Get per-ink separation images as a collection
        ImageCollection images = pg.get_image_separations(pg.get_crop_box(), pip, colorants);

        // Save as multi-page TIFF — each page is a separated color channel
        images.save(output_path, ImageType::TIFF);
        std::cout << "Saved " << output_path << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
