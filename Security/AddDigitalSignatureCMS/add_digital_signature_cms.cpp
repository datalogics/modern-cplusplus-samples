/*
 * This sample program demonstrates the use of AddDigitalSignature for CMS signature type.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <iostream>
#include <string>

using namespace datalogics_interface;

int main(int argc, char* argv[])
{
    std::cout << "AddDigitalSignatureCMS Sample:" << std::endl;

    Library lib;
    std::cout << "Initialized the library." << std::endl;

    std::string input_path  = Library::get_resource_directory() + "Sample_Input/SixPages.pdf";
    std::string logo_path   = Library::get_resource_directory() + "Sample_Input/ducky_alpha.tif";
    std::string output_path = "DigSigCMS-out.pdf";

    std::string der_cert = Library::get_resource_directory() + "Sample_Input/Credentials/DER/RSA_certificate.der";
    std::string der_key  = Library::get_resource_directory() + "Sample_Input/Credentials/DER/RSA_privKey.der";

    if (argc > 1) input_path  = argv[1];
    if (argc > 2) output_path = argv[2];
    if (argc > 3) logo_path   = argv[3];

    std::cout << "Input file: " << input_path << std::endl;
    std::cout << "Writing to output: " << output_path << std::endl;

    try {
        Document doc(input_path);

        SignDoc sig_doc;

        // Setup Sign params
        sig_doc.set_field_id(SignatureFieldID::CreateFieldWithQualifiedName);
        sig_doc.set_field_name("Signature_es_:signatureblock");

        // Set credential related attributes
        sig_doc.set_digest_category(DigestCategory::SHA256);
        sig_doc.set_credential_data_format(CredentialDataFormat::NonPFX);
        sig_doc.set_non_pfx_signer_cert(der_cert, 0, CredentialStorageFormat::OnDisk);
        sig_doc.set_non_pfx_private_key(der_key, 0, CredentialStorageFormat::OnDisk);

        // Set the signature type to CMS
        sig_doc.set_signature_type(SignatureType::CMS);

        // Setup the signer information (logo image is optional)
        sig_doc.set_signer_info(logo_path, 0.5f, "John Doe", "Chicago, IL", "Approval",
                                "Datalogics, Inc.", DisplayTraits::All);

        // Set the size and location of the signature box
        sig_doc.set_signature_box_page_number(0);
        sig_doc.set_signature_box_rectangle(Rect{100, 300, 400, 400});

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
