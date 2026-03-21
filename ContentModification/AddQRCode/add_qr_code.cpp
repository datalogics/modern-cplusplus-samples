/*
 * This sample shows how to add a QR barcode to a PDF page.
 *
 * Copyright (c) 2024-2025, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <datalogics_interface/library.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/page.hpp>
#include <iostream>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "AddQRCode Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string sInput = Library::get_resource_directory() + "Sample_Input/sample_links.pdf";
    const std::string sOutput = "AddQRCode-out.pdf";

    if (argc > 1)
        sInput = argv[1];

    Document doc(sInput);

    Page page = doc.get_page(0);

    Rect cropBox = page.get_crop_box();
    page.add_qr_barcode("Datalogics", 72.0, cropBox.top() - 1.5 * 72.0, 72.0, 72.0);

    doc.save(SaveFlags::Full, sOutput);
    std::cout << "Saved to " << sOutput << std::endl;

    return 0;
}
