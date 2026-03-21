/*
 * This program creates a PDF file with an embedded hyperlink, which takes the viewer to the
 * second page of the document.
 *
 * Copyright (c) 2007-2024, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "LinkAnnotation Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/sample.pdf";
    std::string output_path = "LinkAnnotation-out.pdf";

    if (argc > 1) input_path = argv[1];
    if (argc > 2) output_path = argv[2];

    std::cout << "Input file: " << input_path << ". Writing to output " << output_path << std::endl;

    try {
        Document doc(input_path);

        Page docpage = doc.get_page(0);
        Rect crop = docpage.get_crop_box();

        // Create a link annotation near the top of the page
        Rect link_rect{100, crop.top() - 50, 200, crop.top() - 25};
        LinkAnnotation new_link(docpage, link_rect);

        // Generate and set normal appearance
        auto form = new_link.generate_appearance();
        new_link.set_normal_appearance(*form);

        std::cout << "Current Link Annotation version = " << new_link.get_annotation_feature_level() << std::endl;
        new_link.set_annotation_feature_level(1.0);
        std::cout << "New Link Annotation version = " << new_link.get_annotation_feature_level() << std::endl;

        // Create a view destination on page 0
        Page first_page = doc.get_page(0);
        ViewDestination dest(doc, 0, "XYZ", first_page.get_media_box(), 1.5);

        dest.set_dest_rect(Rect{0.0, 0.0, 200.0, 200.0});
        std::cout << "The new destination rectangle: " << dest.get_dest_rect().to_string() << std::endl;

        dest.set_fit_type("FitV");
        std::cout << "The new fit type: " << dest.get_fit_type() << std::endl;

        dest.set_zoom(2.5);
        std::cout << "The new zoom level: " << dest.get_zoom() << std::endl;

        dest.set_page_number(1);
        std::cout << "The new page number: " << dest.get_page_number() << std::endl;

        // Create a GoToAction and set it on the link
        GoToAction action(dest);
        new_link.set_action(action);

        new_link.set_highlight(HighlightStyle::Invert);

        if (new_link.get_highlight() == HighlightStyle::Invert)
            std::cout << "Invert highlighting." << std::endl;

        doc.save(SaveFlags::Full, output_path);
        std::cout << "Saved " << output_path << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
