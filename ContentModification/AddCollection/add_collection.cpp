/*
 * This sample shows how to add a Collection to a PDF document to turn that document
 * into a PDF Portfolio.
 *
 * A PDF Portfolio can hold and display multiple additional files as attachments.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <vector>

using namespace datalogics_interface;

int main(int argc, char* argv[]) {
    std::cout << "AddCollection Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string sInput = lib.get_resource_directory() + "Sample_Input/Attachments.pdf";
    const std::string sOutput = "Portfolio.pdf";

    if (argc > 1)
        sInput = argv[1];

    Document doc(sInput);

    std::cout << "Input file: " << sInput << ". Writing to " << sOutput << std::endl;

    // Check if document already has collection; if not, create it
    auto collection_ptr = doc.get_collection();
    if (!collection_ptr) {
        doc.create_collection();
        collection_ptr = doc.get_collection();
    }

    Collection& collection = *collection_ptr;

    // Create a couple of schema fields
    CollectionSchemaField field("Description", SchemaFieldSubtype::Description);
    field.set_name("DescriptionField");
    field.set_index(0);
    field.set_visible(true);
    field.set_editable(false);

    CollectionSchemaField field1("Number", SchemaFieldSubtype::Number);
    field1.set_name("NumberField");
    field1.set_index(1);
    field1.set_visible(true);
    field1.set_editable(true);

    // Retrieve schema from collection and add fields
    CollectionSchema schema = collection.get_schema();
    schema.add_field(field);
    schema.add_field(field1);

    // Create sort collection
    std::vector<CollectionSortItem> colSort;
    colSort.emplace_back("Description", false);
    colSort.emplace_back("Number", true);

    collection.set_sort(colSort);

    // Set view mode
    collection.change_view_mode(CollectionViewType::Detail, CollectionSplitType::Preview);

    int fieldsCount = schema.get_field_count();
    for (int i = 0; i < fieldsCount; ++i) {
        auto fld = schema.get_field(i);
        std::cout << "Name: " << fld->get_name() << " Index: " << fld->get_index() << std::endl;
    }

    for (const auto& item : collection.get_sort()) {
        std::cout << "Sort item name: " << item.get_name()
                  << " Order: " << (item.get_ascending() ? "true" : "false") << std::endl;
    }

    doc.save(SaveFlags::Full, sOutput);
    std::cout << "Saved to " << sOutput << std::endl;

    return 0;
}
