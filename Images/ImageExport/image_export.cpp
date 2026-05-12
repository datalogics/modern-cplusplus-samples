/*
 * This sample program reads the pages of the PDF file that you provide and extracts images
 * that it finds on each page and saves those images to external graphics files.
 *
 * The program examines the content stream for image elements and exports those image objects.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace datalogics_interface;

static int next_index = 0;
static ImageCollection image_collection;

static void export_element_images(Content& content, ImageType export_type)
{
    for (int i = 0; i < content.get_num_elements(); ++i) {
        std::unique_ptr<Element> elem = content.get_element(i);
        if (!elem) continue;

        if (auto* img_ptr = elem->try_as<Image>()) {
            // Weed out impossible or nonsensical combinations.
            // (get_color_space is deprecated/unimplemented; skip CMYK check for now)

            try {
                if (export_type == ImageType::TIFF) {
                    image_collection.append(std::move(*img_ptr->clone()));
                    ImageSaveParams isp;
                    isp.set_compression(CompressionCode::LZW);
                    img_ptr->save("ImageExport-out" + std::to_string(next_index) + ".tif",
                                  export_type, isp);
                } else if (export_type == ImageType::JPEG) {
                    ImageSaveParams isp;
                    isp.set_jpeg_quality(80);
                    img_ptr->save("ImageExport-out" + std::to_string(next_index) + ".jpg",
                                  export_type, isp);
                } else if (export_type == ImageType::PNG) {
                    img_ptr->save("ImageExport-out" + std::to_string(next_index) + ".png",
                                  export_type);
                } else if (export_type == ImageType::GIF) {
                    img_ptr->save("ImageExport-out" + std::to_string(next_index) + ".gif",
                                  export_type);
                } else if (export_type == ImageType::BMP) {
                    img_ptr->save("ImageExport-out" + std::to_string(next_index) + ".bmp",
                                  export_type);
                }
            } catch (const std::exception& ex) {
                std::cerr << "Cannot write file: " << ex.what() << std::endl;
            }

            ++next_index;

        } else if (auto* cont = elem->try_as<Container>()) {
            // Recurse into sub-content.
            std::cout << "Recursing through a Container" << std::endl;
            auto sub = cont->get_content();
            if (sub) export_element_images(*sub, export_type);
        } else if (auto* grp = elem->try_as<Group>()) {
            std::cout << "Recursing through a Group" << std::endl;
            auto sub = grp->get_content();
            if (sub) export_element_images(*sub, export_type);
        } else if (auto* frm = elem->try_as<Form>()) {
            std::cout << "Recursing through a Form" << std::endl;
            auto sub = frm->get_content();
            if (sub) export_element_images(*sub, export_type);
        }
    }
}

static void export_doc_images_type(Document& doc, ImageType export_type)
{
    for (int pg_num = 0; pg_num < doc.get_num_pages(); ++pg_num) {
        Page pg = doc.get_page(pg_num);
        Content content = pg.get_content();
        export_element_images(content, export_type);
    }

    if (image_collection.get_count() > 0 && export_type == ImageType::TIFF) {
        try {
            image_collection.save("ImageExport-page-out.tif", ImageType::TIFF);
        } catch (const std::exception& ex) {
            std::cerr << "Cannot write file: " << ex.what() << std::endl;
        }
    }
}

int main(int argc, char* argv[])
{
    std::cout << "Image Export sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/ducky.pdf";

    if (argc > 1) input_path = argv[1];

    std::cout << "Input file: " << input_path << std::endl;

    try {
        Document doc(input_path);

        export_doc_images_type(doc, ImageType::TIFF);
        export_doc_images_type(doc, ImageType::JPEG);
        export_doc_images_type(doc, ImageType::PNG);
        export_doc_images_type(doc, ImageType::GIF);
        export_doc_images_type(doc, ImageType::BMP);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
