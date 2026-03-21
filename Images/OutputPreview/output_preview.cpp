/*
 * This sample demonstrates creating an Output Preview Image which is used during
 * Soft Proofing prior to printing to visualize combining different Colorants.
 *
 * Copyright (c) 2023-2025, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace datalogics_interface;

static std::string create_output_filename(const std::vector<std::string>& colorants)
{
    std::string name = "OutputPreview_";
    for (const auto& c : colorants) {
        name += c + "_";
    }
    name += ".tiff";
    return name;
}

int main(int argc, char* argv[])
{
    std::cout << "OutputPreview Sample:" << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/spotcolors1.pdf";

    if (argc > 1) input_path = argv[1];

    // Colorant names of interest
    std::vector<std::string> colorants_to_use  = {"Yellow", "Black"};
    std::vector<std::string> colorants_to_use2 = {"PANTONE 554 CVC", "PANTONE 814 2X CVC",
                                                   "PANTONE 185 2X CVC"};

    try {
        Library lib;

        Document doc(input_path);
        Page pg = doc.get_page(0);

        // Get all inks on the page
        std::vector<Ink> inks = pg.list_inks();

        // Build first set of colorants
        std::vector<std::shared_ptr<SeparationColorSpace>> colorants1;
        for (Ink& ink : inks) {
            for (const auto& name : colorants_to_use) {
                if (ink.get_colorant_name() == name) {
                    colorants1.push_back(std::make_shared<SeparationColorSpace>(pg, ink));
                }
            }
        }

        // Build second set of colorants
        std::vector<std::shared_ptr<SeparationColorSpace>> colorants2;
        for (Ink& ink : inks) {
            for (const auto& name : colorants_to_use2) {
                if (ink.get_colorant_name() == name) {
                    colorants2.push_back(std::make_shared<SeparationColorSpace>(pg, ink));
                }
            }
        }

        PageImageParams pip;
        pip.set_page_draw_flags(DrawFlags::UseAnnotFaces);
        pip.set_horizontal_resolution(300);
        pip.set_vertical_resolution(300);

        // Create Output Preview images using the specified colorants
        Image image1 = pg.get_output_preview_image(pg.get_crop_box(), pip, colorants1);
        std::string out1 = create_output_filename(colorants_to_use);
        image1.save(out1, ImageType::TIFF);
        std::cout << "Saved " << out1 << std::endl;

        Image image2 = pg.get_output_preview_image(pg.get_crop_box(), pip, colorants2);
        std::string out2 = create_output_filename(colorants_to_use2);
        image2.save(out2, ImageType::TIFF);
        std::cout << "Saved " << out2 << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
