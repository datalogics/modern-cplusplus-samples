/*
 * This sample finds and describes the bookmarks included in a PDF document.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

static void enumerate_bookmarks(const std::unique_ptr<Bookmark>& b)
{
    if (!b) return;

    std::cout << b->to_string() << ": ";
    std::cout << b->get_title();

    auto v = b->get_view_destination();
    if (v) {
        std::cout << ", page " << v->get_page_number();
        std::cout << ", fit " << v->get_fit_type();
        std::cout << ", dest rect " << v->get_dest_rect().to_string();
        std::cout << ", zoom " << v->get_zoom();
    }

    std::cout << std::endl;

    enumerate_bookmarks(b->get_first_child());
    enumerate_bookmarks(b->get_next());
}

int main(int argc, char* argv[])
{
    std::cout << "ListBookmarks Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/sample.pdf";

    if (argc > 1) input_path = argv[1];

    std::cout << "Input file: " << input_path << std::endl;

    try {
        Document doc(input_path);

        auto root = doc.get_bookmark_root();
        enumerate_bookmarks(root);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
