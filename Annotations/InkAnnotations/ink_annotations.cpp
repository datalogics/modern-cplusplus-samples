/*
 * This sample creates and adds a new Ink annotation to a PDF document. An Ink annotation is a
 * freeform line, similar to what you would create with a pen, or with a stylus on a mobile device.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <vector>

using namespace datalogics_interface;

int main()
{
    std::cout << "InkAnnotations Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    try {
        // Create a new document and blank first page
        Document doc;
        Rect rect{0, 0, 612, 792};
        Page page = doc.create_page(Document::before_first_page, rect);
        std::cout << "Created new document and first page." << std::endl;

        // Create and add a new InkAnnotation to the 0th element of first page's annotation array
        InkAnnotation ink_annot(page, rect, -1);
        std::cout << "Created new InkAnnotation as 0th element of annotation array." << std::endl;

        // Ask how many scribbles are in the ink annotation
        std::cout << "Number of scribbles in ink annotation: " << ink_annot.get_num_scribbles() << std::endl;

        // Create a vector of scribble vertices
        std::vector<Point> scribble;
        scribble.push_back({100, 100});
        scribble.push_back({200, 300});
        scribble.push_back({400, 200});
        std::cout << "Created an array of scribble points." << std::endl;

        // Add the scribble to the ink annotation
        ink_annot.add_scribble(scribble);
        std::cout << "Added the scribble to the ink annotation." << std::endl;

        // Ask how many scribbles are in the ink annotation
        std::cout << "Number of scribbles in ink annotation: " << ink_annot.get_num_scribbles() << std::endl;

        // Create another vector of scribble vertices
        scribble.clear();
        scribble.push_back({200, 200});
        scribble.push_back({200, 300});
        scribble.push_back({300, 300});
        scribble.push_back({300, 200});
        scribble.push_back({200, 100});
        std::cout << "Created another array of scribble points." << std::endl;

        ink_annot.add_scribble(scribble);
        std::cout << "Added the scribble to the ink annotation." << std::endl;

        // Create another vector of scribble vertices
        scribble.clear();
        scribble.push_back({300, 400});
        scribble.push_back({200, 300});
        scribble.push_back({300, 300});
        std::cout << "Created another array of scribble points." << std::endl;

        ink_annot.add_scribble(scribble);
        std::cout << "Added the scribble to the ink annotation." << std::endl;

        // Ask how many scribbles are in the ink annotation
        std::cout << "Number of scribbles in ink annotation: " << ink_annot.get_num_scribbles() << std::endl;

        // Get and display the points in scribble 0
        auto scribble0 = ink_annot.get_scribble(0);
        for (std::size_t i = 0; i < scribble0.size(); ++i)
            std::cout << "Scribble 0, point " << i << " : (" << scribble0[i].h << ", " << scribble0[i].v << ")" << std::endl;

        // Get and display the points in scribble 1
        auto scribble1 = ink_annot.get_scribble(1);
        for (std::size_t i = 0; i < scribble1.size(); ++i)
            std::cout << "Scribble 1, point " << i << " : (" << scribble1[i].h << ", " << scribble1[i].v << ")" << std::endl;

        // Set the color and generate an appearance stream
        ink_annot.set_color(Color{0.5, 0.3, 0.8});
        std::cout << "Set the stroke color." << std::endl;

        auto form = ink_annot.generate_appearance();
        ink_annot.set_normal_appearance(*form);
        std::cout << "Generated the appearance stream." << std::endl;

        // Update the page's content and save the file
        page.update_content();
        doc.save(SaveFlags::Full, "InkAnnotations-out1.pdf");
        std::cout << "Saved InkAnnotations-out1.pdf" << std::endl;

        // Remove 0th scribble
        ink_annot.remove_scribble(0);
        std::cout << "Number of scribbles in ink annotation: " << ink_annot.get_num_scribbles() << std::endl;

        // Generate a new appearance stream
        auto form2 = ink_annot.generate_appearance();
        ink_annot.set_normal_appearance(*form2);

        // Update the page's content and save
        page.update_content();
        doc.save(SaveFlags::Full, "InkAnnotations-out2.pdf");
        std::cout << "Saved InkAnnotations-out2.pdf" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
