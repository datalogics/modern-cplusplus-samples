/*
 * This sample demonstrates using the DocTextFinder to find examples of a specific phrase in a PDF
 * document that matches a user-supplied regular expression. When the sample finds the text it
 * will redact the phrase from the output document.
 *
 * Copyright (c) 2007-2024, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "AddRegexRedaction Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input_path  = Library::get_resource_directory() + "Sample_Input/AddRegexRedaction.pdf";
    std::string output1     = "AddRegexRedaction-out.pdf";
    std::string output2     = "AddRegexRedaction-out-applied.pdf";

    // Phone numbers
    std::string regex = "((1-)?(\\()?\\d{3}(\\))?(\\s)?(-)?\\d{3}-\\d{4})";

    if (argc > 1) input_path = argv[1];

    try {
        Document doc(input_path);
        int n_pages = doc.get_num_pages();

        std::cout << "Input file: " << input_path << std::endl;

        // Create a DocTextFinder and search with the regular expression
        WordFinderConfig wf_config;
        wf_config.set_no_hyphen_detection(true);
        DocTextFinder doc_text_finder(doc, wf_config);

        auto doc_matches = doc_text_finder.get_match_list(0, n_pages - 1, regex);

        // Redaction color: red
        Color red{1.0, 0.0, 0.0};

        for (const auto& match : doc_matches) {
            std::cout << match.get_match_string() << std::endl;

            for (const auto& q_info : match.get_quad_info()) {
                Page docpage = doc.get_page(q_info.get_page_num());

                Redaction red_fill(docpage, q_info.get_quads());

                // Fill the "normal" appearance with 25% red
                red_fill.set_fill_normal(true);
                red_fill.set_fill_color(red, 0.25);
            }
        }

        // Save with unapplied redactions
        doc.save(SaveFlags::Full, output1);
        std::cout << "Wrote a PDF document with unapplied redactions." << std::endl;

        // Apply all redactions
        doc.apply_redactions();

        // Save the redacted document
        doc.save(SaveFlags::Full, output2);
        std::cout << "Wrote a redacted PDF document." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
