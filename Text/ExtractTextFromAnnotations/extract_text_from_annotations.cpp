/*
 * Copyright (c) 2022-2025, Datalogics, Inc. All rights reserved.
 *
 * This sample extracts text from the annotations in a PDF document
 * and saves the text to a JSON file.
 */

#include "../../_Common/extract_text.hpp"

#include <datalogics_interface/datalogics_interface.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/library.hpp>

#include <fstream>
#include <iostream>
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

int main()
{
    std::cout << "Annotations Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/sample_annotations.pdf";
    std::string output_path = "ExtractTextFromAnnotations-out.json";

    std::cout << "Input file: " << input_path << std::endl;

    try {
        Document doc(input_path);

        auto result = dl_common::get_annotation_text(doc);

        // Print to console
        for (const auto& annot : result) {
            std::cout << "Annotation Type > " << annot.annotation_type << std::endl;
            std::cout << "Annotation Text > " << annot.annotation_text << std::endl;
        }

        // Write JSON output
        std::cout << "Writing JSON to " << output_path << std::endl;
        std::ofstream out(output_path);
        out << "[\n";
        for (std::size_t i = 0; i < result.size(); ++i) {
            out << "  {\n";
            out << "    \"AnnotationType\": \"" << json_escape(result[i].annotation_type) << "\",\n";
            out << "    \"AnnotationText\": \"" << json_escape(result[i].annotation_text) << "\"\n";
            out << "  }";
            if (i + 1 < result.size()) out << ",";
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
