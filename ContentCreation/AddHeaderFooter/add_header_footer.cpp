/*
 * This sample demonstrates creating a new PDF document with a Header and Footer.
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
#include <datalogics_interface/geometry.hpp>

#include <cmath>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "AddHeaderFooter Sample:" << std::endl;

    double font_size = 12.0;
    double top_bottom_margin = 0.5;
    double page_width = 8.5 * 72;
    double page_height = 11 * 72;
    std::string header_text = "Title of Document";
    std::string footer_text = "Page 1";

    Library lib;

    std::string s_output = "AddHeaderFooter-out.pdf";
    if (argc > 1)
        s_output = argv[1];

    std::cout << "Output file: " << s_output << std::endl;

    Document doc;
    Rect page_rect(0, 0, page_width, page_height);
    Page new_page = doc.create_page(Document::before_first_page, page_rect);

    Font font("CourierStd", FontCreateFlags::DoNotEmbed);

    double text_height = (font.get_ascent() + std::abs(font.get_descent())) / 1000.0;

    GraphicState graphic_state;
    TextState text_state;

    double header_x = (page_width - font.measure_text_width(header_text, font_size)) / 2.0;
    double header_y = page_height - top_bottom_margin * 72.0 + text_height;
    Matrix header_matrix(font_size, 0, 0, font_size, header_x, header_y);

    double footer_x = (page_width - font.measure_text_width(footer_text, font_size)) / 2.0;
    double footer_y = top_bottom_margin * 72.0 - text_height;
    Matrix footer_matrix(font_size, 0, 0, font_size, footer_x, footer_y);

    TextRun header_run(header_text, font, graphic_state, text_state, header_matrix);
    TextRun footer_run(footer_text, font, graphic_state, text_state, footer_matrix);

    Text text;
    text.add_run(header_run);
    text.add_run(footer_run);

    auto content = new_page.get_content();
    content.add_element(text);
    new_page.update_content();

    doc.save(SaveFlags::Full, s_output);

    std::cout << "Created " << s_output << std::endl;
    return 0;
}
