/*
 * Copyright (c) 2007-2025, Datalogics, Inc. All rights reserved.
 *
 * This sample demonstrates using the DocTextFinder to find examples of a specific
 * phrase in a PDF document that match a user-supplied regular expression. When the
 * sample finds the text it highlights each match and saves the file as an output document.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/library.hpp>
#include <datalogics_interface/doc_text_finder.hpp>
#include <datalogics_interface/page.hpp>
#include <datalogics_interface/highlight_annotation.hpp>
#include <datalogics_interface/annotation.hpp>
#include <datalogics_interface/form.hpp>

#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "RegexTextSearch Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/RegexTextSearch.pdf";
    std::string output_path = "RegexTextSearch-out.pdf";

    // Highlight occurrences of phrases matching this regular expression.
    // Phone numbers:
    std::string regex = "((1-)?\\(?\\d{3}\\)?(\\s)?(-)?\\d{3}-\\d{4})";
    // Email addresses:
    // std::string regex = "(\\b[\\w.!#$%&'*+\\/=?^`{|}~-]+@[\\w-]+(?:\\.[\\w-]+)*\\b)";
    // URLs:
    // std::string regex = "((https?:\\/\\/(?:www\\.|(?!www))[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\\.[^\\s]{2,}|www\\.[a-zA-Z0-9][a-zA-Z0-9-]+[a-zA-Z0-9]\\.[^\\s]{2,}|https?:\\/\\/(?:www\\.|(?!www))[a-zA-Z0-9]+\\.[^\\s]{2,}|www\\.[a-zA-Z0-9]+\\.[^\\s]{2,}))";

    if (argc > 1) input_path = argv[1];

    try {
        Document doc(input_path);
        int n_pages = doc.get_num_pages();

        std::cout << "Input file:  " << input_path << std::endl;

        WordFinderConfig wf_config;
        wf_config.set_no_hyphen_detection(true);
        DocTextFinder doc_text_finder(doc, wf_config);
        auto doc_matches = doc_text_finder.get_match_list(0, n_pages - 1, regex);

        for (const auto& match : doc_matches) {
            std::cout << match.get_match_string() << std::endl;

            for (const auto& qi : match.get_quad_info()) {
                Page docpage = doc.get_page(qi.get_page_num());
                // Create a highlight annotation for the matched quads
                HighlightAnnotation highlight(docpage, qi.get_quads());
                auto appearance = highlight.generate_appearance();
                if (appearance) {
                    highlight.set_normal_appearance(*appearance);
                }
            }
        }

        // Save the document with the highlighted matched strings
        doc.save(SaveFlags::Full, output_path);

        std::cout << "Saved output to " << output_path << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
