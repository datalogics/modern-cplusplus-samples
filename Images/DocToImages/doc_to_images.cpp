/*
 * This sample demonstrates how to convert a PDF document into a series of graphic image files,
 * one per page. You can also create a multi-page TIFF file. This program requires that you
 * enter formatting values manually at the command line.
 *
 * Copyright (c) 2007-2023, Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace datalogics_interface;

// -------------------------------------------------------
// Options struct
// -------------------------------------------------------
struct DocToImagesOptions {
    ImageType output_format   = ImageType::Invalid;
    double    hres            = 300.0;
    double    vres            = 300.0;
    int       hpixels         = 0;
    int       vpixels         = 0;
    int       quality         = 0;
    bool      first_page_only = false;
    bool      gray_halftone   = false;
    bool      reverse_gray    = false;
    bool      black_is_one    = false;
    bool      multipage       = false;
    bool      as_printed      = false;
    int       zero_suffix     = 0;
    int       even_odd_pages  = 0;   // 0=all, 1=odd, 2=even
    CompressionCode compress  = CompressionCode::Default;
    SmoothFlags smoothing     = SmoothFlags::None;
    std::string page_region   = "crop";
    std::string output_file   = "";
    std::string output_dir    = "";
    std::vector<int> page_list;
    std::vector<std::string> font_dirs;
};

static void usage()
{
    std::cout << "DocToImages Usage:\n"
              << "DocToImages [options] inputPDF\n"
              << "inputPDF is the name of the PDF file to open\n"
              << "Options are one or more of:\n"
              << "-format=[tif|jpg|bmp|png|gif], No default\n"
              << "-color=[gray|cmyk|rgb], default=rgb (sets image color space)\n"
              << "-grayhalftone=[n|y]\n"
              << "-first=[y|n] Only convert the first page, default=n\n"
              << "-quality=1-100. Only valid for jpg\n"
              << "-resolution=[horiz x vert], default=300\n"
              << "-pixels=[width x height]\n"
              << "-compression=[default|no|lzw|g3|g4|flate|dct]\n"
              << "-region=[crop|media|art|trim|bleed|bounding]\n"
              << "-pages=[list or range, or even or odd]\n"
              << "-output=[filename]\n"
              << "-smoothing=[none|text|all]\n"
              << "-reverse=[y|n]\n"
              << "-blackisone=[y|n]\n"
              << "-multi=[y|n]\n"
              << "-digits=[0-9]\n"
              << "-asprinted=[y|n]\n";
}

static std::string create_file_suffix(const std::string& base, ImageType type)
{
    switch (type) {
        case ImageType::BMP:  return base + ".bmp";
        case ImageType::GIF:  return base + ".gif";
        case ImageType::JPEG: return base + ".jpg";
        case ImageType::PNG:  return base + ".png";
        case ImageType::TIFF: return base + ".tif";
        default: return base;
    }
}

static std::string format_digits(int num_digits, int counter)
{
    if (num_digits == 0) return std::to_string(counter);
    std::ostringstream oss;
    oss << std::setw(num_digits) << std::setfill('0') << counter;
    return oss.str();
}

static void save_image(Image& img, int index, const DocToImagesOptions& opts,
                       const ImageSaveParams& isp)
{
    std::string path;
    if (!opts.output_dir.empty())
        path = opts.output_dir + "/" + opts.output_file + format_digits(opts.zero_suffix, index);
    else
        path = opts.output_file + format_digits(opts.zero_suffix, index);

    path = create_file_suffix(path, opts.output_format);
    try {
        img.save(path, opts.output_format, isp);
        std::cout << "Saved " << path << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Cannot write an image to a file: " << ex.what() << std::endl;
    }
}

// Parse "key=value" from "-key=value" style argument
static std::string opt_value(const std::string& arg)
{
    auto pos = arg.find('=');
    if (pos == std::string::npos) return "";
    return arg.substr(pos + 1);
}

int main(int argc, char* argv[])
{
    std::cout << "PDF Document to Images Sample:" << std::endl;

    if (argc < 2) {
        usage();
        return 1;
    }

    // Last argument must be the PDF path
    std::string doc_path = argv[argc - 1];
    if (doc_path[0] == '-') {
        std::cerr << "The last option must be the path to a PDF file.\n";
        usage();
        return 1;
    }

    DocToImagesOptions options;

    for (int i = 1; i < argc - 1; ++i) {
        std::string arg = argv[i];
        if (arg[0] != '-' || arg.find('=') == std::string::npos) {
            std::cerr << "Invalid option: " << arg << "\n";
            usage();
            return 1;
        }

        std::string val = opt_value(arg);

        if (arg.rfind("-format=", 0) == 0) {
            if      (val == "jpg")  options.output_format = ImageType::JPEG;
            else if (val == "tif")  options.output_format = ImageType::TIFF;
            else if (val == "bmp")  options.output_format = ImageType::BMP;
            else if (val == "png")  options.output_format = ImageType::PNG;
            else if (val == "gif")  options.output_format = ImageType::GIF;
            else { std::cerr << "Invalid format: " << val << "\n"; return 1; }
        }
        else if (arg.rfind("-grayhalftone=", 0) == 0) {
            options.gray_halftone = (val == "y");
        }
        else if (arg.rfind("-first=", 0) == 0) {
            options.first_page_only = (val == "y");
        }
        else if (arg.rfind("-quality=", 0) == 0) {
            options.quality = std::stoi(val);
        }
        else if (arg.find("resolution=") != std::string::npos) {
            auto x = val.find('x');
            if (x != std::string::npos) {
                options.hres = std::stod(val.substr(0, x));
                options.vres = std::stod(val.substr(x + 1));
            } else {
                options.hres = options.vres = std::stod(val);
            }
        }
        else if (arg.rfind("-pixels=", 0) == 0) {
            auto x = val.find('x');
            if (x != std::string::npos) {
                options.hpixels = std::stoi(val.substr(0, x));
                options.vpixels = std::stoi(val.substr(x + 1));
            } else {
                options.hpixels = options.vpixels = std::stoi(val);
            }
        }
        else if (arg.rfind("-compression=", 0) == 0) {
            if      (val == "no" || val == "none") options.compress = CompressionCode::None;
            else if (val == "lzw")                 options.compress = CompressionCode::LZW;
            else if (val == "g3")                  options.compress = CompressionCode::G3;
            else if (val == "g4")                  options.compress = CompressionCode::G4;
            else if (val == "flate")               options.compress = CompressionCode::Flate;
            else if (val == "dct")                 options.compress = CompressionCode::DCT;
            else if (val == "default")             options.compress = CompressionCode::Default;
            else { std::cerr << "Invalid compression: " << val << "\n"; return 1; }
        }
        else if (arg.rfind("-region=", 0) == 0) {
            options.page_region = val;
        }
        else if (arg.find("pages=") != std::string::npos) {
            if (val == "even") {
                options.even_odd_pages = 2;
            } else if (val == "odd") {
                options.even_odd_pages = 1;
            } else {
                // Parse comma-separated list with possible ranges
                std::istringstream iss(val);
                std::string token;
                while (std::getline(iss, token, ',')) {
                    auto dash = token.find('-');
                    if (dash != std::string::npos) {
                        int lo = std::stoi(token.substr(0, dash));
                        int hi = std::stoi(token.substr(dash + 1));
                        for (int p = lo; p <= hi; ++p) options.page_list.push_back(p);
                    } else {
                        options.page_list.push_back(std::stoi(token));
                    }
                }
            }
        }
        else if (arg.rfind("-output=", 0) == 0) {
            options.output_file = val;
        }
        else if (arg.rfind("-smoothing=", 0) == 0) {
            if      (val == "none") options.smoothing = SmoothFlags::None;
            else if (val == "text") options.smoothing = SmoothFlags::Text;
            else if (val == "all")  options.smoothing = SmoothFlags::Text | SmoothFlags::LineArt | SmoothFlags::Image;
            else { std::cerr << "Invalid smoothing: " << val << "\n"; return 1; }
        }
        else if (arg.rfind("-reverse=", 0) == 0) {
            options.reverse_gray = (val == "y");
        }
        else if (arg.rfind("-blackisone=", 0) == 0) {
            options.black_is_one = (val == "y");
        }
        else if (arg.rfind("-multi=", 0) == 0) {
            options.multipage = (val == "y");
        }
        else if (arg.rfind("-digits=", 0) == 0) {
            options.zero_suffix = std::stoi(val);
        }
        else if (arg.rfind("-asprinted=", 0) == 0) {
            options.as_printed = (val == "y");
        }
        else if (arg.rfind("-color=", 0) == 0) {
            // Color space option: stored for future use if set_image_color_space is needed
            // We store the name but DeviceGray/RGB/CMYK ColorSpace objects require
            // getting named color spaces from the API, which is not straightforward here.
            // The C++ API does not expose static named color space accessors.
            // This option is noted but color space change is not applied in this port.
            std::cout << "Note: -color option noted (" << val << ") but not yet applied in C++ port." << std::endl;
        }
        else if (arg.rfind("-fontlist=", 0) == 0) {
            std::istringstream iss(val);
            std::string dir;
            while (std::getline(iss, dir, ';')) {
                if (!dir.empty()) options.font_dirs.push_back(dir);
            }
        }
        else {
            std::cerr << "Invalid option: " << arg << "\n";
            usage();
            return 1;
        }
    }

    if (options.output_format == ImageType::Invalid) {
        std::cerr << "format must be set to tif, jpg, bmp, png, or gif\n";
        return 1;
    }

    if (options.output_format == ImageType::TIFF
        && options.compress == CompressionCode::None) {
        options.compress = CompressionCode::LZW; // default for TIF
    }

    // Derive output filename from doc_path if not specified
    if (options.output_file.empty()) {
        std::string base = doc_path;
        auto slash = base.rfind('/');
        if (slash != std::string::npos) {
            options.output_dir  = base.substr(0, slash);
            base = base.substr(slash + 1);
        }
        auto dot = base.rfind('.');
        if (dot != std::string::npos) base = base.substr(0, dot);
        options.output_file = base;
    }

    Library lib(options.font_dirs);

    Document doc;
    int num_pages = 0;
    try {
        doc = Document(doc_path);
        num_pages = doc.get_num_pages();
    } catch (const std::exception& ex) {
        std::cerr << "Error opening PDF document " << doc_path << ": " << ex.what() << std::endl;
        return 1;
    }

    // Build page list
    if (options.page_list.empty()) {
        int limit = options.first_page_only ? 1 : num_pages;
        for (int i = 0; i < limit; ++i) {
            if (options.even_odd_pages == 0 ||
                (options.even_odd_pages == 1 && (i + 1) % 2 == 1) ||
                (options.even_odd_pages == 2 && (i + 1) % 2 == 0)) {
                options.page_list.push_back(i);
            }
        }
    }

    // Set up PageImageParams
    PageImageParams pip;
    pip.set_page_draw_flags(DrawFlags::UseAnnotFaces | DrawFlags::DoLazyErase);
    pip.set_smoothing(options.smoothing);
    pip.set_horizontal_resolution(options.hres);
    pip.set_vertical_resolution(options.vres);
    if (options.hpixels > 0) pip.set_pixel_width(options.hpixels);
    if (options.vpixels > 0) pip.set_pixel_height(options.vpixels);
    if (options.as_printed)
        pip.set_page_draw_flags(pip.get_page_draw_flags() | DrawFlags::IsPrinting);

    ImageSaveParams isp;
    isp.set_halftone_gray_images(options.gray_halftone);
    isp.set_compression(options.compress);
    if (options.output_format == ImageType::JPEG)
        isp.set_jpeg_quality(options.quality);
    isp.set_reverse_gray(options.reverse_gray);
    isp.set_tiff_black_is_one(options.black_is_one);

    ImageCollection page_collection;

    for (int i = 0; i < static_cast<int>(options.page_list.size()); ++i) {
        Page pg = doc.get_page(options.page_list[i]);

        Rect page_rect;
        if      (options.page_region == "crop")     page_rect = pg.get_crop_box();
        else if (options.page_region == "media")    page_rect = pg.get_media_box();
        else if (options.page_region == "art")      page_rect = pg.get_art_box();
        else if (options.page_region == "bounding") page_rect = pg.get_bbox();
        else if (options.page_region == "bleed")    page_rect = pg.get_bleed_box();
        else if (options.page_region == "trim")     page_rect = pg.get_trim_box();
        else {
            std::cerr << "Unknown page region option.\n";
            return 1;
        }

        try {
            Image page_image = pg.get_image(page_rect, pip);
            if (options.multipage) {
                page_collection.append(std::move(page_image));
            } else {
                save_image(page_image, i + 1, options, isp);
            }
        } catch (const std::exception& ex) {
            std::cerr << "Cannot rasterize page to an image: " << ex.what() << std::endl;
            return 1;
        }
    }

    if (options.multipage) {
        std::string path;
        if (!options.output_dir.empty())
            path = options.output_dir + "/" + options.output_file;
        else
            path = options.output_file;
        path = create_file_suffix(path, options.output_format);
        try {
            page_collection.save(path, options.output_format, isp);
            std::cout << "Saved multi-page " << path << std::endl;
        } catch (const std::exception& ex) {
            std::cerr << "Cannot save images to a multi-page TIF file: " << ex.what() << std::endl;
        }
    }

    return 0;
}
