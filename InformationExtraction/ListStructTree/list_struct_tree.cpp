/*
 * This sample walks the logical structure tree of a tagged PDF document
 * and prints each element's struct type (/S), parent chain, and kid entries
 * (child elements and marked-content references).
 *
 * With no arguments the sample builds a small tagged document in memory
 * (Document -> [P with MCID 0, Figure with /ID "fig-1" and /Alt]) and walks
 * that. Pass a path to a tagged PDF to walk any existing document instead.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

using namespace datalogics_interface;

namespace {

std::string indent(int depth)
{
    return std::string(static_cast<size_t>(depth) * 2, ' ');
}

// Recursively walk a struct element, printing its type, any accessibility
// attributes worth surfacing, and every kid (element or MCR). OBJR and
// unknown kid types are reported but not recursed into.
void walk_element(const StructElement& elem, int depth)
{
    std::cout << indent(depth) << "<" << elem.get_type() << ">";

    // Surface a handful of commonly-set attributes via the escape hatch.
    auto dict = elem.get_pdf_dict();
    auto print_string_attr = [&](const char* key, const char* label) {
        auto val = dict.get(key);
        if (!val) return;
        if (auto* str = val->try_as<PDFString>()) {
            std::cout << " " << label << "=\"" << str->get_value() << "\"";
        }
    };
    print_string_attr("ID", "id");
    print_string_attr("Alt", "alt");

    const size_t kid_count = elem.get_kid_count();
    std::cout << " kids=" << kid_count << std::endl;

    for (size_t i = 0; i < kid_count; ++i) {
        switch (elem.get_kid_type(i)) {
        case StructKidType::Element: {
            StructElement child = elem.get_kid_as_element(i);
            walk_element(child, depth + 1);
            break;
        }
        case StructKidType::MarkedContent: {
            const int mcid = elem.get_kid_as_marked_content_mcid(i);
            Page page = elem.get_kid_as_marked_content_page(i);
            std::cout << indent(depth + 1)
                      << "<MCR mcid=" << mcid
                      << " page=" << page.get_page_number() << ">"
                      << std::endl;
            break;
        }
        case StructKidType::ObjectRef:
            std::cout << indent(depth + 1) << "<OBJR (not traversed)>"
                      << std::endl;
            break;
        case StructKidType::Unknown:
            std::cout << indent(depth + 1) << "<Unknown kid>" << std::endl;
            break;
        }
    }
}

// Produce a small tagged PDF so the sample works without an input file.
// The document mirrors the minimum UA-ish shape: catalog /MarkInfo +
// /StructTreeRoot, one page with a tagged paragraph and a figure with
// /Alt text and an /ID the sample can look up via find_element_by_id.
void build_sample_tagged_pdf(std::ostream& out)
{
    Document doc;
    Page page = doc.create_page(Document::before_first_page,
                                Rect{0.0, 0.0, 612.0, 792.0});

    StructTreeRoot root = doc.create_struct_tree_root();
    StructElement document_elem = root.add_child("Document");
    StructElement paragraph = document_elem.add_child("P");
    StructElement figure = document_elem.add_child("Figure");

    // Tag paragraph content. PDSEdit assigns the MCID; the property dict
    // must be empty so MCID is not double-written.
    PDFDict props(doc, /*indirect=*/false);
    Container mc("P", props, /*is_inline=*/false);
    Content content = page.get_content();
    content.add_element(mc);
    paragraph.add_marked_content_ref(page, mc);
    page.update_content();

    // Accessibility: /Alt on Figure and /ID for find-by-id lookup.
    PDFString alt("A photo of a duck", doc, /*indirect=*/false);
    figure.get_pdf_dict().put("Alt", alt);

    PDFString fig_id("fig-1", doc, /*indirect=*/false);
    figure.get_pdf_dict().put("ID", fig_id);

    // PDSTreeRootGetElementFromID resolves /ID through the struct tree's
    // /IDTree name-tree. Wire a single leaf by hand using the dict escape
    // hatch: { /Names [ "fig-1" figureRef ] }.
    PDFString fig_id_for_tree("fig-1", doc, /*indirect=*/false);
    PDFArray names(doc, /*indirect=*/false);
    names.add(fig_id_for_tree);
    PDFObject figure_ref = figure.as_pdf_object();
    names.add(figure_ref);
    PDFDict id_tree(doc, /*indirect=*/true);
    id_tree.put("Names", names);
    root.get_pdf_dict().put("IDTree", id_tree);

    // UA-2 requires a natural-language hint on the catalog.
    PDFString lang("en-US", doc, /*indirect=*/false);
    doc.get_root()->put("Lang", lang);

    doc.save(SaveFlags::Full, out);
}

}  // namespace

int main(int argc, char* argv[])
{
    std::cout << "ListStructTree Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    // Either walk an input file or synthesize one for the demo.
    std::unique_ptr<Document> doc;
    std::stringstream storage(std::ios::in | std::ios::out | std::ios::binary);
    try {
        if (argc > 1) {
            const std::string input_path = argv[1];
            std::cout << "Input file: " << input_path << std::endl;
            doc = std::make_unique<Document>(input_path);
        } else {
            std::cout << "No input file — producing an in-memory tagged PDF."
                      << std::endl;
            build_sample_tagged_pdf(storage);
            doc = std::make_unique<Document>(storage);
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to open document: " << e.what() << std::endl;
        return 1;
    }

    try {
        auto root = doc->get_struct_tree_root();
        if (!root.has_value()) {
            std::cout << "Document has no structure tree — nothing to walk."
                      << std::endl;
            return 0;
        }

        // Tree-root kids are always struct elements per the PDF spec.
        const size_t top_level = root->get_kid_count();
        std::cout << "StructTreeRoot: " << top_level << " top-level "
                  << (top_level == 1 ? "kid" : "kids") << std::endl;

        for (size_t i = 0; i < top_level; ++i) {
            StructElement kid = root->get_kid(i);
            walk_element(kid, 1);

            // Demonstrate the parent-navigation API on the first leaf we
            // find: walk down the left spine until we hit a non-element
            // kid, then report its parent chain.
            StructElement cursor = root->get_kid(i);
            while (cursor.get_kid_count() > 0 &&
                   cursor.get_kid_type(0) == StructKidType::Element) {
                cursor = cursor.get_kid_as_element(0);
            }
            std::cout << "Parent of deepest <" << cursor.get_type() << ">: ";
            if (cursor.parent_is_tree_root()) {
                std::cout << "StructTreeRoot" << std::endl;
            } else {
                auto parent = cursor.get_parent();
                std::cout << "<" << (parent ? parent->get_type() : std::string("?"))
                          << ">" << std::endl;
            }
        }

        // Demonstrate /IDTree-driven lookup. Succeeds when the producer
        // registered the id in the tree's /IDTree; otherwise reports
        // nullopt, which is also a valid outcome for untagged-by-id docs.
        const std::string probe_id = (argc > 2) ? argv[2] : "fig-1";
        auto found = root->find_element_by_id(probe_id);
        if (found.has_value()) {
            std::cout << "find_element_by_id(\"" << probe_id << "\") -> <"
                      << found->get_type() << ">" << std::endl;
        } else {
            std::cout << "find_element_by_id(\"" << probe_id
                      << "\") -> not found" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error walking struct tree: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
