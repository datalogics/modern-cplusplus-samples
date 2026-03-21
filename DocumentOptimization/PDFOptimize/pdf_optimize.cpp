/*
 * This sample program demonstrates the use of PDFOptimizer. This compresses a PDF document
 * to make it smaller so it's easier to process and download.
 *
 * NOTE: Some documents can't be compressed because they're already well-compressed or contain
 * content that can't be assumed is safe to be removed. However you can fine tune the optimization
 * to suit your application's needs and drop such content to achieve better compression if you
 * already know it's unnecessary.
 *
 * Copyright (c) 2007-2023, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <filesystem>
#include <iostream>
#include <string>

using namespace datalogics_interface;
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    std::cout << "PDF Optimizer:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string input_path  = Library::get_resource_directory() + "Sample_Input/sample.pdf";
    std::string output_path = "PDFOptimizer-out.pdf";

    if (argc > 1) input_path  = argv[1];
    if (argc > 2) output_path = argv[2];

    std::cout << "Input file: " << input_path << std::endl;
    std::cout << "Writing to output " << output_path << std::endl;

    try {
        Document doc(input_path);

        PDFOptimizer optimizer;

        auto before_size = static_cast<double>(fs::file_size(input_path));

        optimizer.optimize(doc, output_path);

        auto after_size = static_cast<double>(fs::file_size(output_path));

        std::cout << std::endl;
        std::cout << "Optimized file: "
                  << (after_size / before_size * 100.0)
                  << "% the size of the original." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
