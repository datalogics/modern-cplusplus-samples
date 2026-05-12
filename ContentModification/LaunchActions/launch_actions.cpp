/*
 * The sample creates a PDF document with a single blank page, featuring a rectangle.
 * An action is added to the rectangle in the form of a hyperlink; if the reader clicks
 * on the rectangle, a different PDF file opens, showing an image.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    Library lib;

    std::string sInput = Library::get_resource_directory() + "Sample_Input/ducky.pdf";
    std::string sOutput = "LaunchActions-out.pdf";

    if (argc > 1)
        sInput = argv[1];
    if (argc > 2)
        sOutput = argv[2];

    std::cout << "Input file: " << sInput << ". Writing to output " << sOutput << std::endl;

    Document doc;

    // Standard letter size page (8.5" x 11")
    Rect pageRect(0, 0, 612, 792);
    Page docpage = doc.create_page(Document::before_first_page, pageRect);
    std::cout << "Created page." << std::endl;

    LinkAnnotation newLink(docpage, Rect(153, 198, 306, 396));
    std::cout << "Created new Link Annotation" << std::endl;

    // FileSpecifications can take either a relative or absolute path.
    FileSpecification fileSpec(doc, sInput);
    std::cout << "Created a new FileSpecification with a path: " << fileSpec.get_path() << std::endl;

    LaunchAction launch(fileSpec);
    std::cout << "Created a new Launch Action" << std::endl;

    // Setting new_window to true causes the document to open in a new window.
    launch.set_new_window(true);

    newLink.set_action(launch);

    doc.save(SaveFlags::Full, sOutput);
    std::cout << "Saved to " << sOutput << std::endl;

    return 0;
}
