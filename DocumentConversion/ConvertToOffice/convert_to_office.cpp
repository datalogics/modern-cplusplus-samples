/*
 * ConvertToOffice converts sample PDF documents to Office Documents.
 *
 * Copyright (c) 2023-2025, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <string>

using namespace datalogics_interface;

#if PLATFORM_HAS_PDFTOOFFICE

enum class OfficeType {
    Word = 0,
    Excel = 1,
    PowerPoint = 2,
};

static void convert_pdf_to_office(const std::string& input_path,
                                   const std::string& output_path,
                                   OfficeType office_type) {
    std::cout << "Converting " << input_path << ", output file is " << output_path << std::endl;

    bool result = false;

    if (office_type == OfficeType::Word) {
        result = Document::convert_to_word(input_path, output_path);
    } else if (office_type == OfficeType::Excel) {
        result = Document::convert_to_excel(input_path, output_path);
    } else if (office_type == OfficeType::PowerPoint) {
        result = Document::convert_to_powerpoint(input_path, output_path);
    }

    if (result)
        std::cout << "Successfully converted " << input_path << " to " << output_path << std::endl;
    else
        std::cout << "ERROR: Could not convert " << input_path << std::endl;
}

#endif  // PLATFORM_HAS_PDFTOOFFICE

int main(int argc, char* argv[]) {
    std::cout << "ConvertToOffice Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

#if PLATFORM_HAS_PDFTOOFFICE
    std::string input_word      = Library::get_resource_directory() + "Sample_Input/Word.pdf";
    std::string output_word     = "word-out.docx";
    std::string input_excel     = Library::get_resource_directory() + "Sample_Input/Excel.pdf";
    std::string output_excel    = "excel-out.xlsx";
    std::string input_ppt       = Library::get_resource_directory() + "Sample_Input/PowerPoint.pdf";
    std::string output_ppt      = "powerpoint-out.pptx";

    convert_pdf_to_office(input_word,  output_word,  OfficeType::Word);
    convert_pdf_to_office(input_excel, output_excel, OfficeType::Excel);
    convert_pdf_to_office(input_ppt,   output_ppt,   OfficeType::PowerPoint);
#else
    std::cout << "PDF to Office conversion is not available on this platform." << std::endl;
#endif

    return 0;
}
