/*
 * Demonstrates working with the Lab color space. The Lab color space is based on the
 * CIE XYZ color space, but it includes a dimension L for lightness, along with a and
 * b coordinates to define the color.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <string>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    Library lib;

    std::string s_output = "Lab-out.pdf";
    if (argc > 1)
        s_output = argv[1];

    std::cout << "Writing to output " << s_output << std::endl;

    Document doc;
    Page page = doc.create_page(Document::before_first_page, Rect(0, 0, 5 * 72, 4 * 72));
    auto content = page.get_content();

    Font font("CourierStd", FontCreateFlags::DoNotEmbed);

    // CIE 1976 L*a*b* space with CCIR XA/11-recommended D65 white point.
    // a* and b* components lie in the range -128 to +127.
    std::vector<double> white_point = {0.9505, 1.0000, 1.0890};
    std::vector<double> black_point = {0.0, 0.0, 0.0};
    std::vector<double> range = {-128.0, 127.0, -128.0, 127.0};

    LabColorSpace cs(white_point, black_point, range);
    GraphicState gs;
    gs.set_fill_color(Color(cs, {55.0, -54.0, 55.0}));

    Matrix text_matrix(24, 0, 0, 24, 1 * 72, 2 * 72);
    TextState ts;
    TextRun text_run("Hello World!", font, gs, ts, text_matrix);
    Text text;
    text.add_run(text_run);
    content.add_element(text);
    page.update_content();

    doc.embed_fonts();
    doc.save(SaveFlags::Full, s_output);

    std::cout << "Created " << s_output << std::endl;
    return 0;
}
