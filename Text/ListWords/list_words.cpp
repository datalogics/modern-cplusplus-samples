/*
 * Copyright (c) Datalogics, Inc. All rights reserved.
 *
 * This sample lists the text for the words in a PDF document.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/library.hpp>
#include <datalogics_interface/word_finder.hpp>
#include <datalogics_interface/word_finder_config.hpp>
#include <datalogics_interface/word.hpp>
#include <datalogics_interface/style.hpp>

#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "ListWords Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/sample.pdf";
    if (argc > 1) input_path = argv[1];

    std::cout << "Input file: " << input_path << std::endl;

    try {
        Document doc(input_path);
        int n_pages = doc.get_num_pages();

        WordFinderConfig word_config;
        word_config.set_ignore_char_gaps(true);
        word_config.set_ignore_line_gaps(false);
        word_config.set_no_annotations(true);
        word_config.set_no_encoding_guess(true);    // leave non-Roman single-byte font alone
        word_config.set_unknown_to_std_enc(false);  // Std Roman treatment for custom encoding
        word_config.set_disable_tagged_pdf(true);   // legacy mode WordFinder creation
        word_config.set_no_xy_sort(false);
        word_config.set_preserve_spaces(false);
        word_config.set_no_ligature_expansion(false);
        word_config.set_no_hyphen_detection(false);
        word_config.set_trust_nb_space(false);
        word_config.set_no_ext_char_offset(false);  // text extraction efficiency
        word_config.set_no_style_info(false);       // text extraction efficiency

        WordFinder word_finder(doc, WordFinderVersion::Latest, word_config);

        for (int i = 0; i < n_pages; ++i) {
            auto page_words = word_finder.get_word_list(i);
            for (const auto& word : page_words) {
                auto quads = word.get_quads();
                for (const auto& q : quads) {
                    std::cout << q.to_string() << std::endl;
                }

                auto style_transitions = word.get_style_transitions();
                for (const auto& st : style_transitions) {
                    std::cout << "char_index=" << st.char_index
                              << " font=" << st.style.get_font_name()
                              << " size=" << st.style.get_font_size()
                              << std::endl;
                }

                auto attrs = word.get_attributes();
                std::cout << "attributes=0x" << std::hex << static_cast<int>(attrs) << std::dec << std::endl;
                std::cout << word.get_text() << std::endl;
            }
        }

        std::cout << "Pages=" << n_pages << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
