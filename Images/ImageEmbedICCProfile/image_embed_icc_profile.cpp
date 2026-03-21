/*
 * This sample program demonstrates how to embed an ICC color profile in a graphics file.
 * The program sets up how the output will be rendered and generates TIF image files.
 *
 * Copyright (c) 2007-2023, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <datalogics_interface/named_color_space.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

static void export_image(Page& pg, int page_num, const std::string& profile_path)
{
    ImageSaveParams isp;
    isp.set_compression(CompressionCode::LZW);

    // Probev1_ICCv2.icc is a CMYK profile — render to DeviceCMYK
    auto cmyk_cs = NamedColorSpace::device_cmyk();

    PageImageParams pip;

    // Apply the ICC profile via the custom path setter
    pip.set_icc_profile_custom_path(profile_path);
    pip.set_image_color_space(cmyk_cs);

    try {
        std::string filename;

        pip.set_render_intent(RenderIntent::Saturation);
        Image img_sat = pg.get_image(pg.get_crop_box(), pip);
        filename = "ImageEmbedICCProfile-out_sat" + std::to_string(page_num) + ".tif";
        img_sat.save(filename, ImageType::TIFF, isp);
        std::cout << "Saved " << filename << std::endl;

        pip.set_render_intent(RenderIntent::AbsColorimetric);
        Image img_abs = pg.get_image(pg.get_crop_box(), pip);
        filename = "ImageEmbedICCProfile-out_abs" + std::to_string(page_num) + ".tif";
        img_abs.save(filename, ImageType::TIFF, isp);
        std::cout << "Saved " << filename << std::endl;

        pip.set_render_intent(RenderIntent::Perceptual);
        Image img_per = pg.get_image(pg.get_crop_box(), pip);
        filename = "ImageEmbedICCProfile-out_per" + std::to_string(page_num) + ".tif";
        img_per.save(filename, ImageType::TIFF, isp);
        std::cout << "Saved " << filename << std::endl;

        pip.set_render_intent(RenderIntent::RelColorimetric);
        Image img_rel = pg.get_image(pg.get_crop_box(), pip);
        filename = "ImageEmbedICCProfile-out_rel" + std::to_string(page_num) + ".tif";
        img_rel.save(filename, ImageType::TIFF, isp);
        std::cout << "Saved " << filename << std::endl;

    } catch (const std::exception& ex) {
        std::cerr << "Cannot write file: " << ex.what() << std::endl;
    }
}

int main(int argc, char* argv[])
{
    std::cout << "Image Embed ICC Profile sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input_path   = Library::get_resource_directory() + "Sample_Input/ducky.pdf";
    std::string profile_path = Library::get_resource_directory() + "Sample_Input/Probev1_ICCv2.icc";

    if (argc > 1) input_path   = argv[1];
    if (argc > 2) profile_path = argv[2];

    std::cout << "Input file: " << input_path
              << " will have profile " << profile_path << " applied." << std::endl;

    try {
        Document doc(input_path);

        for (int pg_num = 0; pg_num < doc.get_num_pages(); ++pg_num) {
            Page pg = doc.get_page(pg_num);
            export_image(pg, pg_num, profile_path);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
