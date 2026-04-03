/*
 * This sample demonstrates drawing a list of grayscale separations from a PDF file.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "DrawSeparations Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input_path  = Library::get_resource_directory() + "Sample_Input/ducky.pdf";
    std::string output_stem = "DrawSeparations-out";

    if (argc > 1) input_path  = argv[1];
    if (argc > 2) output_stem = argv[2];

    std::cout << "Input file: " << input_path
              << ", writing output using prefix: " << output_stem << std::endl;

    try {
        Document doc(input_path);
        Page pg = doc.get_page(0);

        // Get all inks that are present on the page
        std::vector<Ink> inks = pg.list_inks();

        // Build SeparationColorSpace objects for each ink
        std::vector<std::shared_ptr<SeparationColorSpace>> colorants;
        for (Ink& ink : inks) {
            colorants.push_back(std::make_shared<SeparationColorSpace>(pg, ink));
        }

        PageImageParams pip;
        pip.set_page_draw_flags(DrawFlags::UseAnnotFaces);
        pip.set_horizontal_resolution(300);
        pip.set_vertical_resolution(300);

        // Get per-ink separation images
        ImageCollection images = pg.get_image_separations(pg.get_crop_box(), pip, colorants);

        // Save each separation as an individual PNG
        for (int i = 0; i < images.get_count(); ++i) {
            Image sep_img = images.get_image(i);
            std::string name = colorants[i]->get_separation_name();
            std::string filename = output_stem + std::to_string(i) + "-" + name + ".png";
            sep_img.save(filename, ImageType::PNG);
            std::cout << "Saved " << filename << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
