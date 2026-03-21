/*
 * The Watermark sample program shows how to create a watermark and copy it to a new PDF file.
 * You could use this code to create a message to apply to PDF files you select, like
 * "Confidential" or "Draft Copy."
 *
 * Copyright (c) 2007-2023, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "Watermark Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string sInput     = Library::get_resource_directory() + "Sample_Input/sample.pdf";
    std::string sWatermark = Library::get_resource_directory() + "Sample_Input/ducky.pdf";
    std::string sOutput    = "Watermark-out.pdf";

    if (argc > 1) sInput     = argv[1];
    if (argc > 2) sWatermark = argv[2];
    if (argc > 3) sOutput    = argv[3];

    std::cout << "Adding watermark from " << sWatermark << " to " << sInput
              << " and saving to " << sOutput << std::endl;

    Document doc(sInput);
    Document watermarkDoc(sWatermark);

    WatermarkParams watermarkParams;
    watermarkParams.set_opacity(0.8f);
    watermarkParams.set_rotation(45.3f);
    watermarkParams.set_scale(0.5f);
    watermarkParams.set_page_spec(PageSpec::EvenPagesOnly);

    Page wmPage = watermarkDoc.get_page(0);
    doc.watermark(wmPage, watermarkParams);

    watermarkParams.set_page_spec(PageSpec::OddPagesOnly);

    WatermarkTextParams watermarkTextParams;
    Color color(109.0f / 255.0f, 15.0f / 255.0f, 161.0f / 255.0f);
    watermarkTextParams.set_color(color);
    watermarkTextParams.set_text("Multiline\nWatermark");

    Font f("Courier", FontCreateFlags::Embedded | FontCreateFlags::Subset);
    watermarkTextParams.set_font(f);
    watermarkTextParams.set_text_alignment(HorizontalAlignment::Center);

    doc.watermark(watermarkTextParams, watermarkParams);

    doc.embed_fonts();
    doc.save(SaveFlags::Full | SaveFlags::Linearized, sOutput);

    return 0;
}
