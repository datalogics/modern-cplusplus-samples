/*
 * This sample program demonstrates the use of AddDigitalSignature for RFC3161 timestamp
 * signature type.
 *
 * Copyright (c) 2025, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "AddDigitalSignatureRFC3161 Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input_path  = Library::get_resource_directory() + "Sample_Input/CreateAcroForm2h.pdf";
    std::string output_path = "DigSigRFC3161-out.pdf";

    if (argc > 1) input_path  = argv[1];
    if (argc > 2) output_path = argv[2];

    std::cout << "Input file: " << input_path << std::endl;
    std::cout << "Writing to output: " << output_path << std::endl;

    try {
        Document doc(input_path);

        SignDoc sig_doc;

        // Setup Sign params — search for the first unsigned field
        sig_doc.set_field_id(SignatureFieldID::SearchForFirstUnsignedField);

        // Set credential related attributes
        sig_doc.set_digest_category(DigestCategory::SHA256);

        // Set the signature type to RFC3161/TimeStamp
        sig_doc.set_signature_type(SignatureType::RFC3161);

        // Setup Save params
        sig_doc.set_output_path(output_path);

        // Sign and save the document
        sig_doc.add_digital_signature(doc);

        std::cout << "Signed document saved to " << output_path << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
