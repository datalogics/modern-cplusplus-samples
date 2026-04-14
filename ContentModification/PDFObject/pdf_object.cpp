/*
 * This sample demonstrates working with data objects in a PDF document. It examines the
 * Objects and displays information about them. The sample extracts the dictionary for an
 * object called URIAction and updates it using PDFObjects.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 *
 * Input file properties: First page must have an annotation with a URI link.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "PDFObject Sample:" << std::endl;

    Library lib;

    std::string sInput = Library::get_resource_directory() + "Sample_Input/sample_links.pdf";
    std::string sOutput = "PDFObject-out.pdf";

    if (argc > 1) sInput = argv[1];
    if (argc > 2) sOutput = argv[2];

    std::cout << "Input file: " << sInput << ". Writing to output " << sOutput << std::endl;

    Document doc(sInput);
    Page page = doc.get_page(0);

    auto annotPtr = page.get_annotation(1);
    auto* linkAnnot = annotPtr->try_as<LinkAnnotation>();
    auto actionPtr = linkAnnot->get_action();
    auto* uri = actionPtr->try_as<URIAction>();

    // Print some info about the URI action, before we modify it
    std::cout << "Initial URL: " << uri->get_uri() << std::endl;
    std::cout << "Is Map property: " << (uri->get_is_map() ? "true" : "false") << std::endl;

    // Modify the URIAction
    //
    // A URI action is a dictionary containing:
    //    Key: S     Contents: a name object with the value "URI" (required)
    //    Key: URI   Contents: a string object for the uniform resource locator (required)
    //    Key: IsMap Contents: a boolean for whether the link is part of a map (optional)
    //    (see section 8.5.3, "Action Types", of the PDF Reference)
    //
    // We will change the URI entry and delete the IsMap entry for this dictionary

    auto uri_dict = uri->get_pdf_dict(); // Extract the dictionary

    // Create a new string object
    PDFString uri_string("http://www.google.com", doc, false, false);

    uri_dict->put("URI", uri_string);    // Change the URI (replaces the old one)
    uri_dict->remove("IsMap");           // Remove the IsMap entry

    // Check that we deleted the IsMap entry
    std::cout << "Does this dictionary have an IsMap entry? "
              << (uri_dict->contains("IsMap") ? "true" : "false") << std::endl;

    doc.save(SaveFlags::Full, sOutput);
    doc.close();

    // Check the modified contents of the link
    Document doc2(sOutput);
    Page page2 = doc2.get_page(0);
    auto annotPtr2 = page2.get_annotation(1);
    auto* linkAnnot2 = annotPtr2->try_as<LinkAnnotation>();
    auto actionPtr2 = linkAnnot2->get_action();
    auto* uri2 = actionPtr2->try_as<URIAction>();

    std::cout << "Modified URL: " << uri2->get_uri() << std::endl;
    std::cout << "Is Map property (if not present, defaults to false): "
              << (uri2->get_is_map() ? "true" : "false") << std::endl;

    return 0;
}
