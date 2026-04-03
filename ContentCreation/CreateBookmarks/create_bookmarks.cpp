/*
 * This sample program shows how to automatically add bookmarks to a PDF file.
 * The program opens a source file called sample.pdf, adds bookmarks to it, and
 * saves an output file called Bookmark-out.pdf.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <memory>
#include <string>

using namespace datalogics_interface;

static std::unique_ptr<GoToAction> create_goto_action(Document& doc, const Rect& rect, double zoom) {
    ViewDestination view_dest(doc, 0, "XYZ", rect, zoom);
    return std::make_unique<GoToAction>(view_dest);
}

int main(int argc, char* argv[]) {
    std::cout << "CreateBookmarks Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string s_input = Library::get_resource_directory() + "Sample_Input/sample.pdf";
    std::string s_output = "Bookmark-out.pdf";

    if (argc > 1) s_input = argv[1];
    if (argc > 2) s_output = argv[2];

    std::cout << "Input file: " << s_input << ", writing to " << s_output << std::endl;

    Document doc(s_input);

    auto root_bookmark = doc.get_bookmark_root();

    Page page = doc.get_page(0);
    Rect media_box = page.get_media_box();

    // Use create_new_child() to hang a new bookmark off the root
    auto bm0 = root_bookmark->create_new_child(
        "(A) Root child, points to page 1, upper left corner, 300% zoom");
    {
        auto action = create_goto_action(doc, media_box, 3.0);
        bm0->set_action(action.get());
    }

    // Use create_new_child() off the new bookmark
    Rect rect1(media_box.ll_x, media_box.ll_y, media_box.ur_x, media_box.ur_y / 2.0);
    auto bm1 = bm0->create_new_child(
        "(B) Root child's child, points to page 1, halfway down page, 75% zoom");
    {
        auto action = create_goto_action(doc, rect1, 0.75);
        bm1->set_action(action.get());
    }

    // Use create_new_sibling() to hang a bookmark next to bm0
    Rect rect2(rect1.ll_x, rect1.ll_y, rect1.ur_x, rect1.ur_y * 0.75);
    auto bm_sibling = bm0->create_new_sibling(
        "(C) Root child's sibling, points to page 1, 1/4 from top of page, 133% zoom");
    {
        auto action = create_goto_action(doc, rect2, 1.33);
        bm_sibling->set_action(action.get());
    }

    // Move (B) to be a child of (C): unlink (B), then add it as a child of (C)
    {
        auto bm_b = root_bookmark->find_descendant_bookmark(
            "(B) Root child's child, points to page 1, halfway down page, 75% zoom");
        bm_b->unlink();

        auto bm_c = root_bookmark->find_descendant_bookmark(
            "(C) Root child's sibling, points to page 1, 1/4 from top of page, 133% zoom");
        bm_c->add_child(*bm_b);
    }

    // Move (C) (with its child (B)) as a subtree under (A)
    {
        auto bm_a = root_bookmark->find_descendant_bookmark(
            "(A) Root child, points to page 1, upper left corner, 300% zoom");
        auto bm_c = root_bookmark->find_descendant_bookmark(
            "(C) Root child's sibling, points to page 1, 1/4 from top of page, 133% zoom");
        bm_c->unlink();
        bm_a->add_subtree(*bm_c, "Bookmark formerly known as '(C) ... '");
    }

    // Create three child bookmarks off root in the order: Child 3, Child 2, Child 1.
    auto bm_child2 = root_bookmark->create_new_child("Child 2");
    auto bm_child1 = root_bookmark->create_new_child("Child 1");
    bm_child2->add_next_sibling(*bm_child1);
    auto bm_child3 = root_bookmark->create_new_child("Child 3");
    bm_child2->add_previous_sibling(*bm_child3);

    doc.save(SaveFlags::Full, s_output);

    std::cout << "Created " << s_output << std::endl;
    return 0;
}
