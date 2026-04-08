/*
 * This sample shows how to rasterize a page from a PDF document and save that page as an
 * image file. The sample demonstrates using the PageImageParams object with the get_image
 * method.
 *
 * The program creates two images:
 *
 * 1. An output image with a pixel width of 400 at 300 DPI.
 *
 * 2. An output image half the physical size of a PDF page at 96 DPI.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

static PageImageParams scale_page(const Page& pg, double scale_factor, double resolution)
{
    double user_width;
    double user_height;
    Rect crop_box = pg.get_crop_box();
    PageRotation rotation = pg.get_rotation();

    if (rotation == PageRotation::Rotate90 || rotation == PageRotation::Rotate270) {
        user_width  = crop_box.ur_y - crop_box.ll_y;
        user_height = crop_box.ur_x - crop_box.ll_x;
    } else {
        user_width  = crop_box.ur_x - crop_box.ll_x;
        user_height = crop_box.ur_y - crop_box.ll_y;
    }

    double phys_width  = (user_width  / 72.0) * scale_factor;
    double phys_height = (user_height / 72.0) * scale_factor;

    PageImageParams pip;
    pip.set_page_draw_flags(DrawFlags::UseAnnotFaces);
    pip.set_pixel_width(static_cast<int>(phys_width  * resolution));
    pip.set_pixel_height(static_cast<int>(phys_height * resolution));
    pip.set_horizontal_resolution(resolution);
    pip.set_vertical_resolution(resolution);

    return pip;
}

int main(int argc, char* argv[])
{
    std::cout << "RasterizePage Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input_path  = Library::get_resource_directory() + "Sample_Input/ducky.pdf";
    std::string output_stem = "RasterizePage";

    if (argc > 1) input_path  = argv[1];
    if (argc > 2) output_stem = argv[2];

    std::cout << "Using input file " << input_path
              << " writing to output with prefix " << output_stem << std::endl;

    try {
        Document doc(input_path);
        Page pg = doc.get_page(0);

        // -------------------------------------------------------
        // Image 1: exactly 400 pixels wide at 300 DPI
        // -------------------------------------------------------
        PageImageParams pip;
        pip.set_page_draw_flags(DrawFlags::UseAnnotFaces);
        pip.set_pixel_width(400);
        // Default resolution is 300 DPI; height is auto-calculated.

        Image img1 = pg.get_image(pg.get_crop_box(), pip);
        std::string out1 = output_stem + "-400pixel-width.png";
        img1.save(out1, ImageType::PNG);
        std::cout << "Created " << out1 << std::endl;

        // -------------------------------------------------------
        // Image 2: grayscale, half the physical size at 96 DPI
        // -------------------------------------------------------
        // Note: set_image_color_space requires a ColorSpace& but DeviceGray
        // is a singleton; use the page_image_params ICC path approach or
        // simply rely on default color space. For a true gray output the
        // caller would need a DeviceGray ColorSpace object; we demonstrate
        // the resolution/scale approach here without forcing gray.
        PageImageParams pip2 = scale_page(pg, 0.5, 96.0);
        pip2.set_page_draw_flags(DrawFlags::UseAnnotFaces);

        Image img2 = pg.get_image(pg.get_crop_box(), pip2);
        std::string out2 = output_stem + "-halfsize.png";
        img2.save(out2, ImageType::PNG);
        std::cout << "Created " << out2 << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
