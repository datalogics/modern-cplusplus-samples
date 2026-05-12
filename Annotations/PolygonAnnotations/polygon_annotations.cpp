/*
 * This program generates a PDF output file with a polygon shape (a triangle) as an annotation
 * to the file. The program defines the vertices for the outlines of the annotation, and the
 * line and fill colors.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "PolygonAnnotation Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string output_path = "PolygonAnnotations-out.pdf";

    if (argc > 1) output_path = argv[1];

    std::cout << "Writing to output " << output_path << std::endl;

    try {
        // Create a new document and blank first page
        Document doc;
        Rect rect{0, 0, 612, 792};
        Page page = doc.create_page(Document::before_first_page, rect);
        std::cout << "Created new document and first page." << std::endl;

        // Create a vector of polygon vertices
        std::vector<Point> vertices;
        vertices.push_back({100, 100});
        vertices.push_back({200, 300});
        vertices.push_back({400, 200});
        std::cout << "Created an array of vertex points." << std::endl;

        // Create and add a new PolygonAnnotation to the 0th element of first page's annotation array
        PolygonAnnotation polygon_annot(page, rect, vertices, -1);
        std::cout << "Created new PolygonAnnotation as 0th element of annotation array." << std::endl;

        // Retrieve and display the vertices
        auto vertices2 = polygon_annot.get_vertices();
        std::cout << "Retrieved the vertices of the polygon annotation." << std::endl;
        std::cout << "They are:" << std::endl;
        for (std::size_t i = 0; i < vertices2.size(); ++i)
            std::cout << "Vertex " << i << ": (" << vertices2[i].h << ", " << vertices2[i].v << ")" << std::endl;

        // Set colors and generate an appearance stream
        polygon_annot.set_interior_color(Color{0.5, 0.3, 0.8});
        polygon_annot.set_color(Color{0.9, 0.7, 0.1});
        std::cout << "Set the stroke and fill colors." << std::endl;

        auto form = polygon_annot.generate_appearance();
        polygon_annot.set_normal_appearance(*form);
        std::cout << "Generated the appearance stream." << std::endl;

        // Update the page's content and save the file
        page.update_content();
        doc.save(SaveFlags::Full, output_path);
        std::cout << "Saved " << output_path << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
