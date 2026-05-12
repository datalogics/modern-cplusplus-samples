/*
 * This sample demonstrates how to initialize and use RAM memory instead of the local
 * hard disk to save temporary files, in order to save processing time.
 *
 * The program sets the default temp store to TempStoreType::Memory. The program can also
 * set a maximum amount of RAM to use by applying a value to set_default_temp_store_mem_limit.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "MemoryFileSystem Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    std::string output_path = "TempFileSystem.pdf";

    if (argc > 1) output_path = argv[1];

    std::cout << "Writing to output " << output_path << std::endl;

    try {
        // Set in-memory file system as temporary storage
        lib.set_default_temp_store(TempStoreType::Memory);

        // Set memory limit to 100 kB. When occupied memory exceeds the limit,
        // disk temporary storage will be used.
        lib.set_default_temp_store_mem_limit(100);

        Document doc;
        Rect bounds(0, 0, 612, 792);
        doc.create_page(Document::before_first_page, bounds);
        doc.save(SaveFlags::Full, output_path);

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
