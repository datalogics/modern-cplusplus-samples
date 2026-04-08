/*
 * The Graphics State is an internal data structure in a PDF file that holds the parameters
 * that describe graphics within that file. The Extended Graphic State expands the original
 * Graphics State, providing space to define and store more data objects within a PDF.
 *
 * This sample program shows how to use the Extended Graphic State object to add graphics
 * parameters to an image, demonstrating all 16 blend modes.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <datalogics_interface/library.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/page.hpp>
#include <datalogics_interface/content.hpp>
#include <datalogics_interface/image.hpp>
#include <datalogics_interface/text.hpp>
#include <datalogics_interface/text_run.hpp>
#include <datalogics_interface/text_state.hpp>
#include <datalogics_interface/graphic_state.hpp>
#include <datalogics_interface/extended_graphic_state.hpp>
#include <datalogics_interface/font.hpp>
#include <datalogics_interface/color.hpp>
#include <datalogics_interface/geometry.hpp>
#include <iostream>

using namespace datalogics_interface;

struct BlendModeEntry {
    BlendMode mode;
    const char* name;
};

static const BlendModeEntry kBlendModes[] = {
    { BlendMode::Normal,     "Normal"      },
    { BlendMode::Multiply,   "Multiply"    },
    { BlendMode::Screen,     "Screen"      },
    { BlendMode::Overlay,    "Overlay"     },
    { BlendMode::Darken,     "Darken"      },
    { BlendMode::Lighten,    "Lighten"     },
    { BlendMode::ColorDodge, "Color Dodge" },
    { BlendMode::ColorBurn,  "Color Burn"  },
    { BlendMode::HardLight,  "Hard Light"  },
    { BlendMode::SoftLight,  "SoftLight"   },
    { BlendMode::Difference, "Difference"  },
    { BlendMode::Exclusion,  "Exclusion"   },
    { BlendMode::Hue,        "Hue"         },
    { BlendMode::Saturation, "Saturation"  },
    { BlendMode::Color,      "Color"       },
    { BlendMode::Luminosity, "Luminosity"  },
};

static void blend_page(Document& doc, Image& foregroundImage, Image& backgroundImage) {
    const double height = 792.0;
    const double width  = 612.0;
    Rect pageRect(0, 0, width, height);
    Page docpage = doc.create_page(doc.get_num_pages() - 1, pageRect);

    Font f("Arial", FontCreateFlags::Embedded | FontCreateFlags::Subset);

    GraphicState gsText;
    gsText.set_fill_color(Color(0.0, 0.0, 1.0));
    TextState ts;

    double spaceFactor = 18.0;
    double heightOffset = height - 88.0;

    for (int i = 0; i < 16; i++) {
        auto indFG = foregroundImage.clone();
        auto indBG = backgroundImage.clone();

        spaceFactor = (i == 0) ? 0.0 : 18.0;

        indFG->scale(0.125, 0.125);
        indBG->scale(0.125, 0.125);

        if (i > 7) {
            indFG->translate(400, heightOffset - (72.0 + spaceFactor) * (i - 8));
            indBG->translate(400, heightOffset - (72.0 + spaceFactor) * (i - 8));
        } else {
            indFG->translate(100, heightOffset - (72.0 + spaceFactor) * i);
            indBG->translate(100, heightOffset - (72.0 + spaceFactor) * i);
        }

        // Apply extended graphic state blend mode to foreground image
        GraphicState gs = indFG->get_graphic_state();
        ExtendedGraphicState xgs;
        xgs.set_blend_mode(kBlendModes[i].mode);
        gs.set_extended_graphic_state(&xgs);
        indFG->set_graphic_state(std::move(gs));
        std::cout << "Set blend mode: " << kBlendModes[i].name << std::endl;

        Content content = docpage.get_content();
        content.add_element(*indBG);
        std::cout << "Added background image " << (i + 1) << " to the content." << std::endl;
        content.add_element(*indFG);
        std::cout << "Added foreground image " << (i + 1) << " to the content." << std::endl;

        Matrix m;
        if (i > 7)
            m = m.translate(480, heightOffset - (72.0 + spaceFactor) * (i - 8));
        else
            m = m.translate(180, heightOffset - (72.0 + spaceFactor) * i);
        m = m.scale(12.0, 12.0);

        Text t;
        TextRun tr(kBlendModes[i].name, f, gsText, ts, m);
        t.add_run(tr);
        content.add_element(t);

        docpage.update_content();
        std::cout << "Updated the content on page." << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "ExtendedGraphicStates Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string sInput1 = Library::get_resource_directory() + "Sample_Input/ducky_alpha.tif";
    std::string sInput2 = Library::get_resource_directory() + "Sample_Input/rainbow.tif";
    std::string sOutput  = "ExtendedGraphicStates-out.pdf";

    if (argc > 1) sInput1 = argv[1];
    if (argc > 2) sInput2 = argv[2];
    if (argc > 3) sOutput  = argv[3];

    std::cout << "Input files: " << sInput1 << " and " << sInput2
              << ". Saving to output file: " << sOutput << std::endl;

    Document doc;

    Image imageOne(sInput1, doc);
    Image imageTwo(sInput2, doc);

    blend_page(doc, imageOne, imageTwo);
    blend_page(doc, imageTwo, imageOne);

    doc.embed_fonts();
    doc.save(SaveFlags::Full, sOutput);
    std::cout << "Saved to " << sOutput << std::endl;

    return 0;
}
