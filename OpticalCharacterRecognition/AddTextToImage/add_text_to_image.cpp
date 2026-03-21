/*
 * The sample uses an image as input which will be processed by the optical recognition engine.
 * We will then place the image and the processed text in an output pdf.
 *
 * Copyright (c) 2007-2025, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <memory>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "AddTextToImage Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string s_input = Library::get_resource_directory() + "Sample_Input/text_as_image.jpg";
    std::string s_output = "AddTextToImage-out.pdf";

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

    // If your image resolution is not 300 dpi, specify it here. Specifying a
    // correct resolution gives better results for OCR, especially with
    // automatic image preprocessing.
    // ocr_params.set_resolution(600);

    OCREngine ocr_engine(ocr_params);

    Document doc;

    {
        // Create an Image from the input file and add it to a new page.
        Image new_image(s_input, doc);

        // Create a PDF page which is the size of the image.
        // Matrix.A and Matrix.D represent the width and height in PDF user space units.
        // There are 72 PDF user space units in one inch.
        Matrix img_matrix = new_image.get_matrix();
        Rect page_rect(0.0, 0.0, img_matrix.a, img_matrix.d);
        Page doc_page = doc.create_page(Document::before_first_page, page_rect);

        Content page_content = doc_page.get_content();
        page_content.add_element(new_image);
        doc_page.update_content();
    }

    {
        Page page = doc.get_page(0);
        Content content = page.get_content();
        auto elem = content.get_element(0);
        auto* image = static_cast<Image*>(elem.get());

        // place_text_under creates a form with the image and generated text underneath.
        // The original image in the page is then replaced by the form.
        auto form = ocr_engine.place_text_under(*image, doc);
        content.remove_element(0);
        content.add_element(*form, Content::before_first);
        page.update_content();
    }

    doc.save(SaveFlags::Full, s_output);

    return 0;
}
