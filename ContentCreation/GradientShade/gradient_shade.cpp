/*
 * This sample demonstrates changing the shading of an image on a PDF document page.
 * The image gradually changes from black on the left side to red on the right side.
 *
 * Copyright (c) 2007-2024, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/library.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/page.hpp>
#include <datalogics_interface/content.hpp>
#include <datalogics_interface/path.hpp>
#include <datalogics_interface/graphic_state.hpp>
#include <datalogics_interface/color.hpp>
#include <datalogics_interface/color_space.hpp>
#include <datalogics_interface/named_color_space.hpp>
#include <datalogics_interface/exponential_function.hpp>
#include <datalogics_interface/axial_shading_pattern.hpp>
#include <datalogics_interface/geometry.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "GradientShade Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string s_output = "GradientShade-out.pdf";
    if (argc > 1)
        s_output = argv[1];

    std::cout << "Will write new file: " << s_output << std::endl;

    Document doc;
    Rect page_rect(0, 0, 792, 612);
    Page docpage = doc.create_page(Document::before_first_page, page_rect);

    std::vector<double> domain = {0.0, 1.0};
    std::vector<double> c0 = {0.0, 0.0, 0.0};
    std::vector<double> c1 = {1.0, 0.0, 0.0};
    auto f = std::make_shared<ExponentialFunction>(domain, 3, c0, c1, 1.0);

    std::vector<Point> coords = {Point(72, 72), Point(4 * 72, 72)};
    std::vector<std::shared_ptr<Function>> function_list = {f};

    NamedColorSpace rgb_cs = NamedColorSpace::device_rgb();
    AxialShadingPattern asp(rgb_cs, coords, function_list);

    Path path;
    GraphicState gs;
    gs.set_fill_color(Color(asp));
    path.set_graphic_state(&gs);
    path.set_paint_op(PathPaintOp::Stroke | PathPaintOp::Fill);

    path.move_to(Point(36, 36));
    path.add_line(Point(36, 8 * 72 - 36));
    path.add_line(Point(11 * 72 - 36, 8 * 72 - 36));
    path.add_line(Point(11 * 72 - 36, 36));
    path.close_path();

    auto content = docpage.get_content();
    content.add_element(path);
    docpage.update_content();

    doc.save(SaveFlags::Full, s_output);

    std::cout << "Created " << s_output << std::endl;
    return 0;
}
