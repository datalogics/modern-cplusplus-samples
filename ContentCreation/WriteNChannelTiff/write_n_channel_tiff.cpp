/*
 * This sample generates a multi-page TIFF file, selecting graphics drawn from
 * the first page of the PDF document provided.
 *
 * Copyright (c) 2007-2024, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/library.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/page.hpp>
#include <datalogics_interface/ink.hpp>
#include <datalogics_interface/separation_color_space.hpp>
#include <datalogics_interface/page_image_params.hpp>
#include <datalogics_interface/image.hpp>
#include <datalogics_interface/geometry.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "WriteNChannelTiff Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string s_input = Library::get_resource_directory() + "Sample_Input/sample.pdf";
    std::string s_output = "WriteNChannelTiff-out.tif";

    if (argc > 1) s_input = argv[1];
    if (argc > 2) s_output = argv[2];

    std::cout << "Input file: " << s_input << " writing to " << s_output << std::endl;

    Document doc(s_input);
    Page pg = doc.get_page(0);

    // Get all inks present on the page
    auto inks = pg.list_inks();
    std::vector<std::shared_ptr<SeparationColorSpace>> colorants;

    for (auto& ink : inks) {
        // If the ink can't be found in page resources,
        // default tintTransform and alternate will be used
        colorants.push_back(std::make_shared<SeparationColorSpace>(pg, ink));
    }

    PageImageParams pip;
    pip.set_horizontal_resolution(300);
    pip.set_vertical_resolution(300);

    // Get the image with all ink separations as a multi-channel TIFF
    auto images = pg.get_image_separations(pg.get_crop_box(), pip, colorants);
    images.save(s_output, ImageType::TIFF);

    std::cout << "Created " << s_output << std::endl;
    return 0;
}
