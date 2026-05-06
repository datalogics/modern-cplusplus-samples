/*
 * The AddElements sample program creates a new PDF file with three pages and several
 * graphics and text elements. The first page features a pentagram drawing; the second
 * shows how to create colored text, text that is vertical or at an angle, and a shape
 * with color fill. The third page features a rectangle and a curved design.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "AddElements Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string s_output = "AddElements-out.pdf";
    if (argc > 1)
        s_output = argv[1];

    std::cout << "Output file: " << s_output << std::endl;

    Document doc;
    Rect page_rect(0, 0, 612, 792);
    Page docpage = doc.create_page(Document::before_first_page, page_rect);

    // Draw a five-pointed star
    Path starpath;
    GraphicState star_gs;
    star_gs.set_width(2.0);
    star_gs.set_dash_pattern({3.0, 5.0, 6.0});
    star_gs.set_stroke_color(Color(0, 1.0, 0));  // Green
    starpath.set_graphic_state(star_gs);
    starpath.set_paint_op(PathPaintOp::Stroke);

    double center_x = 306.0;
    double center_y = 396.0;
    double radius = 72 * 4.0;
    static const double pi = std::acos(-1.0);
    double radians72 = 72.0 * pi / 180.0;
    double radians36 = 36.0 * pi / 180.0;

    Point pt0(center_x, center_y + radius);
    Point pt1(center_x + radius * std::sin(radians72), center_y + radius * std::cos(radians72));
    Point pt2(center_x + radius * std::sin(radians36), center_y - radius * std::cos(radians36));
    Point pt3(center_x - radius * std::sin(radians36), center_y - radius * std::cos(radians36));
    Point pt4(center_x - radius * std::sin(radians72), center_y + radius * std::cos(radians72));

    starpath.move_to(pt0);
    starpath.add_line(pt2);
    starpath.add_line(pt4);
    starpath.add_line(pt1);
    starpath.add_line(pt3);
    starpath.add_line(pt0);
    starpath.close_path();

    {
        auto content = docpage.get_content();
        content.add_element(starpath);
    }

    // Draw a pentagon around the star
    Path pentpath;
    GraphicState pent_gs;
    pent_gs.set_width(2.0);
    pent_gs.set_dash_pattern({0.0, 3.0});
    pent_gs.set_stroke_color(Color(1.0, 0.0, 1.0));  // Purple
    pentpath.set_graphic_state(pent_gs);
    pentpath.set_paint_op(PathPaintOp::Stroke);
    pentpath.move_to(pt0);
    pentpath.add_line(pt1);
    pentpath.add_line(pt2);
    pentpath.add_line(pt3);
    pentpath.add_line(pt4);
    pentpath.add_line(pt0);
    pentpath.close_path();

    {
        auto content = docpage.get_content();
        content.add_element(pentpath);
    }

    // Add inner star lines
    Path newstar;
    GraphicState inner_gs;
    inner_gs.set_width(1.0);
    inner_gs.set_stroke_color(Color(0, 0, 1.0));  // Blue
    newstar.set_graphic_state(inner_gs);
    newstar.set_paint_op(PathPaintOp::EoFill);

    Point center_pt(center_x, center_y);
    newstar.move_to(center_pt); newstar.add_line(pt0);
    newstar.move_to(center_pt); newstar.add_line(pt1);
    newstar.move_to(center_pt); newstar.add_line(pt2);
    newstar.move_to(center_pt); newstar.add_line(pt3);
    newstar.move_to(center_pt); newstar.add_line(pt4);
    newstar.close_path();

    {
        auto content = docpage.get_content();
        content.add_element(newstar);
    }
    docpage.update_content();

    // Second page: diamond with text inside
    docpage = doc.create_page(0, page_rect);

    Path diamond;
    GraphicState diamond_gs;
    diamond_gs.set_fill_color(Color(1.0, 1.0, 0));
    diamond_gs.set_stroke_color(Color(153.0 / 255.0, 0, 0));
    diamond_gs.set_width(1.0);
    diamond_gs.set_line_join(LineJoin::Bevel);
    diamond.set_graphic_state(diamond_gs);
    diamond.set_paint_op(PathPaintOp::EoFill | PathPaintOp::Stroke);
    diamond.move_to(Point(306, 198));
    diamond.add_line(Point(459, 396));
    diamond.add_line(Point(306, 594));
    diamond.add_line(Point(153, 396));
    diamond.add_line(Point(306, 198));
    diamond.close_path();

    {
        auto content = docpage.get_content();
        content.add_element(diamond);
    }

    // Add text runs
    Font f("CourierStd", FontCreateFlags::DoNotEmbed);
    Text t;

    GraphicState gs1;
    gs1.set_fill_color(Color(0, 0, 1.0));
    TextState ts;
    Matrix m1(24.0, 0, 0, 24.0, 180, 414);
    TextRun tr1("Horizontal Blue Text", f, gs1, ts, m1);
    t.add_run(tr1);

    GraphicState gs2;
    gs2.set_fill_color(Color(1.0, 0, 0));
    gs2.set_stroke_color(Color(0.0, 0.5, 0.5));
    TextState ts2;
    ts2.set_render_mode(TextRenderMode::FillThenStroke);
    Matrix m2 = Matrix(1, 0, 0, 1, 315, 216).scale(24.0, 24.0).rotate(90);
    TextRun tr2("Vertical Red Text", f, gs2, ts2, m2);
    t.add_run(tr2);

    GraphicState gs3;
    gs3.set_fill_color(Color(0, 1.0, 0));
    TextState ts3;
    ts3.set_render_mode(TextRenderMode::Stroke);
    Matrix m3 = Matrix(1, 0, 0, 1, 297, 576).scale(24.0, 24.0).rotate(-52);
    TextRun tr3("Angled Green Text", f, gs3, ts3, m3);
    t.add_run(tr3);

    {
        auto content = docpage.get_content();
        content.add_element(t);
    }
    docpage.update_content();

    // Third page: all segment types
    docpage = doc.create_page(1, page_rect);

    Path path3;
    GraphicState gs_p3;
    gs_p3.set_width(2.0);
    path3.set_graphic_state(gs_p3);
    path3.set_paint_op(PathPaintOp::Stroke);

    // Use segments
    path3.move_to(Point(72.0, 73.0));
    path3.add_line(Point(76.0, 144.0));
    path3.add_curve(Point(120.0, 144.0), Point(121.0, 96.0), Point(97.0, 98.0));
    path3.add_curve_v(Point(80.0, 81.0), Point(128.0, 21.0));
    path3.add_curve_y(Point(200.0, 201.0), Point(22.0, 160.0));
    path3.close_path();
    path3.add_rect(Point(256.0, 257.0), 123.0, 67.0);

    {
        auto content = docpage.get_content();
        content.add_element(path3);
    }
    docpage.update_content();

    doc.embed_fonts();
    doc.save(SaveFlags::Full, s_output);

    std::cout << "Created " << s_output << std::endl;
    return 0;
}
