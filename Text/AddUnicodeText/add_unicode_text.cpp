/*
 * Copyright (c) Datalogics, Inc. All rights reserved.
 *
 * This sample program adds several lines of Unicode text to a PDF file,
 * in different languages.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <string>
#include <vector>

using namespace datalogics_interface;

// Find the first font that can represent all characters in the string.
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
    std::cout << "AddUnicodeText Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string output_path = "AddUnicodeText-out.pdf";
    if (argc > 1) output_path = argv[1];

    std::cout << "Output file: " << output_path << std::endl;

    try {
        Document doc;
        Rect page_rect(0, 0, 612, 792);
        Page docpage = doc.create_page(Document::before_first_page, page_rect);

        Text unicode_text;
        GraphicState gs;
        TextState ts;

        // Unicode strings in various languages (UTF-8 encoded via u8 literals)
        std::vector<std::string> strings = {
            u8"Chinese (Mandarin) - \u4e16\u754c\u4eba\u6743\u5ba3\u8a00",
            u8"Japanese - \u300e\u4e16\u754c\u4eba\u6a29\u5ba3\u8a00\u300f",
            u8"French - D\u00e9claration universelle des droits de l\u2019homme",
            u8"Korean - \uc138 \uacc4 \uc778 \uad8c \uc120 \uc5b8",
            u8"English - Universal Declaration of Human Rights",
            u8"Greek - \u039f\u0399\u039a\u039f\u03a5\u039c\u0395\u039d\u0399\u039a\u0397 "
            u8"\u0394\u0399\u0391\u039a\u0397\u03a1\u03a5\u039e\u0397 "
            u8"\u0393\u0399\u0391 \u03a4\u0391 "
            u8"\u0391\u039d\u0398\u03a1\u03a9\u03a0\u0399\u039d\u0391 "
            u8"\u0394\u0399\u039a\u0391\u0399\u03a9\u039c\u0391\u03a4\u0391",
            u8"Russian - \u0412\u0441\u0435\u043e\u0431\u0449\u0430\u044f "
            u8"\u0434\u0435\u043a\u043b\u0430\u0440\u0430\u0446\u0438\u044f "
            u8"\u043f\u0440\u0430\u0432 \u0447\u0435\u043b\u043e\u0432\u0435\u043a\u0430"
        };

        std::vector<Font> fonts;
        fonts.emplace_back("Arial");
        fonts.emplace_back("KozGoPr6N-Medium");
        fonts.emplace_back("AdobeMyungjoStd-Medium");

        // Place strings starting at (72, 720), moving down 18 points each line
        int x = 1 * 72;
        int y = 10 * 72;

        for (const auto& str : strings) {
            Font* font = get_representable_font(fonts, str);
            if (font == nullptr) {
                std::cout << "Couldn't find a font that can represent all characters in: " << str << std::endl;
            } else {
                Matrix m(14, 0, 0, 14, x, y);
                std::string run_text = font->get_name() + " - " + str;
                TextRun tr(run_text, *font, gs, ts, m);
                unicode_text.add_run(tr);
            }
            y -= 18;
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
