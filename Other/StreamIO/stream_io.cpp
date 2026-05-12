/*
 * Run this program to extract content from a PDF file using streams. The program demonstrates
 * reading a PDF document from a std::ifstream and writing a PDF document to a std::ostringstream.
 *
 * A stream is a string of bytes of any length, embedded in a PDF document with a dictionary
 * that is used to interpret the values in the stream.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace datalogics_interface;

// Demonstrate reading a PDF Document from a std::istream.
static void read_from_stream(const std::string& path, const std::string& output)
{
    // Open the file as a stream. A std::ifstream is used here for demonstration;
    // the technique works with any seekable std::istream.
    std::ifstream fs(path, std::ios::binary);
    if (!fs) {
        throw std::runtime_error("Cannot open input file: " + path);
    }

    // Open the document from the stream. The stream must remain valid for
    // the lifetime of the Document.
    Document d(fs);

    // Add a watermark so there is a visible change in the PDF.
    WatermarkTextParams wtp;
    wtp.set_text("This PDF was opened\nfrom a Stream");

    WatermarkParams wp;
    // Apply to all pages
    wp.set_start_page(0);
    wp.set_end_page(Document::last_page);

    d.watermark(wtp, wp);

    // Save to a file (full save required when backed by a stream).
    d.save(SaveFlags::Full, output);

    // Make another minor change.
    d.set_creator("PDFL StreamIO Sample");

    // Since the document is now backed by a file, an incremental save is OK.
    d.save(SaveFlags::Incremental);
}

// Demonstrate writing a PDF Document to a std::ostream (in-memory).
static void write_to_stream(const std::string& output)
{
    std::ostringstream ms;

    {
        Document d;
        d.set_creator("PDFL StreamIO Sample");

        Rect bounds(0, 0, 612, 792);
        d.create_page(Document::before_first_page, bounds);

        // Save the document to the in-memory stream.
        d.save(SaveFlags::Full, ms);
    }

    // Open a new document from the saved stream data.
    std::istringstream is(ms.str());
    {
        Document d(is);
        std::cout << "creator: " << d.get_creator() << std::endl;
    }

    // Write the stream to a file and open from the file path.
    {
        std::ofstream fs(output, std::ios::binary);
        const std::string& buf = ms.str();
        fs.write(buf.data(), static_cast<std::streamsize>(buf.size()));
    }

    Document d2(output);
    std::cout << "creator: " << d2.get_creator() << std::endl;
}

int main(int argc, char* argv[])
{
    std::cout << "StreamIO Sample:" << std::endl;

    Library lib;

    std::string input_path  = Library::get_resource_directory() + "Sample_Input/sample.pdf";
    std::string output1     = "StreamIO-out1.pdf";
    std::string output2     = "StreamIO-out2.pdf";

    if (argc > 1) input_path = argv[1];
    if (argc > 2) output1    = argv[2];
    if (argc > 3) output2    = argv[3];

    std::cout << "Input file: " << input_path
              << ". Writing to output " << output1
              << " and " << output2 << std::endl;

    try {
        read_from_stream(input_path, output1);
        write_to_stream(output2);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
