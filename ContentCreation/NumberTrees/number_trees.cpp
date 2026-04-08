/*
 * This sample demonstrates working with the NumberTree class. A number tree is a type
 * of dictionary often used as a data structure in PDF files. It is similar to a name
 * tree, except that the keys are integers sorted in ascending numerical order.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/library.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/page.hpp>
#include <datalogics_interface/number_tree.hpp>
#include <datalogics_interface/pdf_string.hpp>
#include <datalogics_interface/pdf_object.hpp>
#include <datalogics_interface/pdf_dict.hpp>
#include <datalogics_interface/geometry.hpp>

#include <iostream>
#include <memory>
#include <string>

using namespace datalogics_interface;

int main() {
    std::cout << "NumberTree Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    Document doc;
    Rect rect(0, 0, 612, 792);
    doc.create_page(Document::before_first_page, rect);
    std::cout << "Created new document and first page." << std::endl;

    // Create a NumberTree and put a key-value pair in it
    NumberTree numbertree(doc);

    PDFString value1("Smorgasbord", doc, false, false);
    numbertree.put(1, value1);
    std::cout << "\nCreated NumberTree and added first key-value pair." << std::endl;

    PDFString value2("Copasetic", doc, false, false);
    numbertree.put(2, value2);

    // Retrieve second entry
    auto lookup = numbertree.get(2);
    std::cout << "\nRetrieving two entries:" << std::endl;
    std::cout << (lookup ? lookup->to_string() : "null") << std::endl;

    // Retrieve first entry
    lookup = numbertree.get(1);
    std::cout << (lookup ? lookup->to_string() : "null") << std::endl;

    // Remove first entry
    numbertree.remove(1);

    std::cout << "\nAfter removing entry 1, we now have:" << std::endl;
    lookup = numbertree.get(1);
    std::cout << (lookup ? lookup->to_string() : "null") << std::endl;
    lookup = numbertree.get(2);
    std::cout << (lookup ? lookup->to_string() : "null") << std::endl;

    // Get the PDFDict from the NumberTree
    auto dict = numbertree.get_pdf_dict();
    std::cout << "\nThe PDFDict from the NumberTree:" << std::endl;
    std::cout << (dict ? dict->to_string() : "null") << std::endl;

    std::cout << "\nDone." << std::endl;
    return 0;
}
