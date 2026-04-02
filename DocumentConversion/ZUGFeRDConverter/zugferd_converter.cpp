/*
 * This sample demonstrates converting the input PDF with the input Invoice ZUGFeRD XML to a
 * ZUGFeRD compliant PDF.
 *
 * Copyright (c) 2007-2025, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <iostream>
#include <string>

using namespace datalogics_interface;

static void add_metadata_and_extension_schema(Document& document,
                                               const std::string& input_invoice_xml) {
    const std::string namespace_uri = "urn:ferd:pdfa:CrossIndustryDocument:invoice:2p0#";
    const std::string namespace_prefix = "zf";

    // Set the XMP ZUGFeRD properties
    document.set_xmp_metadata_property(namespace_uri, namespace_prefix, "DocumentType",     "INVOICE");
    document.set_xmp_metadata_property(namespace_uri, namespace_prefix, "DocumentFileName", input_invoice_xml);
    document.set_xmp_metadata_property(namespace_uri, namespace_prefix, "ConformanceLevel", "BASIC");
    document.set_xmp_metadata_property(namespace_uri, namespace_prefix, "Version",          "2p0");

    // Create the PDF/A Extension Schema for ZUGFeRD since it's not part of the PDF/A standard.
    std::string extension_schema =
        "<rdf:Description rdf:about=\"\"\n"
        "xmlns:pdfaExtension=\"http://www.aiim.org/pdfa/ns/extension/\"\n"
        "xmlns:pdfaSchema=\"http://www.aiim.org/pdfa/ns/schema#\"\n"
        "xmlns:pdfaProperty=\"http://www.aiim.org/pdfa/ns/property#\">\n"
        "<pdfaExtension:schemas>\n"
        "<rdf:Bag>\n"
        "<rdf:li rdf:parseType=\"Resource\">\n"
        "<pdfaSchema:schema>ZUGFeRD PDFA Extension Schema</pdfaSchema:schema>\n"
        "<pdfaSchema:namespaceURI>urn:ferd:pdfa:CrossIndustryDocument:invoice:2p0#</pdfaSchema:namespaceURI>\n"
        "<pdfaSchema:prefix>zf</pdfaSchema:prefix>\n"
        "<pdfaSchema:property>\n"
        "<rdf:Seq>\n"
        "<rdf:li rdf:parseType=\"Resource\">\n"
        "<pdfaProperty:name>DocumentFileName</pdfaProperty:name>\n"
        "<pdfaProperty:valueType>Text</pdfaProperty:valueType>\n"
        "<pdfaProperty:category>external</pdfaProperty:category>\n"
        "<pdfaProperty:description>name of the embedded XML invoice file</pdfaProperty:description>\n"
        "</rdf:li>\n"
        "<rdf:li rdf:parseType=\"Resource\">\n"
        "<pdfaProperty:name>DocumentType</pdfaProperty:name>\n"
        "<pdfaProperty:valueType>Text</pdfaProperty:valueType>\n"
        "<pdfaProperty:category>external</pdfaProperty:category>\n"
        "<pdfaProperty:description>INVOICE</pdfaProperty:description>\n"
        "</rdf:li>\n"
        "<rdf:li rdf:parseType=\"Resource\">\n"
        "<pdfaProperty:name>Version</pdfaProperty:name>\n"
        "<pdfaProperty:valueType>Text</pdfaProperty:valueType>\n"
        "<pdfaProperty:category>external</pdfaProperty:category>\n"
        "<pdfaProperty:description>The actual version of the ZUGFeRD XML schema</pdfaProperty:description>\n"
        "</rdf:li>\n"
        "<rdf:li rdf:parseType=\"Resource\">\n"
        "<pdfaProperty:name>ConformanceLevel</pdfaProperty:name>\n"
        "<pdfaProperty:valueType>Text</pdfaProperty:valueType>\n"
        "<pdfaProperty:category>external</pdfaProperty:category>\n"
        "<pdfaProperty:description>The conformance level of the embedded ZUGFeRD data</pdfaProperty:description>\n"
        "</rdf:li>\n"
        "</rdf:Seq>\n"
        "</pdfaSchema:property>\n"
        "</rdf:li>\n"
        "</rdf:Bag>\n"
        "</pdfaExtension:schemas>\n"
        "</rdf:Description>\n"
        "</rdf:RDF>\n";

    // Replace the ending of the existing XMP metadata with the extension schema
    std::string xmp_metadata = document.get_xmp_metadata();
    std::string end_tag = "</rdf:RDF>";
    auto pos = xmp_metadata.rfind(end_tag);
    if (pos != std::string::npos) {
        xmp_metadata.replace(pos, end_tag.length(), extension_schema);
    }
    document.set_xmp_metadata(xmp_metadata);
}

int main(int argc, char* argv[]) {
    std::cout << "ZUGFeRDConverter Sample:" << std::endl;

    Library lib;

    std::cout << "Initialized the library." << std::endl;

    if (argc < 3) {
        std::cout << "You must specify an input PDF and an input ZUGFeRD Invoice XML file, e.g.:" << std::endl;
        std::cout << std::endl;
        std::cout << "zugferd_converter input-file.pdf input-ZUGFeRD-invoice.xml" << std::endl;
        return 1;
    }

    std::string s_input_pdf        = argv[1];
    std::string s_input_invoice_xml = argv[2];
    std::string s_output           = "ZUGFeRDConverter-out.pdf";

    std::cout << "Converting " << s_input_pdf << " with " << s_input_invoice_xml
              << ", output file is " << s_output << std::endl;

    // Step 1) Open the input PDF
    Document doc(s_input_pdf);

    // Step 2) Open the input Invoice XML and attach it to the PDF
    FileAttachment attachment(doc, s_input_invoice_xml);

    // Make a conversion parameters object
    PDFAConvertParams pdfa_params;
    pdfa_params.set_ignore_font_errors(false);
    pdfa_params.set_no_validation_errors(false);
    pdfa_params.set_validate_implementation_limits(true);

    // Step 3) Convert the input PDF to be a PDF/A-3 document
    try {
        ConvertResult pdfa_result = doc.clone_as_pdfa_document(PDFAConvertType::RGB3b, pdfa_params);

        std::cout << "Successfully converted " << s_input_pdf << " to PDF/A." << std::endl;

        Document& pdfa_doc = pdfa_result.document;

        // Step 4) Add the required XMP metadata entries
        add_metadata_and_extension_schema(pdfa_doc, s_input_invoice_xml);

        // Step 5) Save the document
        pdfa_doc.save(pdfa_result.save_flags, s_output);
    } catch (const std::exception& ex) {
        std::cout << "ERROR: Could not convert " << s_input_pdf << " to PDF/A." << std::endl;
        std::cout << "Exception: " << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
