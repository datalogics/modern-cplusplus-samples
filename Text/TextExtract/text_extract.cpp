/*
 * Copyright (c) 2007-2025, Datalogics, Inc. All rights reserved.
 *
 * This program pulls text from a PDF file and exports it to a text file.
 * It handles both tagged and untagged PDF documents.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/library.hpp>
#include <datalogics_interface/word_finder.hpp>
#include <datalogics_interface/word_finder_config.hpp>
#include <datalogics_interface/word.hpp>
#include <datalogics_interface/pdf_dict.hpp>
#include <datalogics_interface/pdf_boolean.hpp>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>

using namespace datalogics_interface;

// Split a UTF-8 string on any of the three hyphen characters:
// ASCII hyphen '-' (U+002D), Unicode soft hyphen (U+00AD), Unicode hyphen (U+2010).
// Returns the parts between hyphens.
static std::vector<std::string> split_on_hyphens(const std::string& s)
{
    std::vector<std::string> parts;
    std::string current;

    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = static_cast<unsigned char>(s[i]);

        // Check for ASCII hyphen U+002D
        if (c == '-') {
            parts.push_back(current);
            current.clear();
            ++i;
            continue;
        }

        // Check for UTF-8 encoded U+00AD (soft hyphen): 0xC2 0xAD
        if (c == 0xC2 && i + 1 < s.size() &&
            static_cast<unsigned char>(s[i + 1]) == 0xAD) {
            parts.push_back(current);
            current.clear();
            i += 2;
            continue;
        }

        // Check for UTF-8 encoded U+2010 (hyphen): 0xE2 0x80 0x90
        if (c == 0xE2 && i + 2 < s.size() &&
            static_cast<unsigned char>(s[i + 1]) == 0x80 &&
            static_cast<unsigned char>(s[i + 2]) == 0x90) {
            parts.push_back(current);
            current.clear();
            i += 3;
            continue;
        }

        // Regular character (possibly multi-byte UTF-8)
        if (c < 0x80) {
            current += s[i];
            ++i;
        } else if ((c & 0xE0) == 0xC0) {
            current += s.substr(i, 2);
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            current += s.substr(i, 3);
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            current += s.substr(i, 4);
            i += 4;
        } else {
            current += s[i];
            ++i;
        }
    }
    parts.push_back(current);
    return parts;
}

static void extract_text_untagged(Document& doc, WordFinder& word_finder)
{
    int n_pages = doc.get_num_pages();
    std::ofstream logfile("TextExtract-untagged-out.txt");
    std::cout << "Writing TextExtract-untagged-out.txt" << std::endl;

    for (int i = 0; i < n_pages; ++i) {
        auto page_words = word_finder.get_word_list(i);
        std::string text_to_extract;

        for (const auto& word : page_words) {
            std::string s = word.get_text();

            // Check for hyphenated words that break across a line
            if ((word.get_attributes() & WordAttributeFlags::HasSoftHyphen) == WordAttributeFlags::HasSoftHyphen &&
                (word.get_attributes() & WordAttributeFlags::LastWordOnLine)  == WordAttributeFlags::LastWordOnLine)
            {
                // Remove the hyphen and combine the two parts of the word.
                // Note we handle ascii hyphen '-', Unicode soft hyphen U+00AD,
                // and Unicode hyphen U+2010.
                auto parts = split_on_hyphens(s);
                if (parts.size() >= 2) {
                    text_to_extract += parts[0] + parts[1];
                } else {
                    text_to_extract += s;
                }
            } else {
                text_to_extract += s;
            }

            if ((word.get_attributes() & WordAttributeFlags::AdjacentToSpace) == WordAttributeFlags::AdjacentToSpace) {
                text_to_extract += " ";
            }

            if ((word.get_attributes() & WordAttributeFlags::LastWordOnLine) == WordAttributeFlags::LastWordOnLine) {
                text_to_extract += "\n";
            }
        }

        logfile << "<page " << (i + 1) << ">\n";
        logfile << text_to_extract << "\n";
    }

    std::cout << "Extracted " << n_pages << " pages." << std::endl;
    logfile.close();
}

static void extract_text_tagged(Document& doc, WordFinder& word_finder)
{
    int n_pages = doc.get_num_pages();
    std::ofstream logfile("TextExtract-tagged-out.txt");
    std::cout << "Writing TextExtract-tagged-out.txt" << std::endl;

    for (int i = 0; i < n_pages; ++i) {
        auto page_words = word_finder.get_word_list(i);
        std::string text_to_extract;

        for (const auto& word : page_words) {
            std::string s = word.get_text();

            // In tagged PDFs, soft hyphens only break words across lines.
            // Note we handle Unicode soft hyphen U+00AD and Unicode hyphen U+2010
            // (no ASCII hyphen for tagged -- matching the C# reference).
            if ((word.get_attributes() & WordAttributeFlags::HasSoftHyphen) == WordAttributeFlags::HasSoftHyphen) {
                auto parts = split_on_hyphens(s);
                if (parts.size() >= 2) {
                    text_to_extract += parts[0] + parts[1];
                } else {
                    text_to_extract += s;
                }
            } else {
                text_to_extract += s;
            }

            if ((word.get_attributes() & WordAttributeFlags::AdjacentToSpace) == WordAttributeFlags::AdjacentToSpace) {
                text_to_extract += " ";
            }

            if (word.get_is_last_word_in_region()) {
                text_to_extract += "\n";
            }
        }

        logfile << "<page " << (i + 1) << ">\n";
        logfile << text_to_extract << "\n";
    }

    std::cout << "Extracted " << n_pages << " pages." << std::endl;
    logfile.close();
}

int main(int argc, char* argv[])
{
    std::cout << "TextExtract Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    // This is a tagged PDF.
    std::string input_path = Library::get_resource_directory() + "Sample_Input/pdf_intro.pdf";
    if (argc > 1) input_path = argv[1];

    std::cout << "Input file:  " << input_path << std::endl;

    try {
        Document doc(input_path);

        // Determine if the PDF is tagged by examining the MarkInfo dictionary.
        bool doc_is_tagged = false;
        auto root = doc.get_root();
        if (root) {
            auto mark_info_entry = root->get("MarkInfo");
            if (mark_info_entry) {
                auto* mark_info_dict = dynamic_cast<PDFDict*>(mark_info_entry.get());
                if (mark_info_dict) {
                    auto marked_entry = mark_info_dict->get("Marked");
                    if (marked_entry) {
                        auto* marked_bool = dynamic_cast<PDFBoolean*>(marked_entry.get());
                        if (marked_bool && marked_bool->get_value()) {
                            doc_is_tagged = true;
                        }
                    }
                }
            }
        }

        WordFinderConfig word_config;
        word_config.set_ignore_char_gaps(false);
        word_config.set_ignore_line_gaps(false);
        word_config.set_no_annotations(false);
        word_config.set_no_encoding_guess(false);
        word_config.set_unknown_to_std_enc(false);
        word_config.set_disable_tagged_pdf(false);
        word_config.set_no_xy_sort(true);
        word_config.set_preserve_spaces(false);
        word_config.set_no_ligature_expansion(false);
        word_config.set_no_hyphen_detection(false);
        word_config.set_trust_nb_space(false);
        word_config.set_no_ext_char_offset(false);
        word_config.set_no_style_info(false);

        WordFinder word_finder(doc, WordFinderVersion::Latest, word_config);

        if (doc_is_tagged)
            extract_text_tagged(doc, word_finder);
        else
            extract_text_untagged(doc, word_finder);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
