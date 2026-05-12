#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    Library lib;

    std::string input_path = Library::get_resource_directory() + "Sample_Input/sample.pdf";
    std::string output_path = "list_info-out.pdf";

    if (argc > 1) input_path = argv[1];
    if (argc > 2) output_path = argv[2];

    try {
        Document doc(input_path);

        std::cout << "Title: " << doc.get_title() << std::endl;
        std::cout << "Subject: " << doc.get_subject() << std::endl;
        std::cout << "Author: " << doc.get_author() << std::endl;
        std::cout << "Keywords: " << doc.get_keywords() << std::endl;
        std::cout << "Creator: " << doc.get_creator() << std::endl;
        std::cout << "Producer: " << doc.get_producer() << std::endl;

        doc.set_title("Modified title");
        doc.set_subject("Modified subject");
        doc.set_author("Modified author");
        doc.set_keywords("Modified keywords");
        doc.set_creator("Modified creator");

        doc.save(SaveFlags::Full | SaveFlags::Linearized, output_path);

        std::cout << "\nModified document saved to " << output_path << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
