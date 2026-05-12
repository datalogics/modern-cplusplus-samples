/*
 * This sample demonstrates working with color separations with Encapsulated PostScript (EPS)
 * graphics from a PDF file.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "EPS Separations Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input1 = Library::get_resource_directory() + "Sample_Input/spotcolors1.pdf";
    std::string input2 = Library::get_resource_directory() + "Sample_Input/spotcolors.pdf";

    if (argc > 1) input1 = argv[1];
    if (argc > 2) input2 = argv[2];

    std::cout << "Will perform simple separation on " << input1
              << " and complex separation on " << input2 << std::endl;

    try {
        // --- Simple separation: one plate per ink ---
        {
            Document doc(input1);
            for (int pg_num = 0; pg_num < doc.get_num_pages(); ++pg_num) {
                Page pg = doc.get_page(pg_num);
                std::vector<Ink> inks = pg.list_inks();

                std::vector<SeparationPlate> plates;
                std::vector<std::ofstream> streams;
                streams.reserve(inks.size());

                for (const Ink& ink : inks) {
                    std::cout << "Found color " << ink.get_colorant_name()
                              << " on page " << (pg_num + 1) << "." << std::endl;

                    std::string filename = ink.get_colorant_name() + "_page"
                        + std::to_string(pg_num + 1) + ".eps";
                    streams.emplace_back(filename, std::ios::binary);
                    plates.emplace_back(ink, streams.back());
                }

                SeparationParams params(std::move(plates));
                pg.make_separations(params);

                // Delete empty EPS files (inks not actually used on the page)
                auto result_plates = params.get_plates();
                for (size_t i = 0; i < result_plates.size(); ++i) {
                    streams[i].close();
                    std::string filename = inks[i].get_colorant_name() + "_page"
                        + std::to_string(pg_num + 1) + ".eps";
                    if (std::filesystem::file_size(filename) == 0) {
                        std::filesystem::remove(filename);
                    } else {
                        std::cout << "Wrote separation: " << filename << std::endl;
                    }
                }
            }
        }

        // --- Complex separation: adjust density before generating plates ---
        {
            Document doc(input2);
            for (int pg_num = 0; pg_num < doc.get_num_pages(); ++pg_num) {
                Page pg = doc.get_page(pg_num);
                std::vector<Ink> inks = pg.list_inks();

                std::vector<SeparationPlate> plates;
                std::vector<std::ofstream> streams;
                streams.reserve(inks.size());

                for (const Ink& ink : inks) {
                    std::string filename = "complex_" + ink.get_colorant_name() + "_page"
                        + std::to_string(pg_num + 1) + ".eps";
                    streams.emplace_back(filename, std::ios::binary);

                    SeparationPlate plate(ink, streams.back());
                    plate.set_density(1.0);
                    plates.push_back(std::move(plate));
                }

                SeparationParams params(std::move(plates));
                pg.make_separations(params);

                for (size_t i = 0; i < inks.size(); ++i) {
                    streams[i].close();
                    std::string filename = "complex_" + inks[i].get_colorant_name() + "_page"
                        + std::to_string(pg_num + 1) + ".eps";
                    if (std::filesystem::file_size(filename) == 0) {
                        std::filesystem::remove(filename);
                    } else {
                        std::cout << "Wrote separation: " << filename << std::endl;
                    }
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
