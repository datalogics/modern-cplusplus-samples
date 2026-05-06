/*
 * This sample demonstrates creating a file containing a DeviceN color space.
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

    std::string s_output = "DeviceN-out.pdf";
    if (argc > 1)
        s_output = argv[1];

    std::cout << "Writing to output " << s_output << std::endl;

    Document doc;
    Page page = doc.create_page(Document::before_first_page, Rect(0, 0, 5 * 72, 4 * 72));
    auto content = page.get_content();

    Font font("CourierStd", FontCreateFlags::DoNotEmbed);

    NamedColorSpace alternate = NamedColorSpace::device_rgb();

    std::vector<double> domain = {0.0, 1.0, 0.0, 1.0};
    std::vector<double> range = {0.0, 1.0, 0.0, 1.0, 0.0, 1.0};
    std::string code = "{ 0 exch }";
    PostScriptCalculatorFunction tint_transform(domain, range, code);

    DeviceNColorSpace cs({"DLRed", "DLBlue"}, alternate, tint_transform);
    GraphicState gs;
    gs.set_fill_color(Color(cs, {0.75, 0.75}));

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
