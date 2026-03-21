/*
 * This sample demonstrates how to find and resample images within a PDF document.
 * The images are then put back into the PDF document with a new resolution.
 *
 * Resampling involves resizing an image or images within a PDF document. Commonly this
 * process is used to reduce the resolution of an image or series of images, making the
 * PDF document smaller.
 *
 * Copyright (c) 2007-2023, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

static int num_replaced = 0;

static void resample_images(Content& content)
{
    int i = 0;
    while (i < content.get_num_elements()) {
        std::unique_ptr<Element> elem = content.get_element(i);
        if (!elem) { ++i; continue; }

        std::cout << i << " / " << content.get_num_elements()
                  << " = " << static_cast<int>(elem->get_element_type()) << std::endl;

        ElementType etype = elem->get_element_type();

        if (etype == ElementType::Image) {
            Image* img_ptr = dynamic_cast<Image*>(elem.get());
            if (img_ptr) {
                try {
                    Image new_img = img_ptr->change_resolution(400);
                    std::cout << "Replacing an image..." << std::endl;
                    // Add the new image at position i (inserts after i-1)
                    content.add_element(new_img, i - 1);
                    // Remove the old image which is now at i+1
                    content.remove_element(i + 1);
                    std::cout << "Replaced." << std::endl;
                    ++num_replaced;
                } catch (const std::exception& ex) {
                    std::cerr << ex.what() << std::endl;
                }
            }
        } else if (etype == ElementType::Container) {
            std::cout << "Recursing through a Container" << std::endl;
            auto* cont = dynamic_cast<Container*>(elem.get());
            if (cont) {
                auto sub = cont->get_content();
                if (sub) resample_images(*sub);
            }
        } else if (etype == ElementType::Group) {
            std::cout << "Recursing through a Group" << std::endl;
            auto* grp = dynamic_cast<Group*>(elem.get());
            if (grp) {
                auto sub = grp->get_content();
                if (sub) resample_images(*sub);
            }
        } else if (etype == ElementType::Form) {
            std::cout << "Recursing through a Form" << std::endl;
            auto* frm = dynamic_cast<Form*>(elem.get());
            if (frm) {
                auto sub = frm->get_content();
                if (sub) {
                    resample_images(*sub);
                    frm->set_content(sub.get());
                }
            }
        }

        ++i;
    }
}

int main(int argc, char* argv[])
{
    std::cout << "ImageResampling Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input_path  = Library::get_resource_directory() + "Sample_Input/ducky.pdf";
    std::string output_path = "ImageResampling-out.pdf";

    if (argc > 1) input_path  = argv[1];
    if (argc > 2) output_path = argv[2];

    std::cout << "Input file " << input_path
              << ". Writing to output file " << output_path << std::endl;

    try {
        Document doc(input_path);
        std::cout << "Opened a document." << std::endl;

        for (int pg_num = 0; pg_num < doc.get_num_pages(); ++pg_num) {
            num_replaced = 0;
            Page pg = doc.get_page(pg_num);
            Content content = pg.get_content();
            resample_images(content);
            if (num_replaced != 0) {
                pg.update_content();
            }
        }

        doc.save(SaveFlags::Full | SaveFlags::CollectGarbage, output_path);
        std::cout << "Saved " << output_path << std::endl;

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
