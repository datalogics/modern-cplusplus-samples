/*
 * Copyright (c) 2022-2025, Datalogics, Inc. All rights reserved.
 *
 * This sample processes PDF files in a folder and extracts text from specific
 * regions of its pages and saves the text to a CSV file.
 */

#include "../../_Common/extract_text.hpp"

#include <datalogics_interface/datalogics_interface.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/library.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <array>

namespace fs = std::filesystem;
using namespace datalogics_interface;

// Rectangular regions (Left, Right, Bottom, Top) in points
struct Region {
    double left, right, bottom, top;
};

static const Region kInvoiceNumber { 500, 590, 692, 710 };
static const Region kDate          { 500, 590, 680, 700 };
static const Region kOrderNumber   { 500, 590, 672, 688 };
static const Region kCustId        { 500, 590, 636, 654 };
static const Region kTotal         { 500, 590,  52,  73 };

static const std::vector<Region> kRegions = {
    kInvoiceNumber, kDate, kOrderNumber, kCustId, kTotal
};

// Returns true if the quad falls entirely within the region.
static bool check_within_region(const Quad& quad, const Region& region)
{
    return quad.bottom_left.h  >= region.left   &&
           quad.bottom_right.h <= region.right  &&
           quad.top_left.h     >= region.left   &&
           quad.top_right.h    <= region.right  &&
           quad.bottom_left.v  >= region.bottom &&
           quad.top_left.v     <= region.top    &&
           quad.bottom_right.v >= region.bottom &&
           quad.top_right.v    <= region.top;
}

int main()
{
    std::cout << "ExtractTextFromMultiRegions Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string input_dir = Library::get_resource_directory() + "Sample_Input/ExtractTextFromMultiRegions";
    std::string output_path = "ExtractTextFromMultiRegions-out.csv";

    std::cout << "Writing to: " << output_path << std::endl;

    try {
        std::ofstream outfile(output_path);
        outfile << "Filename,Invoice Number,Date,Order Number,Customer ID,Total\n";

        for (const auto& entry : fs::directory_iterator(input_dir)) {
            if (!entry.is_regular_file()) continue;
            std::string file_path = entry.path().string();
            std::string file_name = entry.path().filename().string();

            std::cout << "Input file: " << file_name << std::endl;

            Document doc(file_path);
            outfile << file_name;

            for (int page_num = 0; page_num < doc.get_num_pages(); ++page_num) {
                auto result = dl_common::get_text_and_details(doc, page_num);

                for (const auto& region : kRegions) {
                    outfile << ",";
                    for (const auto& text_info : result) {
                        bool all_quads_within = true;
                        for (const auto& quad : text_info.quads) {
                            if (!check_within_region(quad, region)) {
                                all_quads_within = false;
                                break;
                            }
                        }
                        if (all_quads_within) {
                            outfile << text_info.text;
                        }
                    }
                }
                outfile << "\n";
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
