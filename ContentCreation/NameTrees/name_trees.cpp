/*
 * This sample demonstrates working with the NameTree class. A name tree is a type
 * of dictionary often used as a data structure in PDF files. Unlike a standard
 * dictionary, a name tree uses names as keys to map to data objects.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/library.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/page.hpp>
#include <datalogics_interface/name_tree.hpp>
#include <datalogics_interface/pdf_string.hpp>
#include <datalogics_interface/pdf_object.hpp>
#include <datalogics_interface/pdf_dict.hpp>
#include <datalogics_interface/geometry.hpp>

#include <iostream>
#include <memory>
#include <string>

using namespace datalogics_interface;

int main() {
    std::cout << "NameTree Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    Document doc;
    Rect rect(0, 0, 612, 792);
    doc.create_page(Document::before_first_page, rect);
    std::cout << "Created new document and first page." << std::endl;

    // Create a NameTree and put a key-value pair in it
    NameTree nametree(doc);

    PDFString key("Bailout", doc, false, false);
    PDFString value("Smorgasbord", doc, false, false);
    nametree.put(key, value);
    std::cout << "\nCreated NameTree and added first key-value pair." << std::endl;

    // Put another key-value pair
    PDFString key2("Brandish", doc, false, false);
    PDFString value2("Copasetic", doc, false, false);
    nametree.put(key2, value2);

    // Retrieve second entry
    PDFString search2("Brandish", doc, false, false);
    auto lookup = nametree.get(search2);
    std::cout << "\nRetrieving two entries:" << std::endl;
    std::cout << (lookup ? lookup->to_string() : "null") << std::endl;

    // Retrieve first entry
    PDFString search1("Bailout", doc, false, false);
    lookup = nametree.get(search1);
    std::cout << (lookup ? lookup->to_string() : "null") << std::endl;

    // Remove first entry
    PDFString remove_key("Bailout", doc, false, false);
    nametree.remove(remove_key);

    std::cout << "\nAfter removing entry 1, we now have:" << std::endl;
    PDFString check1("Bailout", doc, false, false);
    lookup = nametree.get(check1);
    std::cout << (lookup ? lookup->to_string() : "null") << std::endl;
    PDFString check2("Brandish", doc, false, false);
    lookup = nametree.get(check2);
    std::cout << (lookup ? lookup->to_string() : "null") << std::endl;

    // Get the PDFDict from the NameTree
    auto dict = nametree.get_pdf_dict();
    std::cout << "\nThe PDFDict from the NameTree:" << std::endl;
    std::cout << (dict ? dict->to_string() : "null") << std::endl;

    // Use Document methods to operate on NameTree
    auto doc_tree = doc.create_name_tree("MyNameTree");
    PDFString argyle_key("Argyle", doc, false, false);
    PDFString seamstress_val("Seamstress", doc, false, false);
    doc_tree->put(argyle_key, seamstress_val);
    std::cout << "Created a NameTree object in the document." << std::endl;

    std::cout << "\nTwo searches for NameTree using get_name_tree() method; first fails, second succeeds:" << std::endl;
    auto found_tree = doc.get_name_tree("Garbage");
    std::cout << (found_tree ? "found" : "null") << std::endl;
    found_tree = doc.get_name_tree("MyNameTree");
    std::cout << (found_tree ? "found" : "null") << std::endl;

    std::cout << "\nRemove the NameTree from the document." << std::endl;
    doc.remove_name_tree("Garbage");
    found_tree = doc.get_name_tree("MyNameTree");
    std::cout << (found_tree ? "found" : "null") << std::endl;
    doc.remove_name_tree("MyNameTree");
    found_tree = doc.get_name_tree("MyNameTree");
    std::cout << (found_tree ? "found" : "null") << std::endl;

    std::cout << "Done." << std::endl;
    return 0;
}
