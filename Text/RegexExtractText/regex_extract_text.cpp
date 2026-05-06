/*
 * Copyright (c) Datalogics, Inc. All rights reserved.
 *
 * This sample demonstrates using DocTextFinder to find instances of a phrase
 * that matches a user-supplied regular expression. The output is a JSON file
 * with the match information.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace datalogics_interface;

static std::string json_escape(const std::string& s)
{
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

static std::string dbl_str(double v)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6) << v;
    return ss.str();
}

int main(int argc, char* argv[])
{
    std::cout << "RegexExtractText Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/RegexExtractText.pdf";
    std::string output_path = "RegexExtractText-out.json";

    // Phone numbers regex
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

        // Build JSON output
        std::cout << "Writing JSON to " << output_path << std::endl;
        std::ofstream out(output_path);
        out << "[\n";

        for (std::size_t mi = 0; mi < doc_matches.size(); ++mi) {
            const auto& match = doc_matches[mi];
            out << "  {\n";
            out << "    \"match-phrase\": \"" << json_escape(match.get_match_string()) << "\",\n";
            out << "    \"match-quads\": [\n";

            const auto& quad_info_list = match.get_quad_info();
            std::size_t quad_entry_count = 0;

            // Count total quad entries
            for (const auto& qi : quad_info_list) {
                quad_entry_count += qi.get_quads().size();
            }

            std::size_t emitted = 0;
            for (const auto& qi : quad_info_list) {
                for (const auto& quad : qi.get_quads()) {
                    out << "      {\n";
                    out << "        \"page-number\": " << qi.get_page_num() << ",\n";
                    out << "        \"quad-location\": {\n";
                    out << "          \"bottom-left\": { \"x\": " << dbl_str(quad.bottom_left.h)
                        << ", \"y\": " << dbl_str(quad.bottom_left.v) << " },\n";
                    out << "          \"bottom-right\": { \"x\": " << dbl_str(quad.bottom_right.h)
                        << ", \"y\": " << dbl_str(quad.bottom_right.v) << " },\n";
                    out << "          \"top-left\": { \"x\": " << dbl_str(quad.top_left.h)
                        << ", \"y\": " << dbl_str(quad.top_left.v) << " },\n";
                    out << "          \"top-right\": { \"x\": " << dbl_str(quad.top_right.h)
                        << ", \"y\": " << dbl_str(quad.top_right.v) << " }\n";
                    out << "        }\n";
                    out << "      }";
                    ++emitted;
                    if (emitted < quad_entry_count) out << ",";
                    out << "\n";
                }
            }

            out << "    ]\n";
            out << "  }";
            if (mi + 1 < doc_matches.size()) out << ",";
            out << "\n";
        }

        out << "]\n";
        out.close();

        std::cout << "Done." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
