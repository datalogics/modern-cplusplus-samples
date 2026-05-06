#pragma once

/*
 * ===============================================================================
 * This header is intended to assist with operations common to text extraction
 * samples. It contains functions to control types of words found and what
 * information is returned to the user.
 * ===============================================================================
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 *
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <memory>
#include <string>
#include <vector>

using namespace datalogics_interface;

namespace dl_common {

// ── Data structures ──────────────────────────────────────────────────────

/// Represents extracted text from a page.
struct TextObject {
    std::string text;
};

/// Represents a color space with a name and component count.
struct DLColorSpace {
    std::string name;
    int num_components = 0;
};

/// Represents a color with component values and an associated color space.
struct DLColor {
    std::vector<double> value;
    DLColorSpace space;
};

/// Represents a text style (font name, size, and color).
struct DLStyle {
    std::string font_name;
    double font_size = 0.0;
    DLColor color;
};

/// Represents a style change at a character index within a word.
struct DLStyleTransition {
    int char_index = 0;
    DLStyle style;
};

/// Represents extracted text with position and style details.
struct TextAndDetailsObject {
    std::string text;
    std::vector<Quad> quads;
    std::vector<DLStyleTransition> style_list;
    std::vector<Quad> char_quads;
};

/// Represents an AcroForm text field's name and value.
struct AcroFormTextFieldObject {
    std::string field_name;
    std::string field_text;
};

/// Represents annotation text with its annotation type.
struct AnnotationTextObject {
    std::string annotation_type;
    std::string annotation_text;
};

// ── Helper: create a default WordFinderConfig ────────────────────────────

/// Creates a WordFinderConfig with default settings matching the C# sample.
/// The C# WordFinderConfig constructor uses all-default (false) values,
/// which is what our default constructor already provides.
inline WordFinderConfig make_default_word_finder_config() {
    WordFinderConfig config;
    return config;
}

// ── Text extraction functions ────────────────────────────────────────────

/// Gets the text on a specified page (0-based page number).
inline std::vector<TextObject> get_text(Document& doc, int page_num) {
    std::vector<TextObject> page_text;

    auto config = make_default_word_finder_config();
    WordFinder word_finder(doc, WordFinderVersion::Latest, config);
    auto words = word_finder.get_word_list(page_num);

    for (const auto& word : words) {
        TextObject obj;
        obj.text = word.get_text();
        page_text.push_back(std::move(obj));
    }

    return page_text;
}

/// Gets the text for the entire document.
inline std::vector<TextObject> get_text(Document& doc) {
    std::vector<TextObject> result;

    for (int page_num = 0; page_num < doc.get_num_pages(); ++page_num) {
        auto page_text = get_text(doc, page_num);
        result.insert(result.end(),
                      std::make_move_iterator(page_text.begin()),
                      std::make_move_iterator(page_text.end()));
    }

    return result;
}

/// Gets the text and detail info (quads, styles) for a specific page.
inline std::vector<TextAndDetailsObject> get_text_and_details(Document& doc, int page_num) {
    std::vector<TextAndDetailsObject> result;

    auto config = make_default_word_finder_config();
    WordFinder word_finder(doc, WordFinderVersion::Latest, config);
    auto words = word_finder.get_word_list(page_num);

    for (const auto& word : words) {
        TextAndDetailsObject obj;
        obj.text = word.get_text();
        obj.char_quads = word.get_char_quads();
        obj.quads = word.get_quads();

        // Extract style transitions
        auto transitions = word.get_style_transitions();
        for (const auto& st : transitions) {
            DLStyleTransition dl_trans;
            dl_trans.char_index = st.char_index;

            DLStyle dl_style;
            dl_style.font_size = st.style.get_font_size();
            dl_style.font_name = st.style.get_font_name();

            // Extract color information
            auto color_ptr = st.style.get_color();
            if (color_ptr) {
                dl_style.color.value = color_ptr->get_value();
                auto space_ptr = color_ptr->get_space();
                if (space_ptr) {
                    dl_style.color.space.name = space_ptr->get_name();
                    dl_style.color.space.num_components = space_ptr->get_num_components();
                }
            }

            dl_trans.style = std::move(dl_style);
            obj.style_list.push_back(std::move(dl_trans));
        }

        result.push_back(std::move(obj));
    }

    return result;
}

// ── AcroForm field extraction ────────────────────────────────────────────

namespace detail {

/// Gets the text value from a form field dictionary.
inline std::string get_acro_form_field_text(PDFDict& field) {
    auto entry = field.get("V");
    if (!entry) return "";
    auto* str_ptr = dynamic_cast<PDFString*>(entry.get());
    if (str_ptr) {
        return str_ptr->get_value();
    }
    return "";
}

/// Recursively enumerates AcroForm fields and collects text field data.
inline void enumerate_acro_form_field(PDFObject& field_entry,
                                      const std::string& prefix,
                                      std::vector<AcroFormTextFieldObject>& result) {
    auto* field = dynamic_cast<PDFDict*>(&field_entry);
    if (!field) return;

    // Get the field's partial name ("T" entry)
    auto t_entry = field->get("T");
    if (!t_entry) return;
    auto* t_str = dynamic_cast<PDFString*>(t_entry.get());
    if (!t_str) return;

    std::string name_part = t_str->get_value();
    std::string field_name = prefix.empty() ? name_part : prefix + "." + name_part;

    // Recursively handle "Kids"
    auto kids_entry = field->get("Kids");
    if (kids_entry) {
        auto* kids = dynamic_cast<PDFArray*>(kids_entry.get());
        if (kids) {
            for (int i = 0; i < kids->get_length(); ++i) {
                auto kid_entry = kids->get(i);
                if (kid_entry) {
                    enumerate_acro_form_field(*kid_entry, field_name, result);
                }
            }
        }
    }

    // Check if this is a text field ("FT" == "Tx")
    auto ft_entry = field->get("FT");
    if (ft_entry) {
        auto* ft_name = dynamic_cast<PDFName*>(ft_entry.get());
        if (ft_name && ft_name->get_value() == "Tx") {
            AcroFormTextFieldObject obj;
            obj.field_name = field_name;
            obj.field_text = get_acro_form_field_text(*field);
            result.push_back(std::move(obj));
        }
    }
}

}  // namespace detail

/// Gets the AcroForm text field data from the document.
inline std::vector<AcroFormTextFieldObject> get_acro_form_field_data(Document& doc) {
    std::vector<AcroFormTextFieldObject> result;

    auto root = doc.get_root();
    if (!root) return result;

    auto form_entry = root->get("AcroForm");
    if (!form_entry) return result;

    auto* form_root = dynamic_cast<PDFDict*>(form_entry.get());
    if (!form_root) return result;

    auto fields_entry = form_root->get("Fields");
    if (!fields_entry) return result;

    auto* fields = dynamic_cast<PDFArray*>(fields_entry.get());
    if (!fields) return result;

    for (int i = 0; i < fields->get_length(); ++i) {
        auto field_entry = fields->get(i);
        if (field_entry) {
            detail::enumerate_acro_form_field(*field_entry, "", result);
        }
    }

    return result;
}

// ── Annotation text extraction ───────────────────────────────────────────

/// Gets the annotation text on a specified page (0-based page number).
inline std::vector<AnnotationTextObject> get_annotation_text(Document& doc, int page_num) {
    std::vector<AnnotationTextObject> result;

    auto page = doc.get_page(page_num);
    for (int annot_num = 0; annot_num < page.get_num_annotations(); ++annot_num) {
        auto annot = page.get_annotation(annot_num);
        if (!annot) continue;

        std::string subtype = annot->get_subtype();
        if (subtype == "Text" || subtype == "FreeText") {
            AnnotationTextObject obj;
            obj.annotation_type = subtype;
            obj.annotation_text = annot->get_contents();
            result.push_back(std::move(obj));
        }
    }

    return result;
}

/// Gets the annotation text for the entire document.
inline std::vector<AnnotationTextObject> get_annotation_text(Document& doc) {
    std::vector<AnnotationTextObject> result;

    for (int page_num = 0; page_num < doc.get_num_pages(); ++page_num) {
        auto page_text = get_annotation_text(doc, page_num);
        result.insert(result.end(),
                      std::make_move_iterator(page_text.begin()),
                      std::make_move_iterator(page_text.end()));
    }

    return result;
}

}  // namespace dl_common
