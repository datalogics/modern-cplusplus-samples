/*
 * Demonstrates working with the Indexed Color Space. Indexed Color Spaces are used
 * to reduce the amount of system memory used for processing images when a limited
 * number of colors are needed.
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

    std::string s_output = "Indexed-out.pdf";
    if (argc > 1)
        s_output = argv[1];

    std::cout << "Writing to output " << s_output << std::endl;

    Document doc;
    Page page = doc.create_page(Document::before_first_page, Rect(0, 0, 5 * 72, 4 * 72));
    auto content = page.get_content();

    Font font("CourierStd", FontCreateFlags::DoNotEmbed);

    NamedColorSpace base_cs = NamedColorSpace::device_rgb();

    // Build lookup table: all combinations of 0/255 for R, G, B
    std::vector<long> lookup;
    int lowhi[] = {0, 255};
    for (int r : lowhi) {
        for (int g : lowhi) {
            for (int b : lowhi) {
                lookup.push_back(r);
                lookup.push_back(g);
                lookup.push_back(b);
            }
        }
    }

    IndexedColorSpace cs(base_cs, 7, lookup);
    GraphicState gs;
    gs.set_fill_color(Color(cs, {4.0}));

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
