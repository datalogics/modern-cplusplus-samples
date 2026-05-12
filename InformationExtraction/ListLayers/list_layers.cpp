/*
 * This sample searches for and lists the names of the color layers found in a PDF document.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace datalogics_interface;

// Recursively collect all OCGs from an order array (depth-first).
static void collect_ocgs(OptionalContentOrderArray& arr,
                         std::vector<OptionalContentGroup>& out)
{
    for (int i = 0; i < arr.get_length(); ++i) {
        auto node = arr.get(i);
        if (auto* leaf = node->try_as<OptionalContentOrderLeaf>()) {
            out.push_back(leaf->get_optional_content_group());
        } else if (auto* sub = node->try_as<OptionalContentOrderArray>()) {
            collect_ocgs(*sub, out);
        }
    }
}

int main(int argc, char* argv[])
{
    std::cout << "ListLayers Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/Layers.pdf";

    if (argc > 1) input_path = argv[1];

    std::cout << "Input file: " << input_path << std::endl;

    try {
        Document doc(input_path);

        // Collect OCGs from the order array in the default config.
        OptionalContentConfig config(doc);
        std::vector<OptionalContentGroup> ocgs;

        auto order = config.get_order();
        if (order) {
            collect_ocgs(*order, ocgs);
        }

        // Print name and intent for each OCG.
        for (auto& ocg : ocgs) {
            std::cout << ocg.get_name() << std::endl;
            std::cout << "  Intent: [";
            auto intent = ocg.get_intent();
            for (std::size_t i = 0; i < intent.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << intent[i];
            }
            std::cout << "]" << std::endl;
        }

        // Print the on/off states for all collected OCGs.
        OptionalContentContext ctx(doc);
        std::cout << "Optional content states: [";
        auto states = ctx.get_ocg_states(ocgs);
        for (std::size_t i = 0; i < states.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << (states[i] ? "True" : "False");
        }
        std::cout << "]" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
