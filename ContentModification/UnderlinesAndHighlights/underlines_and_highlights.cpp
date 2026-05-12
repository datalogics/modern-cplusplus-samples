/*
 * This program shows how to add annotations to an existing PDF file that will highlight
 * and underline words. When you run it, the program generates a PDF output file. The output
 * sample annotates a PDF file showing a National Weather Service web page, highlighting the
 * word "Cloudy" wherever it appears and underlining the word "Rain."
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

using namespace datalogics_interface;

static std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

int main(int argc, char* argv[]) {
    std::cout << "UnderlinesAndHighlights Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string sInput = Library::get_resource_directory() + "Sample_Input/sample.pdf";
    std::string sOutput = "UnderlinesAndHighlights-out.pdf";

    if (argc > 1) sInput = argv[1];
    if (argc > 2) sOutput = argv[2];

    Document doc(sInput);

    std::cout << "Opened a document " << sInput << std::endl;

    Page docpage = doc.get_page(0);

    // Highlight occurrences of "cloudy" and underline "rain" using the WordFinder.
    std::vector<Quad> cloudyQuads;
    std::vector<Quad> rainQuads;

    WordFinderConfig wfc;
    WordFinder wf(doc, WordFinderVersion::Latest, wfc);
    std::vector<Word> words = wf.get_word_list(docpage.get_page_number());

    for (const Word& w : words) {
        std::string lower = to_lower(w.get_text());
        WordAttributeFlags attrs = w.get_attributes();
        bool hasTrailingPunct = (attrs & WordAttributeFlags::HasTrailingPunctuation) ==
                                WordAttributeFlags::HasTrailingPunctuation;

        if (lower == "cloudy" || (hasTrailingPunct && lower.find("cloudy") == 0)) {
            for (const Quad& q : w.get_quads())
                cloudyQuads.push_back(q);
        }

        if (lower == "rain" || (hasTrailingPunct && lower.find("rain") == 0)) {
            for (const Quad& q : w.get_quads())
                rainQuads.push_back(q);
        }
    }

    HighlightAnnotation highlights(docpage, cloudyQuads);
    highlights.set_color(Color(1.0, 0.75, 1.0));
    auto highlightApp = highlights.generate_appearance();
    if (highlightApp)
        highlights.set_normal_appearance(*highlightApp);

    UnderlineAnnotation underlines(docpage, rainQuads);
    underlines.set_color(Color(0.0, 0.0, 0.0));
    auto underlineApp = underlines.generate_appearance();
    if (underlineApp)
        underlines.set_normal_appearance(*underlineApp);

    // Read back the annotated text
    std::cout << "Cloudy text: " << highlights.get_annotated_text(true) << std::endl;
    std::cout << "Rainy text: " << underlines.get_annotated_text(false) << std::endl;

    doc.save(SaveFlags::Full, sOutput);
    std::cout << "Saved to " << sOutput << std::endl;

    return 0;
}
