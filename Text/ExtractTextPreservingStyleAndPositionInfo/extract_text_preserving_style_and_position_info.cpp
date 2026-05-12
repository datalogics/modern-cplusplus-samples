/*
 * Copyright (c) Datalogics, Inc. All rights reserved.
 *
 * This sample extracts text and details (style and position) of that text
 * from a PDF document and saves the text to a JSON file.
 */

#include "../../_Common/extract_text.hpp"

#include <datalogics_interface/datalogics_interface.hpp>

#include <cmath>
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

static std::string round2(double v)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << v;
    return ss.str();
}

static std::string round3(double v)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3) << v;
    return ss.str();
}

static std::string point_str(const Point& p)
{
    return "(" + round2(p.h) + ", " + round2(p.v) + ")";
}

static void save_json(const std::vector<dl_common::TextAndDetailsObject>& result,
                      const std::string& output_path)
{
    std::ofstream out(output_path);
    out << "[\n";

    for (std::size_t wi = 0; wi < result.size(); ++wi) {
        const auto& text_info = result[wi];
        out << "  {\n";
        out << "    \"text\": \"" << json_escape(text_info.text) << "\",\n";

        // Quads array
        out << "    \"quads\": [\n";
        for (std::size_t qi = 0; qi < text_info.quads.size(); ++qi) {
            const auto& q = text_info.quads[qi];
            out << "      {\n";
            out << "        \"top-left\": \"" << point_str(q.top_left) << "\",\n";
            out << "        \"top-right\": \"" << point_str(q.top_right) << "\",\n";
            out << "        \"bottom-left\": \"" << point_str(q.bottom_left) << "\",\n";
            out << "        \"bottom-right\": \"" << point_str(q.bottom_right) << "\"\n";
            out << "      }";
            if (qi + 1 < text_info.quads.size()) out << ",";
            out << "\n";
        }
        out << "    ],\n";

        // Styles array
        out << "    \"styles\": [\n";
        for (std::size_t si = 0; si < text_info.style_list.size(); ++si) {
            const auto& st = text_info.style_list[si];
            out << "      {\n";
            out << "        \"char-index\": \"" << st.char_index << "\",\n";
            out << "        \"font-name\": \"" << json_escape(st.style.font_name) << "\",\n";
            out << "        \"font-size\": \"" << round2(st.style.font_size) << "\",\n";
            out << "        \"color-space\": \"" << json_escape(st.style.color.space.name) << "\",\n";
            out << "        \"color-values\": [";
            for (std::size_t ci = 0; ci < st.style.color.value.size(); ++ci) {
                if (ci > 0) out << ", ";
                out << "\"" << round3(st.style.color.value[ci]) << "\"";
            }
            out << "]\n";
            out << "      }";
            if (si + 1 < text_info.style_list.size()) out << ",";
            out << "\n";
        }
        out << "    ]\n";

        out << "  }";
        if (wi + 1 < result.size()) out << ",";
        out << "\n";
    }

    out << "]\n";
    out.close();
}

int main()
{
    std::cout << "ExtractTextPreservingStyleAndPositionInfo Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/sample.pdf";
    std::string output_path = "ExtractTextPreservingStyleAndPositionInfo-out.json";

    std::cout << "Input file: " << input_path << std::endl;

    try {
        Document doc(input_path);

        // Extract text and details for all pages
        std::vector<dl_common::TextAndDetailsObject> all_results;
        for (int page_num = 0; page_num < doc.get_num_pages(); ++page_num) {
            auto page_result = dl_common::get_text_and_details(doc, page_num);
            all_results.insert(all_results.end(),
                               std::make_move_iterator(page_result.begin()),
                               std::make_move_iterator(page_result.end()));
        }

        std::cout << "Writing JSON to " << output_path << std::endl;
        save_json(all_results, output_path);

        std::cout << "Done." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
