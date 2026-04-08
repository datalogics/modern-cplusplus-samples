/*
 * This sample demonstrates working with Clip objects. A clipping path is used
 * to edit the borders of a graphics object.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/library.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/page.hpp>
#include <datalogics_interface/content.hpp>
#include <datalogics_interface/path.hpp>
#include <datalogics_interface/clip.hpp>
#include <datalogics_interface/graphic_state.hpp>
#include <datalogics_interface/color.hpp>
#include <datalogics_interface/geometry.hpp>

#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "Clips Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string s_output = "Clips-out.pdf";
    if (argc > 1)
        s_output = argv[1];

    std::cout << "Output file: " << s_output << std::endl;

    Document doc;
    Rect rect(0, 0, 612, 792);
    Page page = doc.create_page(Document::before_first_page, rect);
    std::cout << "Created new document and first page." << std::endl;

    // Create a path with a fill color
    Path path;
    GraphicState gs;
    gs.set_fill_color(Color(0.0, 0.0, 0.0));
    path.set_graphic_state(&gs);
    path.set_paint_op(PathPaintOp::Fill);

    // Add a rectangle to the path
    path.add_rect(Point(100, 500), 300, 200);
    std::cout << "Created new path and added rectangle to it." << std::endl;

    // Add a curve to the path
    path.add_curve_v(Point(400, 450), Point(350, 300));
    std::cout << "Added curve to the path." << std::endl;

    // Create a clipping path
    Path clip_path;
    clip_path.add_rect(Point(50, 300), 300, 250);
    std::cout << "Created clipping path and added rectangle to it." << std::endl;

    // Create a Clip, add the clip path to it, and assign it to the fill path
    Clip clip;
    clip.add_element(clip_path);
    path.set_clip(clip);
    std::cout << "Created new clip, assigned clipping path to it, and added new clip to original path." << std::endl;

    // Add the path (with clip) to the page content
    auto content = page.get_content();
    content.add_element(path);
    std::cout << "Added path to page in document." << std::endl;

    page.update_content();
    doc.save(SaveFlags::Full, s_output);

    std::cout << "Created " << s_output << std::endl;
    return 0;
}
