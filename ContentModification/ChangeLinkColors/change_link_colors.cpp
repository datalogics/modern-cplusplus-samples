/*
 * This sample demonstrates how to change the color of hyperlink text (usually blue).
 *
 * The program works by identifying text in a PDF file that is associated with hyperlinks.
 * Each link appears as a rectangle layer in the PDF file; ChangeLinkColors identifies these
 * rectangles, and then finds the text that lines up within these rectangles and changes the
 * color of each character that is a part of the hyperlink.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <vector>
#include <cmath>

using namespace datalogics_interface;

void find_and_process_text(Content& content, const std::vector<Rect>& link_rects);
void check_characters_in_text(Text& txt, const std::vector<Rect>& link_rects);

int main(int argc, char* argv[]) {
    std::cout << "ChangeLinkColors Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string sInput = Library::get_resource_directory() + "Sample_Input/sample_links.pdf";
    std::string sOutput = "ChangeLinkColors-out.pdf";

    if (argc > 1)
        sInput = argv[1];
    if (argc > 2)
        sOutput = argv[2];

    std::cout << "Input file: " << sInput << ", writing to " << sOutput << std::endl;

    Document doc(sInput);

    std::cout << "Opened a document." << std::endl;

    Page page = doc.get_page(0);

    // Collect link annotation rects
    std::vector<Rect> link_rects;
    int numAnnots = page.get_num_annotations();
    for (int i = 0; i < numAnnots; i++) {
        auto annot = page.get_annotation(i);
        if (annot->get_subtype() == "Link") {
            link_rects.push_back(annot->get_rect());
        }
    }

    Content content = page.get_content();
    find_and_process_text(content, link_rects);

    page.update_content();
    doc.save(SaveFlags::Full, sOutput);
    std::cout << "Saved to " << sOutput << std::endl;

    return 0;
}

void find_and_process_text(Content& content, const std::vector<Rect>& link_rects) {
    for (int i = 0; i < content.get_num_elements(); i++) {
        auto elem = content.get_element(i);

        if (auto* cont = elem->try_as<Container>()) {
            auto nested = cont->get_content();
            if (nested)
                find_and_process_text(*nested, link_rects);
        } else if (auto* form = elem->try_as<Form>()) {
            auto nested = form->get_content();
            if (nested)
                find_and_process_text(*nested, link_rects);
        } else if (auto* group = elem->try_as<Group>()) {
            auto nested = group->get_content();
            if (nested)
                find_and_process_text(*nested, link_rects);
        } else if (auto* txt = elem->try_as<Text>()) {
            std::cout << "Found a Text object." << std::endl;
            check_characters_in_text(*txt, link_rects);
        }
    }
}

void check_characters_in_text(Text& txt, const std::vector<Rect>& link_rects) {
    for (size_t i = 0; i < link_rects.size(); i++) {
        const Rect& linkRect = link_rects[i];
        int charIndex = 0;
        int numChars = txt.get_number_of_characters();

        // Find first character that intersects the link annotation rect
        for (charIndex = 0; charIndex < numChars; charIndex++) {
            if (txt.rect_intersects_character(linkRect, charIndex))
                break;
        }

        if (charIndex >= numChars)
            continue;

        // Verify using text matrix position (filter false positives)
        Matrix charMatrix = txt.get_text_matrix_for_character(charIndex);
        bool hWithin = (std::floor(linkRect.left()) < std::ceil(charMatrix.h)) &&
                       (std::floor(charMatrix.h) < std::ceil(linkRect.right()));
        bool vWithin = (std::floor(linkRect.bottom()) < std::ceil(charMatrix.v)) &&
                       (std::floor(charMatrix.v) < std::ceil(linkRect.top()));

        if (hWithin && vWithin) {
            std::cout << "Found a character that falls within the bounds of LinkAnnotation " << i << std::endl;

            int startRunIndex;
            if (charIndex == 0) {
                startRunIndex = 0;
            } else {
                txt.split_text_run_at_character(charIndex);
                startRunIndex = txt.find_text_run_index_for_character(charIndex);
            }

            // Find last character that intersects
            while (charIndex < txt.get_number_of_characters()) {
                if (!txt.rect_intersects_character(linkRect, charIndex))
                    break;
                charIndex++;
            }

            int endRunIndex;
            if (charIndex >= txt.get_number_of_characters()) {
                endRunIndex = txt.get_number_of_runs() - 1;
            } else {
                txt.split_text_run_at_character(charIndex);
                endRunIndex = txt.find_text_run_index_for_character(charIndex);
            }

            // Change the GraphicState on all TextRuns in range
            for (int runIndex = startRunIndex; runIndex <= endRunIndex; runIndex++) {
                auto txtRun = txt.get_run(runIndex);
                auto gs = txtRun->get_graphic_state();
                gs->set_fill_color(Color(0.0, 0.0, 1.0));
                txtRun->set_graphic_state(*gs);
            }
        }
    }
}
