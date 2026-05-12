/*
 * Copyright (c) Datalogics, Inc. All rights reserved.
 *
 * This program describes how to render text from top to bottom on a page.
 * It provides WritingMode::Vertical with Unicode characters to present sample text
 * in English, Mandarin, Japanese, and Korean.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <string>
#include <vector>

using namespace datalogics_interface;

Font* get_representable_font(std::vector<Font>& fonts, const std::string& str)
{
    for (auto& font : fonts) {
        if (font.is_text_representable(str))
            return &font;
    }
    return nullptr;
}

int main(int argc, char* argv[])
{
    std::cout << "AddVerticalText Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string output_path = "AddVerticalText-out.pdf";
    if (argc > 1) output_path = argv[1];

    std::cout << "Output file: " << output_path << std::endl;

    try {
        Document doc;
        Rect page_rect(0, 0, 612, 792);
        Page docpage = doc.create_page(Document::before_first_page, page_rect);

        Text unicode_text;
        GraphicState gs;
        TextState ts;

        std::vector<std::string> strings = {
            u8"Universal Declaration of Human Rights",
            u8"\u4e16\u754c\u4eba\u6743\u5ba3\u8a00",
            u8"\u300e\u4e16\u754c\u4eba\u6a29\u5ba3\u8a00\u300f",
            u8"\uc138 \uacc4 \uc778 \uad8c \uc120 \uc5b8"
        };

        // Create fonts with Vertical writing mode
        // FontCreateFlags value 0 (no special flags)
        std::vector<Font> fonts;
        fonts.emplace_back("KozGoPr6N-Medium",
                           static_cast<FontCreateFlags>(0),
                           WritingMode::Vertical);
        fonts.emplace_back("AdobeMyungjoStd-Medium",
                           static_cast<FontCreateFlags>(0),
                           WritingMode::Vertical);

        int x = 1 * 72;
        int y = 10 * 72;

        for (const auto& str : strings) {
            Font* font = get_representable_font(fonts, str);
            if (font == nullptr) {
                std::cout << "Couldn't find a font that can represent all characters in: " << str << std::endl;
            } else {
                Matrix m(14, 0, 0, 14, x, y);
                TextRun tr(str, *font, gs, ts, m);
                unicode_text.add_run(tr);
            }
            // Move across the page to the right for the next column
            x += 30;
        }

        Content content = docpage.get_content();
        content.add_element(unicode_text);
        docpage.update_content();

        std::cout << "Embedding fonts." << std::endl;
        doc.embed_fonts();
        doc.save(SaveFlags::Full, output_path);

        std::cout << "Saved output to " << output_path << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
