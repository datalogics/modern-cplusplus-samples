/*
 * This sample adds Optional Content Groups (layers) to a PDF document and
 * then adds Content to those layers.
 *
 * The related ChangeLayerConfiguration program makes layers visible or invisible.
 *
 * You can toggle back and forth to make a layer visible or invisible in a PDF Viewer.
 *
 * Copyright (c) 2007-2023, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <datalogics_interface/library.hpp>
#include <datalogics_interface/document.hpp>
#include <datalogics_interface/page.hpp>
#include <datalogics_interface/content.hpp>
#include <datalogics_interface/element.hpp>
#include <datalogics_interface/image.hpp>
#include <datalogics_interface/text.hpp>
#include <datalogics_interface/text_run.hpp>
#include <datalogics_interface/text_state.hpp>
#include <datalogics_interface/graphic_state.hpp>
#include <datalogics_interface/font.hpp>
#include <datalogics_interface/container.hpp>
#include <datalogics_interface/optional_content_config.hpp>
#include <datalogics_interface/optional_content_group.hpp>
#include <datalogics_interface/optional_content_order.hpp>
#include <datalogics_interface/optional_content_membership_dict.hpp>
#include <datalogics_interface/geometry.hpp>
#include <iostream>
#include <vector>
#include <string>

using namespace datalogics_interface;

std::vector<OptionalContentGroup> create_new_ocgs(Document& doc, const std::vector<std::string>& names);
void associate_ocg_with_container(Document& doc, OptionalContentGroup& ocg, Container& cont);

int main(int argc, char* argv[]) {
    std::cout << "CreateLayer Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string sInput = Library::get_resource_directory() + "Sample_Input/ducky.pdf";
    std::string sOutput = "CreateLayer-out.pdf";

    if (argc > 1)
        sInput = argv[1];
    if (argc > 2)
        sOutput = argv[2];

    std::cout << "Input file: " << sInput << ", writing to " << sOutput << std::endl;

    Document doc(sInput);

    std::cout << "Opened a document." << std::endl;

    Page pg = doc.get_page(0);

    // Get existing image and scale it to half
    auto elemPtr = pg.get_content().get_element(0);
    auto* image = static_cast<Image*>(elemPtr.get());
    Matrix m = image->get_matrix();
    image->set_matrix(Matrix(m.a * 0.5, 0, 0, m.d * 0.5, m.h, m.v));

    Image image2(Library::get_resource_directory() + "Sample_Input/Image.png");

    Text text;
    Font font("Helvetica");
    GraphicState graphicState;
    TextState textState;

    Matrix matrix1(42, 0, 0, 22, 72, 72);
    TextRun textRun("sample text", font, graphicState, textState, matrix1);
    text.add_run(textRun);

    Text text2;
    Matrix matrix2(30, 0, 0, 30, 72, 288);
    TextRun textRun2("Text definition provided here", font, graphicState, textState, matrix2);
    text2.add_run(textRun2);

    // Containers are required to attach OCGs to images/text
    Container imageContainer;
    Content imgContent1;
    imgContent1.add_element(*image);
    imageContainer.set_content(&imgContent1);

    Container imageContainer2;
    Content imgContent2;
    imgContent2.add_element(image2);
    imageContainer2.set_content(&imgContent2);

    Container textContainer;
    Content txtContent1;
    txtContent1.add_element(text);
    textContainer.set_content(&txtContent1);

    Container textContainer2;
    Content txtContent2;
    txtContent2.add_element(text2);
    textContainer2.set_content(&txtContent2);

    Document newDoc;
    Page newPage = newDoc.create_page(Document::before_first_page, pg.get_media_box());

    Content pageContent = newPage.get_content();
    pageContent.add_element(imageContainer);
    pageContent.add_element(imageContainer2);
    pageContent.add_element(textContainer);
    pageContent.add_element(textContainer2);

    // Create OCGs and add to Order array
    std::vector<OptionalContentGroup> ocgs = create_new_ocgs(
        newDoc, {"Rubber Ducky", "PNG Logo", "Example Text", "Text Definition"});

    associate_ocg_with_container(newDoc, ocgs[0], imageContainer);
    associate_ocg_with_container(newDoc, ocgs[1], imageContainer2);
    associate_ocg_with_container(newDoc, ocgs[2], textContainer);
    associate_ocg_with_container(newDoc, ocgs[3], textContainer2);

    newPage.update_content();
    newDoc.save(SaveFlags::Full, sOutput);
    std::cout << "Saved to " << sOutput << std::endl;

    return 0;
}

std::vector<OptionalContentGroup> create_new_ocgs(Document& doc, const std::vector<std::string>& names) {
    std::vector<OptionalContentGroup> ocgs;
    for (const auto& name : names)
        ocgs.emplace_back(doc, name);

    OptionalContentConfig config(doc);
    auto order_list = config.get_order();

    OptionalContentOrderArray grouping(doc, "Image Grouping");
    OptionalContentOrderLeaf leaf0(ocgs[0]);
    OptionalContentOrderLeaf leaf1(ocgs[1]);
    grouping.add(leaf0);
    grouping.add(leaf1);

    OptionalContentOrderArray grouping2(doc, "Text Grouping");
    OptionalContentOrderLeaf leaf2(ocgs[2]);
    OptionalContentOrderLeaf leaf3(ocgs[3]);
    grouping2.add(leaf2);
    grouping2.add(leaf3);

    order_list->insert(order_list->get_length(), grouping);
    order_list->insert(order_list->get_length(), grouping2);

    return ocgs;
}

void associate_ocg_with_container(Document& doc, OptionalContentGroup& ocg, Container& cont) {
    std::vector<OptionalContentGroup> ocg_vec;
    ocg_vec.emplace_back(doc, ocg.get_name());
    OptionalContentMembershipDict ocmd(doc, ocg_vec, VisibilityPolicy::AnyOn);
    cont.set_optional_content_membership_dict(&ocmd);
}
