/*
 * This sample demonstrates working with page labels in a PDF document. Each PDF file has a
 * data structure that governs how page numbers appear, such as the font and type of numeral.
 *
 * Copyright (c) 2007-2023, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <datalogics_interface/library.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/page_label.hpp>
#include <iostream>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "Page Labels Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string sInput = Library::get_resource_directory() + "Sample_Input/pagelabels.pdf";

    if (argc > 1)
        sInput = argv[1];

    std::cout << "Input file " << sInput << std::endl;

    Document doc(sInput);

    // Extract a page label from the document
    std::string labelString = doc.find_label_for_page_num(doc.get_num_pages() - 1);
    std::cout << "Last page in the document is labeled " << labelString << std::endl;

    // Find index for that label
    int index = doc.find_page_num_for_label(labelString);
    std::cout << labelString << " has an index of " << index << " in the document." << std::endl;

    // Add a new page label range starting on page 5 with Roman lowercase, prefix "A-", starting at 1
    PageLabel pl(5, NumberStyle::RomanLowercase, "A-", 1);

    std::vector<PageLabel> labels = doc.get_page_labels();
    labels.push_back(pl);
    doc.set_page_labels(labels);

    std::cout << "Added page range starting on page 5." << std::endl;

    // Change the properties of the third page range
    labels = doc.get_page_labels();
    labels[2].set_prefix("Section 3-");
    labels[2].set_first_number_in_range(2);
    doc.set_page_labels(labels);

    std::cout << "Changed the prefix for the third range." << std::endl;

    // Walk the list of page labels
    for (const PageLabel& label : doc.get_page_labels()) {
        std::cout << "Label range starts on page " << label.get_start_page_index()
                  << ", ends on page " << label.get_end_page_index() << std::endl;
        std::cout << "The prefix is '" << label.get_prefix()
                  << "' and begins with number " << label.get_first_number_in_range() << std::endl;
        std::cout << std::endl;
    }

    return 0;
}
