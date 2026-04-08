/*
 * Demonstrates working with the Calibrated RGB Color Space, based on the CIE color space.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/library.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/page.hpp>
#include <datalogics_interface/content.hpp>
#include <datalogics_interface/text.hpp>
#include <datalogics_interface/text_run.hpp>
#include <datalogics_interface/text_state.hpp>
#include <datalogics_interface/font.hpp>
#include <datalogics_interface/graphic_state.hpp>
#include <datalogics_interface/color.hpp>
#include <datalogics_interface/cal_rgb_color_space.hpp>
#include <datalogics_interface/geometry.hpp>

#include <iostream>
#include <string>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    Library lib;

    std::string s_output = "CalRGB-out.pdf";
    if (argc > 1)
        s_output = argv[1];

    std::cout << "Writing to output " << s_output << std::endl;

    Document doc;
    Page page = doc.create_page(Document::before_first_page, Rect(0, 0, 5 * 72, 4 * 72));
    auto content = page.get_content();

    Font font("CourierStd", FontCreateFlags::DoNotEmbed);

    // CalRGB for CCIR XA/11-recommended D65 white point with 1.8 gammas
    // and Sony Trinitron phosphor chromaticities
    std::vector<double> white_point = {0.9505, 1.0000, 1.0890};
    std::vector<double> black_point = {0.0, 0.0, 0.0};
    std::vector<double> gamma = {1.8, 1.8, 1.8};
    std::vector<double> matrix = {0.4497, 0.2446, 0.0252,
                                   0.3163, 0.6720, 0.1412,
                                   0.1845, 0.0833, 0.9227};

    CalRGBColorSpace cs(white_point, black_point, gamma, matrix);
    GraphicState gs;
    gs.set_fill_color(Color(cs, {0.3, 0.7, 0.3}));

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
