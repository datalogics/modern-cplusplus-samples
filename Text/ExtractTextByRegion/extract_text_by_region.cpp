/*
 * Copyright (c) Datalogics, Inc. All rights reserved.
 *
 * This sample extracts text from a specific target area of a page in a PDF
 * document and saves the text to a file.
 */

#include "../../_Common/extract_text.hpp"

#include <datalogics_interface/datalogics_interface.hpp>

#include <fstream>
#include <iostream>
#include <string>

using namespace datalogics_interface;

// Rectangular region to extract text in points (origin at bottom left)
// (545,576,694,710) is a rectangle encompassing an invoice entry for this sample.
static const double kTargetLeft   = 545;
static const double kTargetRight  = 576;
static const double kTargetBottom = 694;
static const double kTargetTop    = 710;

// Returns true if all four corners of the quad fall within the target region.
static bool check_within_region(const Quad& quad)
{
    return quad.bottom_left.h  >= kTargetLeft   &&
           quad.bottom_right.h <= kTargetRight  &&
           quad.top_left.h     >= kTargetLeft   &&
           quad.top_right.h    <= kTargetRight  &&
           quad.bottom_left.v  >= kTargetBottom &&
           quad.top_left.v     <= kTargetTop    &&
           quad.bottom_right.v >= kTargetBottom &&
           quad.top_right.v    <= kTargetTop;
}

int main()
{
    std::cout << "ExtractTextByRegion Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/ExtractTextByRegion.pdf";
    std::string output_path = "ExtractTextByRegion-out.txt";

    std::cout << "Input file: " << input_path << std::endl;

    try {
        Document doc(input_path);

        std::ofstream outfile(output_path);
        std::cout << "Writing to: " << output_path << std::endl;

        // Extract text and details for all pages (matching C# behavior)
        for (int page_num = 0; page_num < doc.get_num_pages(); ++page_num) {
            auto result = dl_common::get_text_and_details(doc, page_num);

            for (const auto& text_info : result) {
                bool all_quads_within_region = true;
                for (const auto& quad : text_info.quads) {
                    if (!check_within_region(quad)) {
                        all_quads_within_region = false;
                        break;
                    }
                }
                if (all_quads_within_region) {
                    outfile << text_info.text << "\n";
                }
            }
        }

        outfile.close();
        std::cout << "Done." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
