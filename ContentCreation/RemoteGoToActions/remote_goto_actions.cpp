/*
 * RemoteGoToActions generates a PDF file with an annotation in the form of a rectangle.
 * Click on the rectangle in this PDF file and a separate PDF file opens.
 * The RemoteGoToAction includes a RemoteDestination describing the view in the target file.
 *
 * Copyright (c) 2007-2024, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/library.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/page.hpp>
#include <datalogics_interface/link_annotation.hpp>
#include <datalogics_interface/file_specification.hpp>
#include <datalogics_interface/remote_destination.hpp>
#include <datalogics_interface/remote_goto_action.hpp>
#include <datalogics_interface/geometry.hpp>

#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    Library lib;

    std::string s_file_spec = Library::get_resource_directory() + "Sample_Input/ducky.pdf";
    std::string s_output = "RemoteGoToActions-out.pdf";

    if (argc > 1) s_file_spec = argv[1];
    if (argc > 2) s_output = argv[2];

    std::cout << "Writing to output " << s_output
              << ". Using " << s_file_spec << " as file specification" << std::endl;

    Document doc;
    Rect page_rect(0, 0, 612, 792);
    Page docpage = doc.create_page(Document::before_first_page, page_rect);
    std::cout << "Created page." << std::endl;

    LinkAnnotation new_link(docpage, Rect(153, 198, 306, 396));
    std::cout << "Created new Link Annotation" << std::endl;

    // FileSpecification specifies which file should be opened when the link is clicked.
    FileSpecification file_spec(doc, s_file_spec);
    std::cout << "Path to remote document: " << file_spec.get_path() << std::endl;

    // RemoteDestination specifies the view in the target file (page, fit type, rect, zoom).
    RemoteDestination remote_dest(doc, 0, "XYZ", Rect(0, 0, 4 * 72, 4 * 72), 1.5);
    std::cout << "When the Link is clicked the remote document will open to:" << std::endl;
    std::cout << "Page Number: " << remote_dest.get_page_number() << std::endl;
    std::cout << "zoom level: " << remote_dest.get_zoom() << std::endl;
    std::cout << "fit type: " << remote_dest.get_fit_type() << std::endl;
    std::cout << "rectangle: " << remote_dest.get_dest_rect().to_string() << std::endl;

    // Create the RemoteGoToAction from the file spec and the remote destination
    RemoteGoToAction remote_action(file_spec, remote_dest);

    // Assign the RemoteGoToAction to the LinkAnnotation
    new_link.set_action(remote_action);

    doc.save(SaveFlags::Full, s_output);

    std::cout << "Created " << s_output << std::endl;
    return 0;
}
