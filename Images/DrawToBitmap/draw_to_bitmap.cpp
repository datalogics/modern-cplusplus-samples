/*
 * This program sample converts a PDF file to a series of image files.
 *
 * Copyright (c) 2007-2023, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>
#include <cmath>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "DrawToBitmap Sample" << std::endl;

    try {
        Library lib;
        std::cout << "Initialized the library." << std::endl;

        std::string input_path = Library::get_resource_directory() + "Sample_Input/ducky.pdf";

        if (argc > 1) input_path = argv[1];

        std::cout << "Input file: " << input_path << std::endl;

        Document doc(input_path);
        Page pg = doc.get_page(0);

        const double resolution = 96.0;
        double scale_factor = resolution / 72.0;

        Rect media_box = pg.get_media_box();

        // Set up PageImageParams with resolution scaling
        PageImageParams pip;
        pip.set_page_draw_flags(DrawFlags::UseAnnotFaces | DrawFlags::DoLazyErase);
        pip.set_horizontal_resolution(resolution);
        pip.set_vertical_resolution(resolution);
        pip.set_black_point_compensation(true);

        // Render and save the page as PNG
        Image page_image = pg.get_image(media_box, pip);
        page_image.save("DrawToBitmap-out.png", ImageType::PNG);
        std::cout << "Created DrawToBitmap-out.png" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An exception occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
