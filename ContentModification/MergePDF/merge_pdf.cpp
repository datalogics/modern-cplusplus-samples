/*
 * This sample demonstrates merging one PDF document into another. The program
 * inserts the content of the second PDF file into the first PDF file and saves
 * the result in a third PDF file.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <datalogics_interface/library.hpp>
#include <datalogics_interface/document.hpp>
#include <iostream>
#include <stdexcept>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "MergePDF Sample:" << std::endl;

    Library lib;

    std::string sInput1 = Library::get_resource_directory() + "Sample_Input/merge_pdf1.pdf";
    std::string sInput2 = Library::get_resource_directory() + "Sample_Input/merge_pdf2.pdf";
    std::string sOutput  = "MergePDF-out.pdf";

    if (argc > 1) sInput1 = argv[1];
    if (argc > 2) sInput2 = argv[2];
    if (argc > 3) sOutput  = argv[3];

    std::cout << "MergePDF: adding " << sInput1 << " and " << sInput2 << " and writing to " << sOutput << std::endl;

    Document doc1(sInput1);
    Document doc2(sInput2);

    try {
        doc1.insert_pages(
            Document::last_page,
            doc2,
            0,
            Document::all_pages,
            PageInsertFlags::Bookmarks |
            PageInsertFlags::Threads |
            // For best performance processing large documents, set these flags:
            PageInsertFlags::DoNotMergeFonts |
            PageInsertFlags::DoNotResolveInvalidStructureParentReferences |
            PageInsertFlags::DoNotRemovePageInheritance);
    } catch (const std::exception& ex) {
        std::string msg = ex.what();
        if (msg.find("An incorrect structure tree was found") == std::string::npos)
            throw;
        std::cout << msg << std::endl;
    }

    // For best performance processing large documents, set the following flags.
    doc1.save(SaveFlags::Full | SaveFlags::SaveLinearizedNoOptimizeFonts | SaveFlags::Compressed, sOutput);
    std::cout << "Saved to " << sOutput << std::endl;

    return 0;
}
