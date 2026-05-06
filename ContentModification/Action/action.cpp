/*
 * This sample creates a PDF document with a single page, featuring a rectangle.
 * An action is added to the rectangle in the form of a hyperlink; if the viewer
 * clicks on the rectangle, it opens a Datalogics web page.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>

using namespace datalogics_interface;

int main() {
    std::cout << "Action Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    const std::string sOutput = "Action-out.pdf";

    Document doc;

    Rect pageRect(0, 0, 100, 100);
    Page docpage = doc.create_page(Document::before_first_page, pageRect);
    std::cout << "Created page." << std::endl;

    // Create first link with a URI action
    LinkAnnotation newLink(docpage, Rect(1.0, 2.0, 3.0, 4.0));
    std::cout << newLink.to_string() << std::endl;

    doc.set_base_uri("http://www.datalogics.com");
    URIAction uri("/adobe-pdf-library/", false);
    std::cout << "Action data: " << uri.to_string() << std::endl;

    newLink.set_action(uri);

    // Create a second link with a GoTo action
    LinkAnnotation secondLink(docpage, Rect(5.0, 6.0, 7.0, 8.0));

    Rect r(5, 5, 100, 100);
    ViewDestination vd(doc, 0, "FitR", r, 1.0);
    GoToAction gta(vd);
    std::cout << "Action data: " << gta.to_string() << std::endl;

    secondLink.set_action(gta);

    // Read some URI properties
    std::cout << "Extracted URI: " << uri.get_uri() << std::endl;

    if (uri.get_is_map())
        std::cout << "Send mouse coordinates" << std::endl;
    else
        std::cout << "Don't send mouse coordinates" << std::endl;

    // Change the URI properties
    doc.set_base_uri("http://www.datalogics.com");
    uri.set_uri("/products/pdf/pdflibrary/");
    uri.set_is_map(true);

    std::cout << "Complete changed URI: " << doc.get_base_uri() << uri.get_uri() << std::endl;

    if (uri.get_is_map())
        std::cout << "Send mouse coordinates" << std::endl;
    else
        std::cout << "Don't send mouse coordinates" << std::endl;

    auto dest = gta.get_destination();
    std::cout << "Fit type of destination: " << dest->get_fit_type() << std::endl;
    Rect destRect = dest->get_dest_rect();
    std::cout << "Rectangle of destination: " << destRect.left() << " " << destRect.bottom()
              << " " << destRect.right() << " " << destRect.top() << std::endl;
    std::cout << "Zoom of destination: " << dest->get_zoom() << std::endl;
    std::cout << "Page number of destination: " << dest->get_page_number() << std::endl;

    doc.save(SaveFlags::Full, sOutput);
    std::cout << "Saved to " << sOutput << std::endl;

    return 0;
}
