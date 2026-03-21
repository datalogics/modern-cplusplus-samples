/*
 * This sample shows how to redact a PDF document. The program opens an input PDF, searches for
 * specific words using the Adobe PDF Library WordFinder, and then removes these words from the text.
 *
 * Copyright (c) 2007-2024, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "Redactions Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/sample.pdf";
    std::string output1    = "Redactions-out.pdf";
    std::string output2    = "Redactions-out-applied.pdf";

    if (argc > 1) input_path = argv[1];

    std::cout << "Input file: " << input_path << std::endl;

    try {
        Document doc(input_path);

        Page docpage = doc.get_page(0);

        std::vector<Quad> cloudy_quads;
        std::vector<Quad> rain_quads;

        // Use WordFinder with WordFinderConfig to find words
        WordFinderConfig word_config;
        WordFinder wf(doc, WordFinderVersion::Latest, word_config);

        auto words = wf.get_word_list(docpage.get_page_number());

        for (const auto& w : words) {
            std::string text = w.get_text();
            // Convert to lowercase for comparison
            std::string lower = text;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            std::cout << " " << lower << std::endl;

            // Collect quads for "cloudy" words
            bool has_trailing = (w.get_attributes() & WordAttributeFlags::HasTrailingPunctuation)
                                != static_cast<WordAttributeFlags>(0);
            if (lower == "cloudy" || (has_trailing && lower.substr(0, 6) == "cloudy")) {
                auto q = w.get_quads();
                cloudy_quads.insert(cloudy_quads.end(), q.begin(), q.end());
            }

            // Collect quads for "rain" words
            if (lower == "rain" || (has_trailing && lower.substr(0, 4) == "rain")) {
                auto q = w.get_quads();
                rain_quads.insert(rain_quads.end(), q.begin(), q.end());
            }
        }

        std::cout << "Found Cloudy instances: " << cloudy_quads.size() << std::endl;

        Color red{1.0, 0.0, 0.0};
        Color white{1.0};
        Color green{0.0, 1.0, 0.0};

        // Redact "cloudy" occurrences with a red fill
        if (!cloudy_quads.empty()) {
            Redaction not_cloudy(docpage, cloudy_quads);
            not_cloudy.set_fill_normal(true);
            not_cloudy.set_fill_color(red, 0.25);
        }

        std::cout << "Found rain instances: " << rain_quads.size() << std::endl;

        // Redact "rain" occurrences with overlay text
        if (!rain_quads.empty()) {
            Redaction no_rain(docpage, rain_quads);
            no_rain.set_internal_color(green);
            no_rain.set_overlay_text("rain");
            no_rain.set_repeat(true);
            no_rain.set_scale_to_fit(true);
            no_rain.set_text_color(white);
            no_rain.set_font_face("CourierStd");
            no_rain.set_font_size(8.0);
        }

        doc.save(SaveFlags::Full, output1);
        std::cout << "Wrote a pdf doc with unapplied redactions." << std::endl;

        // Apply all the redactions in the document
        doc.apply_redactions();

        doc.save(SaveFlags::Full, output2);
        std::cout << "Wrote a redacted pdf doc." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
