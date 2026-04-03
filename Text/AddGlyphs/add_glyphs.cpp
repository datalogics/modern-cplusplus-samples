/*
 * Copyright (c) Datalogics, Inc. All rights reserved.
 *
 * Use this program to create a new PDF file and add glyphs to the page,
 * managing them by individual Glyph ID codes.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "AddGlyphs Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string output_path = "AddGlyphs-out.pdf";
    if (argc > 1) output_path = argv[1];

    std::cout << "Output file: " << output_path << std::endl;

    Document doc;

    Rect page_rect(0, 0, 612, 792);
    Page docpage = doc.create_page(Document::before_first_page, page_rect);
    std::cout << "Created page." << std::endl;

    Font font("Times-Roman");

    // Glyph IDs for "HELLO" in the Times-Roman encoding
    std::vector<std::uint8_t> glyph_ids = {0x2b, 0x28, 0x2f, 0x2f, 0x32};

    // Unicode text corresponding to each glyph
    std::vector<std::uint8_t> unicode = {'H', 'E', 'L', 'L', 'O'};

    TextState state;
    state.set_font_size(50);

    GraphicState gs;

    Rect crop = docpage.get_crop_box();
    Matrix m;
    m = m.translate(crop.bottom(), crop.right());

    Text text;
    text.add_glyphs(glyph_ids, unicode, font, gs, state, m, TextFlags::TextRun);

    Content content = docpage.get_content();
    content.add_element(text);
    docpage.update_content();

    doc.embed_fonts();
    doc.save(SaveFlags::Full, output_path);

    return 0;
}
