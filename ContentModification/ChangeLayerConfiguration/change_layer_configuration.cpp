/*
 * This sample changes the On/Off configuration for a set of Optional Content Groups,
 * or layers, within a PDF document. By changing the On or Off state in the default
 * configuration, the sample makes the layers visible or invisible when opened in a
 * PDF viewer.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <vector>

using namespace datalogics_interface;

namespace {
void collect_ocgs_from_order(OptionalContentOrderArray& order,
                             std::vector<OptionalContentGroup>& out)
{
    for (int i = 0; i < order.get_length(); ++i) {
        auto node = order.get(i);
        if (auto* leaf = node->try_as<OptionalContentOrderLeaf>()) {
            out.push_back(leaf->get_optional_content_group());
        } else if (auto* sub = node->try_as<OptionalContentOrderArray>()) {
            collect_ocgs_from_order(*sub, out);
        }
    }
}
} // namespace

int main(int argc, char* argv[]) {
    std::cout << "ChangeLayerConfiguration Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    /*
     * The Layers.pdf file has four layers: 'Layer 1', 'Layer 2', 'Layer 3',
     * and 'Guides and Grids'. By default, the first 3 layers are on,
     * and 'Guides and Grids' is off. We want to create a new document with:
     *  o   Layer 1 = OFF
     *  o   Layer 2 = OFF
     *  o   Layer 3 = ON
     *  o   Guides and Grids = ON
     *
     * We use Method One: set BaseState to Off, add 'Layer 3' and 'Guides and Grids'
     * to the ON array.
     */

    std::string sInput = Library::get_resource_directory() + "Sample_Input/Layers.pdf";
    std::string sOutput = "ChangeLayerConfiguration-out.pdf";

    if (argc > 1)
        sInput = argv[1];
    if (argc > 2)
        sOutput = argv[2];

    std::cout << "Input file: " << sInput << ", writing to " << sOutput << std::endl;

    Document doc(sInput);

    // Get the default OptionalContentConfig
    OptionalContentConfig config(doc);

    // Collect all OCGs via the order tree (on/off arrays may be empty
    // when BaseState is ON, since all layers default to visible)
    std::vector<OptionalContentGroup> allOcgs;
    auto order = config.get_order();
    if (order) {
        collect_ocgs_from_order(*order, allOcgs);
    }

    // Set the BaseState to Off
    config.set_base_state(OptionalContentBaseState::Off);

    // Build the ON array: Layer 3 and Guides and Grids
    std::vector<OptionalContentGroup> onList;
    for (auto& ocg : allOcgs) {
        const std::string name = ocg.get_name();
        if (name == "Layer 3" || name == "Guides and Grids")
            onList.push_back(OptionalContentGroup(doc, name));
    }

    config.set_on_array(std::move(onList));
    config.set_off_array({});

    doc.save(SaveFlags::Full, sOutput);
    std::cout << "Saved to " << sOutput << std::endl;

    return 0;
}
