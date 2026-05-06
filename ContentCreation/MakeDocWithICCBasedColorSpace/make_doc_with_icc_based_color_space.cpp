/*
 * This sample demonstrates creating a file containing an ICC-based color space.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    Library lib;

    std::string s_input = Library::get_resource_directory() + "Sample_Input/sRGB_IEC61966-2-1_noBPC.icc";
    std::string s_output = "ICCBased-out.pdf";

    if (argc > 1) s_input = argv[1];
    if (argc > 2) s_output = argv[2];

    std::cout << "Writing to output " << s_output << std::endl;

    Document doc;
    Page page = doc.create_page(Document::before_first_page, Rect(0, 0, 5 * 72, 4 * 72));
    auto content = page.get_content();

    Font font("CourierStd", FontCreateFlags::DoNotEmbed);

    // Create a PDFStream from the ICC file
    std::ifstream icc_file(s_input, std::ios::binary);
    if (!icc_file) {
        std::cerr << "Could not open ICC file: " << s_input << std::endl;
        return 1;
    }
    PDFStream pdf_stream(icc_file, doc);

    ICCBasedColorSpace cs(pdf_stream, 3);
    GraphicState gs;
    gs.set_fill_color(Color(cs, {1.0, 0.0, 0.0}));

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
