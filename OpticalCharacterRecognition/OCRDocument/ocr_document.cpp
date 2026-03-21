/*
 * Runs OCR on the document recognizing text found on its rasterized pages.
 *
 * Copyright (c) 2007-2025, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <memory>

using namespace datalogics_interface;

// Replace all images in the content tree with OCR-processed forms containing
// invisible text under the image, making the page text-searchable.
static void recognize_content(Document& doc, Content& content, OCREngine& engine) {
    for (int index = 0; index < content.get_num_elements(); index++) {
        auto elem = content.get_element(index);
        ElementType type = elem->get_element_type();

        if (type == ElementType::Image) {
            auto* image = static_cast<Image*>(elem.get());
            auto form = engine.place_text_under(*image, doc);
            content.remove_element(index);
            content.add_element(*form, index - 1);
        } else if (type == ElementType::Container) {
            auto* container = static_cast<Container*>(elem.get());
            auto sub = container->get_content();
            recognize_content(doc, *sub, engine);
        } else if (type == ElementType::Group) {
            auto* group = static_cast<Group*>(elem.get());
            auto sub = group->get_content();
            recognize_content(doc, *sub, engine);
        } else if (type == ElementType::Form) {
            auto* form = static_cast<Form*>(elem.get());
            auto sub = form->get_content();
            recognize_content(doc, *sub, engine);
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "OCRDocument Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string s_input = Library::get_resource_directory() + "Sample_Input/scanned_images.pdf";
    std::string s_output = "OCRDocument-out.pdf";

    if (argc > 1)
        s_input = argv[1];
    if (argc > 2)
        s_output = argv[2];

    std::cout << "Input file: " << s_input << std::endl;
    std::cout << "Writing output to: " << s_output << std::endl;

    // Configure OCR parameters
    OCRParams ocr_params;
    // The languages parameter controls which languages the OCR engine attempts to detect.
    // By default the OCR engine searches for English.
    ocr_params.set_languages({OCRLanguageSetting{OCRLanguage::English, false}});

    // If the resolution for the images in your document are not 300 dpi,
    // specify a default resolution here. Specifying a correct resolution gives
    // better results for OCR, especially with automatic image preprocessing.
    // ocr_params.set_resolution(600);

    OCREngine ocr_engine(ocr_params);

    Document doc(s_input);

    for (int page_num = 0; page_num < doc.get_num_pages(); ++page_num) {
        Page page = doc.get_page(page_num);
        Content content = page.get_content();
        recognize_content(doc, content, ocr_engine);
        page.update_content();
    }

    doc.save(SaveFlags::Full, s_output);

    return 0;
}
