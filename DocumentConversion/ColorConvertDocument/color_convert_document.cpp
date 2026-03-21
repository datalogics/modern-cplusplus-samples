/*
 * The ColorConvertDocument sample program demonstrates working with color conversion in PDF documents.
 * The color conversion process allows you to apply a different color profile to an object in a PDF
 * document, and thus effectively change the colors found in that object. This process applies a set
 * of colors to an image or other object within a PDF and embeds that information in the document,
 * so that the right set of colors will be rendered when the PDF document is sent to a printer or to
 * another output device.
 *
 * Note that the color profile is not embedded by default; the user must set the embed option to true.
 *
 * Copyright (c) 2007-2025, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "ColorConvertDocument Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string s_input = Library::get_resource_directory() + "Sample_Input/ducky.pdf";
    std::string s_output = "ColorConvertDocument-out.pdf";

    if (argc > 1)
        s_input = argv[1];
    if (argc > 2)
        s_output = argv[2];

    std::cout << "Input file: " << s_input << ", writing to " << s_output << std::endl;

    Document doc(s_input);

    /* Create the list of color conversion actions to be applied to the document.
     * Each object in the document is compared against the selection criteria for each
     * of the actions until a matching action is found. Actions do not chain,
     * except in the case of aliased ink definitions.
     */
    ColorConvertActions action;

    /* In this example, make any object in the document a candidate for color conversion.
     * Also allow for any kind of Color Space. The ColorConvertObjAttrs values can be
     * combined together for more specific matching patterns using the | operator.
     * This is also true for Color Spaces.
     */
    action.set_must_match_any_attrs(ColorConvertObjAttrs::AnyObject);
    action.set_must_match_any_cs_attrs(ColorConvertSpaceType::AnySpace);
    action.set_action(ColorConvertActionType::Convert);
    action.set_convert_profile(ColorProfile::DotGain10);

    /* Once all the actions to be performed are on the list, a ColorConvertParams object is
     * created to hold them. Defaults for Render Intent and Device Color Profiles can also
     * be set here.
     */
    ColorConvertParams params(std::vector<ColorConvertActions>{action});

    bool success = doc.color_convert_pages(params);
    if (success)
        std::cout << "Color conversion succeeded." << std::endl;
    else
        std::cout << "Color conversion reported no changes." << std::endl;

    doc.save(SaveFlags::Full | SaveFlags::CollectGarbage, s_output);

    return 0;
}
