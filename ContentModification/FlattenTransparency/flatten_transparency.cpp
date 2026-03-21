/*
 * This sample shows how to flatten transparencies in a PDF document.
 *
 * PDF files can have objects that are partially or fully transparent, and thus
 * can blend in various ways with objects behind them. The process to flatten a set
 * of transparencies merges them into a single image on the page.
 *
 * Copyright (c) 2007-2023, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <datalogics_interface/library.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/page.hpp>

#if PLATFORM_HAS_PDFLATTENER_PLUGIN
#include <datalogics_interface/flatten_transparency_params.hpp>
#endif

#include <iostream>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "FlattenTransparency sample:" << std::endl;

    Library lib;

    std::string sInput1 = Library::get_resource_directory() + "Sample_Input/trans_1page.pdf";
    std::string sOutput1 = "FlattenTransparency-out1.pdf";
    std::string sInput2 = Library::get_resource_directory() + "Sample_Input/trans_multipage.pdf";
    std::string sOutput2 = "FlattenTransparency-out2.pdf";

    if (argc > 1) sInput1 = argv[1];
    if (argc > 2) sInput2 = argv[2];
    if (argc > 3) sOutput1 = argv[3];
    if (argc > 4) sOutput2 = argv[4];

#if PLATFORM_HAS_PDFLATTENER_PLUGIN
    // Open a document with a single page.
    Document doc1(sInput1);

    // Verify that the page has transparency.
    Page pg1 = doc1.get_page(0);
    bool isTransparent = pg1.has_transparency(true);

    if (isTransparent) {
        // Dispose of the page reference before flattening
        { Page tmp = std::move(pg1); }

        doc1.flatten_transparency();
        std::cout << "Flattened single page document " << sInput1 << " as " << sOutput1 << "." << std::endl;
        doc1.save(SaveFlags::Full, sOutput1);
    }

    // Open a document with multiple pages.
    Document doc2(sInput2);

    isTransparent = false;
    int totalPages = doc2.get_num_pages();
    int pageCounter = 0;

    while (!isTransparent && pageCounter <= totalPages) {
        Page pg = doc2.get_page(pageCounter);
        if (pg.has_transparency(true)) {
            isTransparent = true;
            { Page tmp = std::move(pg); }
            break;
        }
        pageCounter++;
    }

    if (isTransparent) {
        FlattenTransparencyParams ftParams;
        ftParams.set_quality(50);

        doc2.flatten_transparency(ftParams, pageCounter, Document::last_page);
        std::cout << "Flattened a multi-page document " << sInput2 << " as " << sOutput2 << "." << std::endl;
        doc2.save(SaveFlags::Full, sOutput2);
    }
#else
    std::cout << "FlattenTransparency is not available on this platform." << std::endl;
#endif

    return 0;
}
