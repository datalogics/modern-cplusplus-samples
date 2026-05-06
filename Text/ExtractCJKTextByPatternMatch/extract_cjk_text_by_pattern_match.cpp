/*
 * Copyright (c) Datalogics, Inc. All rights reserved.
 *
 * This sample extracts text that matches a given CJK pattern in a PDF
 * document and saves the text to a file.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <fstream>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main()
{
    std::cout << "ExtractCJKTextByPatternMatch Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/ExtractUnicodeText.pdf";
    std::string output_path = "ExtractCJKTextByPatternMatch-out.txt";
    // CJK pattern: "The World Human Rights Declaration" in Japanese (UTF-8)
    std::string pattern = u8"\u300e\u4e16\u754c\u4eba\u6a29\u5ba3\u8a00\u300f";

    std::cout << "Input file:  " << input_path << std::endl;

    try {
        Document doc(input_path);
        int n_pages = doc.get_num_pages();

        std::ofstream outfile(output_path);
        std::cout << "Writing to: " << output_path << std::endl;

        // Create a DocTextFinder and retrieve matching phrases
        WordFinderConfig wf_config;
        wf_config.set_no_hyphen_detection(true);
        DocTextFinder doc_text_finder(doc, wf_config);
        auto doc_matches = doc_text_finder.get_match_list(0, n_pages - 1, pattern);

        for (const auto& match : doc_matches) {
            outfile << match.get_match_string() << "\n";
        }

        outfile.close();
        std::cout << "Done." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
