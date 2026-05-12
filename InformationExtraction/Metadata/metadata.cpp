/*
 * This sample shows how to view and edit metadata for a PDF document. The metadata values
 * appear on the Properties window in a PDF viewer (File > Properties > Additional Metadata).
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

// Extract the dc:title text from an XMP metadata string using basic string search.
static std::string get_title(const std::string& xmp)
{
    // Look for <dc:title> ... <rdf:li> ... </rdf:li>
    auto title_start = xmp.find("<dc:title");
    if (title_start == std::string::npos) return "";

    auto li_start = xmp.find("<rdf:li", title_start);
    if (li_start == std::string::npos) return "";

    auto content_start = xmp.find('>', li_start);
    if (content_start == std::string::npos) return "";
    ++content_start;

    auto li_end = xmp.find("</rdf:li>", content_start);
    if (li_end == std::string::npos) return "";

    return xmp.substr(content_start, li_end - content_start);
}

static void display_document_metadata(const std::string& input, const std::string& output)
{
    Document doc(input);

    std::string metadata = doc.get_xmp_metadata();

    std::cout << "Title: " << get_title(metadata) << std::endl;
    std::cout << "CreatorTool: "
              << doc.get_xmp_metadata_property("http://ns.adobe.com/xap/1.0/", "CreatorTool")
              << std::endl;
    std::cout << "format: "
              << doc.get_xmp_metadata_property("http://purl.org/dc/elements/1.1/", "format")
              << std::endl;

    int num_authors = doc.count_xmp_metadata_array_items("http://ns.adobe.com/xap/1.0/", "Authors");
    std::cout << "Number of authors: " << num_authors << std::endl;
    for (int i = 1; i <= num_authors; ++i) {
        std::cout << "Author: "
                  << doc.get_xmp_metadata_array_item("http://ns.adobe.com/xap/1.0/", "Authors", i)
                  << std::endl;
    }

    // Demonstrate setting a property
    doc.set_xmp_metadata_array_item("http://ns.adobe.com/xap/1.0/", "tetractys",
                                    "Authors", 2, "Metadata Tester");
    doc.save(SaveFlags::Full, output);
}

// Recursively search content for an Image element and print its XMP metadata.
static bool display_image_xmp(Content& content)
{
    for (int i = 0; i < content.get_num_elements(); ++i) {
        auto elem = content.get_element(i);
        if (!elem) continue;

        if (auto* img = elem->try_as<Image>()) {
            std::string xmp = img->get_xmp_metadata();
            if (!xmp.empty()) {
                std::cout << "Image XMP Metadata:" << std::endl;
                std::cout << xmp << std::endl;
            } else {
                std::cout << "Image found but has no XMP metadata." << std::endl;
            }
            return true;
        }

        // Recurse into containers, forms, and groups
        if (auto* container = elem->try_as<Container>()) {
            if (auto sub = container->get_content()) {
                if (display_image_xmp(*sub)) return true;
            }
        } else if (auto* form = elem->try_as<Form>()) {
            if (auto sub = form->get_content()) {
                if (display_image_xmp(*sub)) return true;
            }
        } else if (auto* group = elem->try_as<Group>()) {
            if (auto sub = group->get_content()) {
                if (display_image_xmp(*sub)) return true;
            }
        }
    }
    return false;
}

int main(int argc, char* argv[])
{
    Library lib;

    std::string input1  = Library::get_resource_directory() + "Sample_Input/sample.pdf";
    std::string input2  = Library::get_resource_directory() + "Sample_Input/Ducky_with_metadata.pdf";
    std::string output  = "sample-metadata-out.pdf";

    if (argc > 1) input1  = argv[1];
    if (argc > 2) input2  = argv[2];
    if (argc > 3) output  = argv[3];

    std::cout << "Input files " << input1 << " and " << input2
              << ". Writing to output file " << output << std::endl;

    try {
        display_document_metadata(input1, output);

        // Display image metadata from the second document.
        {
            Document doc(input2);
            Content content = doc.get_page(0).get_content();
            if (!display_image_xmp(content)) {
                std::cout << "No image elements found on page 0." << std::endl;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
