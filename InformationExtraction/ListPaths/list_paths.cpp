/*
 * This sample searches for and lists the contents of paths found in an existing PDF document.
 * Paths in PDF documents, or clipping paths, define the boundaries for art or graphics.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

static void list_path(const Path& path, int pgno)
{
    auto segments = path.get_segments();
    std::cout << "Path on page " << pgno << ":" << std::endl;
    std::cout << "Transformation matrix: " << path.get_matrix().to_string() << std::endl;

    for (const auto& seg : segments) {
        if (const auto* m = dynamic_cast<const MoveTo*>(seg.get())) {
            std::cout << "  MoveTo x=" << m->point().h
                      << ", y=" << m->point().v << std::endl;
        } else if (const auto* l = dynamic_cast<const LineTo*>(seg.get())) {
            std::cout << "  LineTo x=" << l->point().h
                      << ", y=" << l->point().v << std::endl;
        } else if (const auto* c = dynamic_cast<const CurveTo*>(seg.get())) {
            std::cout << "  CurveTo x1=" << c->point1().h << ", y1=" << c->point1().v
                      << ", x2=" << c->point2().h << ", y2=" << c->point2().v
                      << ", x3=" << c->point3().h << ", y3=" << c->point3().v << std::endl;
        } else if (const auto* cv = dynamic_cast<const CurveToV*>(seg.get())) {
            std::cout << "  CurveToV x2=" << cv->point2().h << ", y2=" << cv->point2().v
                      << ", x3=" << cv->point3().h << ", y3=" << cv->point3().v << std::endl;
        } else if (const auto* cy = dynamic_cast<const CurveToY*>(seg.get())) {
            std::cout << "  CurveToY x1=" << cy->point1().h << ", y1=" << cy->point1().v
                      << ", x3=" << cy->point3().h << ", y3=" << cy->point3().v << std::endl;
        } else if (const auto* r = dynamic_cast<const RectSegment*>(seg.get())) {
            std::cout << "  Rectangle x=" << r->point().h << ", y=" << r->point().v
                      << ", width=" << r->width() << ", height=" << r->height() << std::endl;
        } else if (dynamic_cast<const ClosePathSegment*>(seg.get())) {
            std::cout << "  ClosePath" << std::endl;
        }
    }
}

static void list_paths_in_content(const Content& content, int pgno);

static void list_paths_in_content(Content& content, int pgno)
{
    for (int i = 0; i < content.get_num_elements(); ++i) {
        auto elem = content.get_element(i);
        if (!elem) continue;

        switch (elem->get_element_type()) {
        case ElementType::Path:
            list_path(static_cast<const Path&>(*elem), pgno);
            break;
        case ElementType::Container:
        {
            std::cout << "Recurring through a Container" << std::endl;
            auto& c = static_cast<Container&>(*elem);
            auto sub = c.get_content();
            if (sub) list_paths_in_content(*sub, pgno);
            break;
        }
        case ElementType::Group:
        {
            std::cout << "Recurring through a Group" << std::endl;
            auto& g = static_cast<Group&>(*elem);
            auto sub = g.get_content();
            if (sub) list_paths_in_content(*sub, pgno);
            break;
        }
        case ElementType::Form:
        {
            std::cout << "Recurring through a Form" << std::endl;
            auto& f = static_cast<Form&>(*elem);
            auto sub = f.get_content();
            if (sub) list_paths_in_content(*sub, pgno);
            break;
        }
        default:
            break;
        }
    }
}

int main(int argc, char* argv[])
{
    std::cout << "ListPaths Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/sample.pdf";

    if (argc > 1) input_path = argv[1];

    std::cout << "Input file: " << input_path << std::endl;

    try {
        Document doc(input_path);

        for (int pgno = 0; pgno < doc.get_num_pages(); ++pgno) {
            Page page = doc.get_page(pgno);
            Content content = page.get_content();
            list_paths_in_content(content, pgno);
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
