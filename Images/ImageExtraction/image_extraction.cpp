/*
 * This sample program searches through the PDF file that you select and identifies
 * raster drawings, diagrams and photographs among the text. Then, it extracts these
 * images from the PDF file and saves them as PNG files.
 *
 * Vector images will not be exported.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

static int next_index = 0;

static void extract_images(Content& content)
{
    for (int i = 0; i < content.get_num_elements(); ++i) {
        std::unique_ptr<Element> elem = content.get_element(i);
        if (!elem) continue;

        if (auto* img_ptr = elem->try_as<Image>()) {
            std::cout << "Saving an image" << std::endl;

            // Save the image at original resolution
            img_ptr->save("ImageExtraction-extract-out" + std::to_string(next_index) + ".png",
                          ImageType::PNG);

            // Save a resampled version at 500 DPI
            Image resampled = img_ptr->change_resolution(500);
            resampled.save("ImageExtraction-extract-Resolution-500-out" + std::to_string(next_index) + ".png",
                           ImageType::PNG);

            ++next_index;

        } else if (auto* cont = elem->try_as<Container>()) {
            auto sub = cont->get_content();
            if (sub) extract_images(*sub);
        } else if (auto* grp = elem->try_as<Group>()) {
            auto sub = grp->get_content();
            if (sub) extract_images(*sub);
        } else if (auto* frm = elem->try_as<Form>()) {
            auto sub = frm->get_content();
            if (sub) extract_images(*sub);
        }
    }
}

int main(int argc, char* argv[])
{
    std::cout << "ImageExtraction Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/ducky.pdf";

    if (argc > 1) input_path = argv[1];

    std::cout << "Input file: " << input_path << std::endl;

    try {
        Document doc(input_path);
        Page pg = doc.get_page(0);
        Content content = pg.get_content();
        extract_images(content);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
