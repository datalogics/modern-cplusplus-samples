/*
 * This sample converts a practical Markdown subset directly to a newly
 * created, tagged PDF document using the Datalogics C++ APDFL API. It parses
 * Markdown itself (headings, paragraphs, lists, task lists, tables, fenced
 * code blocks, blockquotes, horizontal rules, links, emphasis, inline code,
 * strikethrough, and an "HTML-lite" subset), lays the content out with real
 * font metrics, and emits logical structure (H1-H6, P, L/LI, Table/TR/TH/TD,
 * Code, BlockQuote, Link) so the output is a tagged, accessible PDF.
 *
 * Images are intentionally not embedded; they are replaced with an
 * "[Image omitted: ...]" note so the sample stays local-file only.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <filesystem>
#include <locale>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace datalogics_interface;

namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Command line
// ─────────────────────────────────────────────────────────────────────────────

class CommandLineException : public std::runtime_error {
public:
    explicit CommandLineException(const std::string& message) : std::runtime_error(message) {}
};

// ASCII-only classification helpers. The APDFL Library changes the process
// locale when it initializes, so the <cctype> functions must not be used on
// UTF-8 text: multibyte continuation bytes would be misclassified.
bool ascii_space(unsigned char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

bool ascii_digit(unsigned char c) { return c >= '0' && c <= '9'; }

bool ascii_alnum(unsigned char c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool ascii_punct(unsigned char c) { return c >= '!' && c <= '~' && !ascii_alnum(c); }

// Locale-independent floating point parse ('.' decimal separator).
double parse_double(const std::string& value)
{
    std::istringstream stream(value);
    stream.imbue(std::locale::classic());
    double out = 0;
    if (!(stream >> out) || !(stream >> std::ws).eof())
        throw std::invalid_argument("not a number: " + value);
    return out;
}

std::string to_lower(const std::string& s)
{
    std::string out = s;
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

enum class Orientation { Auto, Portrait, Landscape };

struct ConversionOptions {
    std::string input_path = "sample.md";
    std::string output_path = "MarkdownToPDF-out.pdf";
    std::string title;                    // empty -> input file name
    std::string language = "en-US";
    std::string page_size = "Letter";
    Orientation orientation = Orientation::Auto;
    double margin = 72.0;
    std::string font_family = "Times";
    std::string heading_font_family;      // empty -> font_family
    std::string code_font_family = "Courier";
    std::string cjk_font_family;
    std::vector<std::string> fallback_fonts;
    bool overwrite = false;
    bool verbose = false;
    bool include_unrendered_html = false;
};

const std::map<std::string, std::pair<double, double>>& named_page_sizes()
{
    static const std::map<std::string, std::pair<double, double>> sizes = {
        {"letter", {612, 792}},  {"legal", {612, 1008}}, {"ledger", {1224, 792}},
        {"a3", {842, 1191}},     {"a4", {595, 842}},     {"a5", {420, 595}},
        {"tabloid", {792, 1224}},
    };
    return sizes;
}

std::pair<double, double> resolve_page_size(const std::string& value)
{
    auto it = named_page_sizes().find(to_lower(value));
    if (it != named_page_sizes().end())
        return it->second;

    static const std::regex custom(R"(^(\d+(?:\.\d+)?)\s*[xX]\s*(\d+(?:\.\d+)?)$)");
    std::smatch m;
    if (std::regex_match(value, m, custom)) {
        double w = parse_double(m[1].str());
        double h = parse_double(m[2].str());
        if (w < 144 || w > 2880 || h < 144 || h > 2880)
            throw CommandLineException("Custom page dimensions must be 144-2880 points: " + value);
        return {w, h};
    }
    throw CommandLineException(
        "Unsupported page size: " + value +
        ". Use Letter, Legal, Ledger, A3, A4, A5, Tabloid, or WIDTHxHEIGHT in points.");
}

void print_usage()
{
    std::cout <<
        "Usage:\n"
        "  markdown_to_pdf [options]\n"
        "  markdown_to_pdf [options] input.md output.pdf\n"
        "\n"
        "With no positional arguments the sample converts sample.md to\n"
        "MarkdownToPDF-out.pdf in the current directory.\n"
        "\n"
        "Document metadata:\n"
        "  --title <text>         PDF title. Defaults to the input file name.\n"
        "  --lang <tag>           Document language tag. Defaults to en-US.\n"
        "\n"
        "Page setup:\n"
        "  --page-size <value>    Letter, Legal, Ledger, A3, A4, A5, Tabloid, or\n"
        "                         WIDTHxHEIGHT in points. Defaults to Letter.\n"
        "  --orientation <value>  Auto, Portrait, or Landscape. Defaults to Auto.\n"
        "  --margin <points>      Margin on all sides in PDF points (18-144).\n"
        "                         Defaults to 72.\n"
        "\n"
        "Fonts:\n"
        "  --font-family <name>          Body font family. Defaults to Times.\n"
        "  --heading-font-family <name>  Heading font family. Defaults to the body font.\n"
        "  --code-font-family <name>     Code font family. Defaults to Courier.\n"
        "  --cjk-font-family <name>      Preferred font for CJK text.\n"
        "  --fallback-fonts <csv>        Comma-separated fallback font names.\n"
        "  --list-font-families          List usable font families and exit.\n"
        "\n"
        "HTML handling:\n"
        "  --include-unrendered-html     Keep unsupported raw HTML tags as literal text.\n"
        "\n"
        "Diagnostics and help:\n"
        "  --overwrite            Replace the output file if it exists.\n"
        "  --verbose              Print conversion settings after converting.\n"
        "  --self-test            Run parser self-tests without creating a PDF.\n"
        "  --help                 Show this help.\n"
        "\n"
        "Set APDFL_LICENSE_KEY to provide a Datalogics APDFL activation key before\n"
        "Library initialization.\n";
}

ConversionOptions parse_options(const std::vector<std::string>& args)
{
    ConversionOptions options;
    std::vector<std::string> positional;

    auto require_value = [&](size_t& i, const std::string& option) -> const std::string& {
        if (i + 1 >= args.size())
            throw CommandLineException(option + " requires a value.");
        return args[++i];
    };

    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& arg = args[i];
        if (arg.empty() || arg[0] != '-') {
            positional.push_back(arg);
            continue;
        }
        const std::string lower = to_lower(arg);
        if (lower == "--title") {
            options.title = require_value(i, arg);
        } else if (lower == "--lang") {
            options.language = require_value(i, arg);
            if (options.language.empty())
                options.language = "en-US";
        } else if (lower == "--page-size") {
            options.page_size = require_value(i, arg);
            resolve_page_size(options.page_size);  // validate early
        } else if (lower == "--orientation") {
            const std::string value = to_lower(require_value(i, arg));
            if (value == "auto")
                options.orientation = Orientation::Auto;
            else if (value == "portrait")
                options.orientation = Orientation::Portrait;
            else if (value == "landscape")
                options.orientation = Orientation::Landscape;
            else
                throw CommandLineException("Orientation must be Auto, Portrait, or Landscape.");
        } else if (lower == "--margin") {
            const std::string value = require_value(i, arg);
            try {
                options.margin = parse_double(value);
            } catch (const std::exception&) {
                throw CommandLineException("Margin must be a number of points: " + value);
            }
            if (options.margin < 18 || options.margin > 144)
                throw CommandLineException("Margin must be between 18 and 144 points.");
        } else if (lower == "--font-family") {
            options.font_family = require_value(i, arg);
        } else if (lower == "--heading-font-family") {
            options.heading_font_family = require_value(i, arg);
        } else if (lower == "--code-font-family") {
            options.code_font_family = require_value(i, arg);
        } else if (lower == "--cjk-font-family") {
            options.cjk_font_family = require_value(i, arg);
        } else if (lower == "--fallback-fonts" || lower == "--fallback-font-family") {
            std::stringstream stream(require_value(i, arg));
            std::string name;
            while (std::getline(stream, name, ',')) {
                const auto begin = name.find_first_not_of(" \t");
                const auto end = name.find_last_not_of(" \t");
                if (begin != std::string::npos)
                    options.fallback_fonts.push_back(name.substr(begin, end - begin + 1));
            }
        } else if (lower == "--overwrite") {
            options.overwrite = true;
        } else if (lower == "--verbose") {
            options.verbose = true;
        } else if (lower == "--include-unrendered-html" || lower == "--include-raw-html") {
            options.include_unrendered_html = true;
        } else {
            throw CommandLineException("Unknown option: " + arg);
        }
    }

    if (!positional.empty() && positional.size() != 2)
        throw CommandLineException("Expected exactly two positional arguments: input.md output.pdf");
    if (positional.size() == 2) {
        options.input_path = positional[0];
        options.output_path = positional[1];
    } else {
        options.overwrite = true;  // the bundled default output is safe to replace
    }
    if (options.heading_font_family.empty())
        options.heading_font_family = options.font_family;
    if (!options.cjk_font_family.empty())
        options.fallback_fonts.insert(options.fallback_fonts.begin(), options.cjk_font_family);
    return options;
}

// ─────────────────────────────────────────────────────────────────────────────
// UTF-8 helpers
// ─────────────────────────────────────────────────────────────────────────────

// Decodes the code point starting at `index` and advances `index` past it.
// Invalid sequences decode as one byte to keep the scanner moving.
unsigned int next_code_point(const std::string& text, size_t& index)
{
    const auto byte = [&](size_t at) { return static_cast<unsigned char>(text[at]); };
    const unsigned char lead = byte(index);
    unsigned int cp = lead;
    size_t len = 1;
    if (lead >= 0xF0 && index + 3 < text.size()) {
        cp = ((lead & 0x07u) << 18) | ((byte(index + 1) & 0x3Fu) << 12) |
             ((byte(index + 2) & 0x3Fu) << 6) | (byte(index + 3) & 0x3Fu);
        len = 4;
    } else if (lead >= 0xE0 && index + 2 < text.size()) {
        cp = ((lead & 0x0Fu) << 12) | ((byte(index + 1) & 0x3Fu) << 6) | (byte(index + 2) & 0x3Fu);
        len = 3;
    } else if (lead >= 0xC0 && index + 1 < text.size()) {
        cp = ((lead & 0x1Fu) << 6) | (byte(index + 1) & 0x3Fu);
        len = 2;
    }
    index += len;
    return cp;
}

std::string encode_code_point(unsigned int cp)
{
    std::string out;
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return out;
}

bool is_cjk_code_point(unsigned int cp)
{
    return (cp >= 0x2E80 && cp <= 0x2EFF) || (cp >= 0x2F00 && cp <= 0x2FDF) ||
           (cp >= 0x3000 && cp <= 0x303F) || (cp >= 0x3040 && cp <= 0x30FF) ||
           (cp >= 0x3100 && cp <= 0x312F) || (cp >= 0x31C0 && cp <= 0x31EF) ||
           (cp >= 0x3400 && cp <= 0x4DBF) || (cp >= 0x4E00 && cp <= 0x9FFF) ||
           (cp >= 0xAC00 && cp <= 0xD7AF) || (cp >= 0xF900 && cp <= 0xFAFF) ||
           (cp >= 0xFF00 && cp <= 0xFFEF);
}

bool is_cyrillic_or_greek_code_point(unsigned int cp)
{
    return (cp >= 0x0370 && cp <= 0x03FF) || (cp >= 0x0400 && cp <= 0x052F);
}

// ─────────────────────────────────────────────────────────────────────────────
// HTML-lite normalization: convert a small, common HTML subset to Markdown so
// documents that mix the two still render. Runs before block parsing and
// skips fenced code regions.
// ─────────────────────────────────────────────────────────────────────────────

std::string decode_html_entities(const std::string& text)
{
    static const std::map<std::string, std::string> named = {
        {"amp", "&"},      {"lt", "<"},       {"gt", ">"},      {"quot", "\""},
        {"apos", "'"},     {"nbsp", " "},     {"mdash", u8"—"}, {"ndash", u8"–"},
        {"hellip", u8"…"}, {"copy", u8"©"}, {"bull", u8"•"}, {"middot", u8"·"},
    };
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size();) {
        if (text[i] != '&') {
            out.push_back(text[i++]);
            continue;
        }
        const size_t semi = text.find(';', i + 1);
        if (semi == std::string::npos || semi - i > 10) {
            out.push_back(text[i++]);
            continue;
        }
        const std::string body = text.substr(i + 1, semi - i - 1);
        if (!body.empty() && body[0] == '#') {
            unsigned int cp = 0;
            try {
                cp = (body.size() > 1 && (body[1] == 'x' || body[1] == 'X'))
                         ? static_cast<unsigned int>(std::stoul(body.substr(2), nullptr, 16))
                         : static_cast<unsigned int>(std::stoul(body.substr(1)));
            } catch (const std::exception&) {
                cp = 0;
            }
            if (cp != 0) {
                out += encode_code_point(cp);
                i = semi + 1;
                continue;
            }
        }
        auto it = named.find(to_lower(body));
        if (it != named.end()) {
            out += it->second;
            i = semi + 1;
            continue;
        }
        out.push_back(text[i++]);
    }
    return out;
}

std::string strip_tags(const std::string& text)
{
    static const std::regex tag(R"(</?[A-Za-z][A-Za-z0-9-]*(?:\s[^>]*?)?/?>)");
    return std::regex_replace(text, tag, "");
}

std::string normalize_html_line(const std::string& input, bool include_unrendered_html)
{
    std::string line = input;

    static const std::regex comment(R"(<!--.*?-->)");
    line = std::regex_replace(line, comment, "");

    // <h1>..</h6> -> ATX headings on their own line.
    static const std::regex heading(R"(<[hH]([1-6])[^>]*>(.*?)</[hH]\1\s*>)");
    std::smatch m;
    while (std::regex_search(line, m, heading)) {
        const int level = m[1].str()[0] - '0';
        const std::string text = decode_html_entities(strip_tags(m[2].str()));
        line = std::string(m.prefix()) + "\n" + std::string(static_cast<size_t>(level), '#') +
               " " + text + "\n" + std::string(m.suffix());
    }

    // <a href="...">label</a> -> [label](url)
    static const std::regex anchor(
        R"re(<[aA]\s+[^>]*[hH][rR][eE][fF]\s*=\s*(?:"([^"]*)"|'([^']*)'|([^\s>]+))[^>]*>(.*?)</[aA]\s*>)re");
    while (std::regex_search(line, m, anchor)) {
        std::string url = m[1].matched ? m[1].str() : (m[2].matched ? m[2].str() : m[3].str());
        url = decode_html_entities(url);
        const std::string label = decode_html_entities(strip_tags(m[4].str()));
        const std::string replacement =
            url.empty() ? label : "[" + label + "](" + url + ")";
        line = std::string(m.prefix()) + replacement + std::string(m.suffix());
    }

    // <img ...> -> [Image omitted: alt-or-src]
    static const std::regex img(R"(<[iI][mM][gG]\b[^>]*/?>)");
    static const std::regex alt_attr(R"re([aA][lL][tT]\s*=\s*(?:"([^"]*)"|'([^']*)'))re");
    static const std::regex src_attr(R"re([sS][rR][cC]\s*=\s*(?:"([^"]*)"|'([^']*)'))re");
    while (std::regex_search(line, m, img)) {
        const std::string tag = m[0].str();
        std::smatch attr;
        std::string label;
        if (std::regex_search(tag, attr, alt_attr))
            label = attr[1].matched ? attr[1].str() : attr[2].str();
        if (label.empty() && std::regex_search(tag, attr, src_attr))
            label = attr[1].matched ? attr[1].str() : attr[2].str();
        const std::string replacement =
            label.empty() ? "[Image omitted]" : "[Image omitted: " + decode_html_entities(label) + "]";
        line = std::string(m.prefix()) + replacement + std::string(m.suffix());
    }

    // Paired style tags -> Markdown, repeated until stable to handle nesting.
    struct PairedTag { const char* pattern; const char* prefix; const char* suffix; };
    static const PairedTag paired[] = {
        {R"(<(?:strong|b|STRONG|B)\b[^>]*>(.*?)</(?:strong|b|STRONG|B)\s*>)", "**", "**"},
        {R"(<(?:em|i|EM|I)\b[^>]*>(.*?)</(?:em|i|EM|I)\s*>)", "*", "*"},
        {R"(<(?:code|kbd|samp|CODE|KBD|SAMP)\b[^>]*>(.*?)</(?:code|kbd|samp|CODE|KBD|SAMP)\s*>)", "`", "`"},
        {R"(<(?:del|s|strike|DEL|S|STRIKE)\b[^>]*>(.*?)</(?:del|s|strike|DEL|S|STRIKE)\s*>)", "~~", "~~"},
    };
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& entry : paired) {
            const std::regex pattern(entry.pattern);
            if (std::regex_search(line, m, pattern)) {
                const std::string inner = decode_html_entities(strip_tags(m[1].str()));
                line = std::string(m.prefix()) + entry.prefix + inner + entry.suffix +
                       std::string(m.suffix());
                changed = true;
            }
        }
    }

    static const std::regex br(R"(<[bB][rR]\s*/?>)");
    line = std::regex_replace(line, br, "\n\n");

    static const std::regex block_tag(
        R"(</?(?:div|p|section|article|header|footer|main|center|nav|aside|ul|ol|li)\b[^>]*>)",
        std::regex::icase);
    line = std::regex_replace(line, block_tag, "\n");

    static const std::regex inline_tag(R"(</?(?:span|font|small|u|mark|sup|sub)\b[^>]*>)",
                                       std::regex::icase);
    line = std::regex_replace(line, inline_tag, "");

    if (!include_unrendered_html)
        line = strip_tags(line);

    return decode_html_entities(line);
}

std::string normalize_html(const std::string& markdown, bool include_unrendered_html)
{
    std::string text = markdown;
    // Normalize newlines.
    text = std::regex_replace(text, std::regex("\r\n"), "\n");
    std::replace(text.begin(), text.end(), '\r', '\n');

    std::stringstream stream(text);
    std::string line;
    std::string out;
    bool in_fence = false;
    std::string fence_marker;
    while (std::getline(stream, line)) {
        std::string trimmed = line;
        const auto first = trimmed.find_first_not_of(" \t");
        trimmed = first == std::string::npos ? "" : trimmed.substr(first);
        const bool fence_line =
            trimmed.rfind("```", 0) == 0 || trimmed.rfind("~~~", 0) == 0;
        if (fence_line) {
            const std::string marker = trimmed.substr(0, 3);
            if (!in_fence) {
                in_fence = true;
                fence_marker = marker;
            } else if (marker == fence_marker) {
                in_fence = false;
            }
            out += line + "\n";
            continue;
        }
        out += (in_fence ? line : normalize_html_line(line, include_unrendered_html)) + "\n";
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// Markdown model
// ─────────────────────────────────────────────────────────────────────────────

struct InlineRun {
    std::string text;
    bool bold = false;
    bool italic = false;
    bool code = false;
    bool strike = false;
    bool link = false;
    std::string url;

    bool same_style(const InlineRun& other) const
    {
        return bold == other.bold && italic == other.italic && code == other.code &&
               strike == other.strike && link == other.link && url == other.url;
    }
};

enum class ColumnAlignment { Left, Center, Right };

struct ListItem {
    std::vector<InlineRun> runs;
    int level = 0;
    int number = 0;
    int task_state = -1;  // -1 none, 0 unchecked, 1 checked
};

enum class BlockType { Heading, Paragraph, CodeBlock, List, BlockQuote, Table, HorizontalRule };

struct Block {
    BlockType type = BlockType::Paragraph;
    int heading_level = 1;
    std::vector<InlineRun> runs;                       // heading / paragraph / quote
    std::string language;                              // code block
    std::vector<std::string> code_lines;               // code block
    bool ordered = false;                              // list
    std::vector<ListItem> items;                       // list
    std::vector<std::vector<InlineRun>> header_cells;  // table
    std::vector<std::vector<std::vector<InlineRun>>> rows;  // table
    std::vector<ColumnAlignment> alignments;           // table
};

// ─────────────────────────────────────────────────────────────────────────────
// Inline parser
// ─────────────────────────────────────────────────────────────────────────────

class InlineParser {
public:
    explicit InlineParser(const std::map<std::string, std::string>& references)
        : references_(references)
    {
    }

    std::vector<InlineRun> parse(const std::string& text) const
    {
        InlineRun base;
        std::vector<InlineRun> runs;
        parse_into(text, base, runs);
        return merge(runs);
    }

    static std::string normalize_label(const std::string& label)
    {
        std::string out;
        bool pending_space = false;
        for (char c : label) {
            if (ascii_space(static_cast<unsigned char>(c))) {
                pending_space = !out.empty();
                continue;
            }
            if (pending_space) {
                out.push_back(' ');
                pending_space = false;
            }
            if (c >= 'A' && c <= 'Z')
                c = static_cast<char>(c - 'A' + 'a');
            out.push_back(c);
        }
        return out;
    }

private:
    const std::map<std::string, std::string>& references_;

    static std::vector<InlineRun> merge(const std::vector<InlineRun>& runs)
    {
        std::vector<InlineRun> out;
        for (const auto& run : runs) {
            if (run.text.empty())
                continue;
            if (!out.empty() && out.back().same_style(run))
                out.back().text += run.text;
            else
                out.push_back(run);
        }
        return out;
    }

    static void append_text(std::vector<InlineRun>& runs, const InlineRun& style,
                            const std::string& text)
    {
        if (text.empty())
            return;
        InlineRun run = style;
        run.text = text;
        runs.push_back(run);
    }

    enum class CharClass { Whitespace, Punctuation, Other };

    static CharClass classify(const std::string& text, size_t index)
    {
        if (index >= text.size())
            return CharClass::Whitespace;
        const unsigned char c = static_cast<unsigned char>(text[index]);
        if (ascii_space(c))
            return CharClass::Whitespace;
        if (ascii_punct(c))
            return CharClass::Punctuation;
        return CharClass::Other;  // letters, digits, and all non-ASCII bytes
    }

    static CharClass classify_before(const std::string& text, size_t index)
    {
        if (index == 0)
            return CharClass::Whitespace;
        return classify(text, index - 1);
    }

    // CommonMark-flavored flanking checks for * and _ delimiter runs.
    static bool can_open(const std::string& text, size_t start, size_t len, char delim)
    {
        const CharClass after = classify(text, start + len);
        const CharClass before = classify_before(text, start);
        const bool left_flanking =
            after != CharClass::Whitespace &&
            (after != CharClass::Punctuation ||
             before == CharClass::Whitespace || before == CharClass::Punctuation);
        if (!left_flanking)
            return false;
        if (delim == '_' && before == CharClass::Other)
            return false;  // no intraword emphasis with underscore
        return true;
    }

    static bool can_close(const std::string& text, size_t start, size_t len, char delim)
    {
        const CharClass before = classify_before(text, start);
        const CharClass after = classify(text, start + len);
        const bool right_flanking =
            before != CharClass::Whitespace &&
            (before != CharClass::Punctuation ||
             after == CharClass::Whitespace || after == CharClass::Punctuation);
        if (!right_flanking)
            return false;
        if (delim == '_' && after == CharClass::Other)
            return false;
        return true;
    }

    static size_t delimiter_run_length(const std::string& text, size_t index)
    {
        const char delim = text[index];
        size_t len = 0;
        while (index + len < text.size() && text[index + len] == delim)
            ++len;
        return len;
    }

    void parse_into(const std::string& text, const InlineRun& style,
                    std::vector<InlineRun>& runs) const
    {
        std::string plain;
        auto flush = [&]() {
            append_text(runs, style, plain);
            plain.clear();
        };

        for (size_t i = 0; i < text.size();) {
            const char c = text[i];

            if (c == '\\' && i + 1 < text.size()) {
                plain.push_back(text[i + 1]);
                i += 2;
                continue;
            }

            if (c == '`') {
                const size_t close = text.find('`', i + 1);
                if (close != std::string::npos) {
                    flush();
                    InlineRun code = style;
                    code.code = true;
                    append_text(runs, code, text.substr(i + 1, close - i - 1));
                    i = close + 1;
                    continue;
                }
            }

            if (c == '~' && i + 1 < text.size() && text[i + 1] == '~') {
                const size_t close = find_strike_close(text, i + 2);
                if (close != std::string::npos && close > i + 2) {
                    flush();
                    InlineRun strike = style;
                    strike.strike = true;
                    parse_into(text.substr(i + 2, close - i - 2), strike, runs);
                    i = close + 2;
                    continue;
                }
            }

            if (c == '!' && i + 1 < text.size() && text[i + 1] == '[') {
                std::string alt, url;
                size_t consumed = 0;
                if (parse_bracket_pair(text, i + 1, alt, url, consumed)) {
                    flush();
                    InlineRun note = style;
                    note.italic = true;
                    std::string label = !alt.empty() ? alt : url;
                    append_text(runs, note,
                                label.empty() ? "[Image omitted]" : "[Image omitted: " + label + "]");
                    i += 1 + consumed;
                    continue;
                }
            }

            if (c == '[') {
                if (try_parse_link(text, i, style, runs, plain))
                    continue;
            }

            if (c == '<') {
                if (try_parse_autolink(text, i, style, runs, plain))
                    continue;
            }

            if ((c == 'h') && (text.compare(i, 7, "http://") == 0 ||
                               text.compare(i, 8, "https://") == 0)) {
                size_t end = i;
                while (end < text.size() &&
                       !ascii_space(static_cast<unsigned char>(text[end])) && text[end] != '<')
                    ++end;
                std::string url = text.substr(i, end - i);
                while (!url.empty() && std::string(".,;:)]").find(url.back()) != std::string::npos)
                    url.pop_back();
                if (url.size() > 8) {
                    flush();
                    InlineRun link = style;
                    link.link = true;
                    link.url = url;
                    append_text(runs, link, url);
                    i += url.size();
                    continue;
                }
            }

            if (c == '*' || c == '_') {
                const size_t run_len = delimiter_run_length(text, i);
                const size_t use_len = std::min<size_t>(run_len, 3);
                if (can_open(text, i, run_len, c)) {
                    const size_t close = find_emphasis_close(text, i + run_len, c, use_len);
                    if (close != std::string::npos) {
                        flush();
                        InlineRun inner = style;
                        if (use_len >= 2)
                            inner.bold = true;
                        if (use_len == 1 || use_len == 3)
                            inner.italic = true;
                        parse_into(text.substr(i + use_len, close - i - use_len), inner, runs);
                        i = close + use_len;
                        continue;
                    }
                }
                plain += text.substr(i, run_len);
                i += run_len;
                continue;
            }

            plain.push_back(c);
            ++i;
        }
        flush();
    }

    static size_t find_strike_close(const std::string& text, size_t from)
    {
        for (size_t i = from; i + 1 < text.size(); ++i) {
            if (text[i] == '~' && text[i + 1] == '~' &&
                !ascii_space(static_cast<unsigned char>(text[i - 1])))
                return i;
        }
        return std::string::npos;
    }

    static size_t find_emphasis_close(const std::string& text, size_t from, char delim,
                                      size_t length)
    {
        for (size_t i = from; i < text.size(); ++i) {
            if (text[i] == '\\') {
                ++i;
                continue;
            }
            if (text[i] != delim)
                continue;
            const size_t run_len = delimiter_run_length(text, i);
            if (run_len >= length && can_close(text, i, run_len, delim) && i > from)
                return i;
            i += run_len - 1;
        }
        return std::string::npos;
    }

    // Parses [text](url) starting at `start` (which must point at '[').
    static bool parse_bracket_pair(const std::string& text, size_t start, std::string& label,
                                   std::string& url, size_t& consumed)
    {
        if (start >= text.size() || text[start] != '[')
            return false;
        int depth = 1;
        size_t i = start + 1;
        while (i < text.size() && depth > 0) {
            if (text[i] == '\\')
                ++i;
            else if (text[i] == '[')
                ++depth;
            else if (text[i] == ']')
                --depth;
            ++i;
        }
        if (depth != 0 || i >= text.size() || text[i] != '(')
            return false;
        label = text.substr(start + 1, i - start - 2);
        const size_t close = text.find(')', i + 1);
        if (close == std::string::npos)
            return false;
        url = text.substr(i + 1, close - i - 1);
        consumed = close - start + 1;
        return true;
    }

    bool try_parse_link(const std::string& text, size_t& i, const InlineRun& style,
                        std::vector<InlineRun>& runs, std::string& plain) const
    {
        auto emit_link = [&](const std::string& label, const std::string& url, size_t consumed) {
            append_text(runs, style, plain);
            plain.clear();
            InlineRun link = style;
            link.link = true;
            link.url = url;
            append_text(runs, link, label.empty() ? url : label);
            i += consumed;
        };

        std::string label, url;
        size_t consumed = 0;
        if (parse_bracket_pair(text, i, label, url, consumed)) {
            emit_link(label, url, consumed);
            return true;
        }

        // Reference forms: [label][id], [label][], [id]
        const size_t label_close = text.find(']', i + 1);
        if (label_close == std::string::npos)
            return false;
        label = text.substr(i + 1, label_close - i - 1);
        std::string ref_id = label;
        size_t end = label_close + 1;
        if (end < text.size() && text[end] == '[') {
            const size_t ref_close = text.find(']', end + 1);
            if (ref_close == std::string::npos)
                return false;
            const std::string explicit_id = text.substr(end + 1, ref_close - end - 1);
            if (!explicit_id.empty())
                ref_id = explicit_id;
            end = ref_close + 1;
        }
        auto it = references_.find(normalize_label(ref_id));
        if (it == references_.end())
            return false;
        emit_link(label, it->second, end - i);
        return true;
    }

    bool try_parse_autolink(const std::string& text, size_t& i, const InlineRun& style,
                            std::vector<InlineRun>& runs, std::string& plain) const
    {
        const size_t close = text.find('>', i + 1);
        if (close == std::string::npos)
            return false;
        const std::string body = text.substr(i + 1, close - i - 1);
        static const std::regex email(R"(^[^@\s<>]+@[^@\s<>]+\.[^@\s<>]+$)");
        std::string url;
        if (body.rfind("http://", 0) == 0 || body.rfind("https://", 0) == 0)
            url = body;
        else if (std::regex_match(body, email))
            url = "mailto:" + body;
        else
            return false;
        append_text(runs, style, plain);
        plain.clear();
        InlineRun link = style;
        link.link = true;
        link.url = url;
        append_text(runs, link, body);
        i = close + 1;
        return true;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Block parser
// ─────────────────────────────────────────────────────────────────────────────

class MarkdownParser {
public:
    std::vector<Block> parse(const std::string& markdown, bool include_unrendered_html) const
    {
        const std::string normalized = normalize_html(markdown, include_unrendered_html);
        std::vector<std::string> lines;
        {
            std::stringstream stream(normalized);
            std::string line;
            while (std::getline(stream, line))
                lines.push_back(expand_tabs(line));
        }

        std::map<std::string, std::string> references;
        extract_references(lines, references);
        InlineParser inline_parser(references);

        std::vector<Block> blocks;
        size_t i = 0;
        while (i < lines.size()) {
            const std::string& line = lines[i];
            if (is_blank(line)) {
                ++i;
                continue;
            }
            if (is_fence(line)) {
                blocks.push_back(parse_code_block(lines, i));
                continue;
            }
            if (Block heading; try_parse_atx_heading(line, inline_parser, heading)) {
                blocks.push_back(std::move(heading));
                ++i;
                continue;
            }
            if (is_horizontal_rule(line)) {
                Block hr;
                hr.type = BlockType::HorizontalRule;
                blocks.push_back(std::move(hr));
                ++i;
                continue;
            }
            if (Block heading; try_parse_setext_heading(lines, i, inline_parser, heading)) {
                blocks.push_back(std::move(heading));
                continue;
            }
            if (trimmed_start(line).rfind('>', 0) == 0) {
                blocks.push_back(parse_blockquote(lines, i, inline_parser));
                continue;
            }
            if (Block table; try_parse_table(lines, i, inline_parser, table)) {
                blocks.push_back(std::move(table));
                continue;
            }
            if (int level, number; is_list_item(line, level, number) >= 0) {
                blocks.push_back(parse_list(lines, i, inline_parser));
                continue;
            }
            blocks.push_back(parse_paragraph(lines, i, inline_parser));
        }
        return blocks;
    }

private:
    static std::string expand_tabs(const std::string& line)
    {
        std::string out;
        for (char c : line) {
            if (c == '\t')
                out += "    ";
            else
                out.push_back(c);
        }
        return out;
    }

    static bool is_blank(const std::string& line)
    {
        return line.find_first_not_of(" \t") == std::string::npos;
    }

    static std::string trimmed_start(const std::string& line)
    {
        const auto first = line.find_first_not_of(" \t");
        return first == std::string::npos ? "" : line.substr(first);
    }

    static std::string trim(const std::string& text)
    {
        const auto first = text.find_first_not_of(" \t");
        if (first == std::string::npos)
            return "";
        const auto last = text.find_last_not_of(" \t");
        return text.substr(first, last - first + 1);
    }

    static bool is_fence(const std::string& line)
    {
        const std::string t = trimmed_start(line);
        return t.rfind("```", 0) == 0 || t.rfind("~~~", 0) == 0;
    }

    static bool is_horizontal_rule(const std::string& line)
    {
        static const std::regex hr(R"(^\s{0,3}([-*_])(?:\s*\1){2,}\s*$)");
        return std::regex_match(line, hr);
    }

    // Returns marker kind: -1 not a list item, 0 unordered, 1 ordered.
    static int is_list_item(const std::string& line, int& level, int& number)
    {
        static const std::regex item(R"(^(\s*)([-+*]|\d+[.)])\s+(.*)$)");
        std::smatch m;
        if (!std::regex_match(line, m, item))
            return -1;
        level = std::min(6, static_cast<int>(m[1].str().size()) / 2);
        const std::string marker = m[2].str();
        if (ascii_digit(static_cast<unsigned char>(marker[0]))) {
            number = std::atoi(marker.c_str());
            return 1;
        }
        number = 0;
        return 0;
    }

    static bool starts_new_block(const std::string& line)
    {
        if (is_blank(line) || is_fence(line) || is_horizontal_rule(line))
            return true;
        static const std::regex atx(R"(^#{1,6}(?:\s+|$).*$)");
        if (std::regex_match(line, atx))
            return true;
        if (trimmed_start(line).rfind('>', 0) == 0)
            return true;
        int level, number;
        return is_list_item(line, level, number) >= 0;
    }

    static void extract_references(std::vector<std::string>& lines,
                                   std::map<std::string, std::string>& references)
    {
        static const std::regex definition(
            R"(^\s{0,3}\[([^\]]+)\]:\s*(\S+)(?:\s+(?:"[^"]*"|'[^']*'|\([^)]*\)))?\s*$)");
        bool in_fence = false;
        for (auto& line : lines) {
            if (is_fence(line)) {
                in_fence = !in_fence;
                continue;
            }
            if (in_fence)
                continue;
            std::smatch m;
            if (std::regex_match(line, m, definition)) {
                const std::string key = InlineParser::normalize_label(m[1].str());
                references.emplace(key, m[2].str());  // first definition wins
                line.clear();
            }
        }
    }

    static bool try_parse_atx_heading(const std::string& line, const InlineParser& inline_parser,
                                      Block& out)
    {
        static const std::regex atx(R"(^(#{1,6})(?:\s+|$)(.*)$)");
        std::smatch m;
        if (!std::regex_match(line, m, atx))
            return false;
        std::string text = m[2].str();
        // Strip a trailing closing-hash run.
        static const std::regex trailing(R"(\s+#+\s*$)");
        text = std::regex_replace(text, trailing, "");
        out.type = BlockType::Heading;
        out.heading_level = static_cast<int>(m[1].str().size());
        out.runs = inline_parser.parse(trim(text));
        return true;
    }

    static bool try_parse_setext_heading(const std::vector<std::string>& lines, size_t& i,
                                         const InlineParser& inline_parser, Block& out)
    {
        if (i + 1 >= lines.size())
            return false;
        static const std::regex underline(R"(^\s{0,3}(=+|-+)\s*$)");
        std::smatch m;
        if (!std::regex_match(lines[i + 1], m, underline))
            return false;
        const std::string& text = lines[i];
        if (starts_new_block(text) || text.find('|') != std::string::npos)
            return false;
        out.type = BlockType::Heading;
        out.heading_level = m[1].str()[0] == '=' ? 1 : 2;
        out.runs = inline_parser.parse(trim(text));
        i += 2;
        return true;
    }

    static Block parse_code_block(const std::vector<std::string>& lines, size_t& i)
    {
        Block block;
        block.type = BlockType::CodeBlock;
        const std::string opener = trimmed_start(lines[i]);
        const std::string marker = opener.substr(0, 3);
        block.language = trim(opener.substr(3));
        ++i;
        while (i < lines.size()) {
            const std::string t = trimmed_start(lines[i]);
            if (t.rfind(marker, 0) == 0) {
                ++i;
                break;
            }
            block.code_lines.push_back(lines[i]);
            ++i;
        }
        return block;
    }

    static Block parse_blockquote(const std::vector<std::string>& lines, size_t& i,
                                  const InlineParser& inline_parser)
    {
        std::string text;
        while (i < lines.size()) {
            std::string t = trimmed_start(lines[i]);
            if (t.rfind('>', 0) != 0)
                break;
            t.erase(0, 1);
            if (!t.empty() && t[0] == ' ')
                t.erase(0, 1);
            if (!text.empty())
                text += " ";
            text += t;
            ++i;
        }
        Block block;
        block.type = BlockType::BlockQuote;
        block.runs = inline_parser.parse(trim(text));
        return block;
    }

    static std::vector<std::string> split_table_cells(const std::string& line)
    {
        std::string body = trim(line);
        if (!body.empty() && body.front() == '|')
            body.erase(0, 1);
        if (!body.empty() && body.back() == '|' &&
            (body.size() < 2 || body[body.size() - 2] != '\\'))
            body.pop_back();
        std::vector<std::string> cells;
        std::string current;
        for (size_t i = 0; i < body.size(); ++i) {
            if (body[i] == '\\' && i + 1 < body.size() && body[i + 1] == '|') {
                current.push_back('|');
                ++i;
            } else if (body[i] == '|') {
                cells.push_back(trim(current));
                current.clear();
            } else {
                current.push_back(body[i]);
            }
        }
        cells.push_back(trim(current));
        return cells;
    }

    static bool has_unescaped_pipe(const std::string& line)
    {
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] == '|' && (i == 0 || line[i - 1] != '\\'))
                return true;
        }
        return false;
    }

    static bool try_parse_table(const std::vector<std::string>& lines, size_t& i,
                                const InlineParser& inline_parser, Block& out)
    {
        if (i + 1 >= lines.size() || !has_unescaped_pipe(lines[i]))
            return false;
        const std::vector<std::string> separator_cells = split_table_cells(lines[i + 1]);
        if (separator_cells.empty())
            return false;
        std::vector<ColumnAlignment> alignments;
        static const std::regex separator(R"(^(:?)-{1,}(:?)$)");
        for (const auto& cell : separator_cells) {
            std::smatch m;
            if (cell.size() < 3 || !std::regex_match(cell, m, separator))
                return false;
            if (m[1].length() > 0 && m[2].length() > 0)
                alignments.push_back(ColumnAlignment::Center);
            else if (m[2].length() > 0)
                alignments.push_back(ColumnAlignment::Right);
            else
                alignments.push_back(ColumnAlignment::Left);
        }

        std::vector<std::string> header_cells = split_table_cells(lines[i]);
        const size_t columns = std::max(header_cells.size(), alignments.size());
        header_cells.resize(columns);
        alignments.resize(columns, ColumnAlignment::Left);

        out.type = BlockType::Table;
        out.alignments = alignments;
        for (const auto& cell : header_cells)
            out.header_cells.push_back(inline_parser.parse(cell));

        i += 2;
        while (i < lines.size()) {
            const std::string& line = lines[i];
            if (is_blank(line) || is_fence(line) || is_horizontal_rule(line) ||
                trimmed_start(line).rfind('>', 0) == 0 || !has_unescaped_pipe(line))
                break;
            static const std::regex atx(R"(^#{1,6}(?:\s+|$).*$)");
            if (std::regex_match(line, atx))
                break;
            std::vector<std::string> cells = split_table_cells(line);
            cells.resize(columns);
            std::vector<std::vector<InlineRun>> row;
            for (const auto& cell : cells)
                row.push_back(inline_parser.parse(cell));
            out.rows.push_back(std::move(row));
            ++i;
        }
        return true;
    }

    static Block parse_list(const std::vector<std::string>& lines, size_t& i,
                            const InlineParser& inline_parser)
    {
        Block block;
        block.type = BlockType::List;
        int first_level, first_number;
        block.ordered = is_list_item(lines[i], first_level, first_number) == 1;

        while (i < lines.size()) {
            int level, number;
            const int kind = is_list_item(lines[i], level, number);
            if (kind < 0 || (kind == 1) != block.ordered)
                break;

            static const std::regex item(R"(^(\s*)([-+*]|\d+[.)])\s+(.*)$)");
            std::smatch m;
            std::regex_match(lines[i], m, item);
            std::string text = m[3].str();
            ++i;

            ListItem entry;
            entry.level = level;
            entry.number = number;
            const std::string trimmed = trim(text);
            if (trimmed.rfind("[ ]", 0) == 0) {
                entry.task_state = 0;
                text = trim(trimmed.substr(3));
            } else if (trimmed.rfind("[x]", 0) == 0 || trimmed.rfind("[X]", 0) == 0) {
                entry.task_state = 1;
                text = trim(trimmed.substr(3));
            }

            // Continuation lines: more-indented plain text is appended.
            while (i < lines.size()) {
                const std::string& next = lines[i];
                int next_level, next_number;
                if (is_blank(next) || is_list_item(next, next_level, next_number) >= 0 ||
                    starts_new_block(next))
                    break;
                const size_t indent = next.find_first_not_of(' ');
                if (indent == std::string::npos ||
                    static_cast<int>(indent) <= level * 2)
                    break;
                text += " " + trim(next);
                ++i;
            }

            entry.runs = inline_parser.parse(trim(text));
            block.items.push_back(std::move(entry));

            if (i < lines.size() && is_blank(lines[i]))
                break;
        }
        return block;
    }

    static Block parse_paragraph(const std::vector<std::string>& lines, size_t& i,
                                 const InlineParser& inline_parser)
    {
        std::string text = trim(lines[i]);
        ++i;
        while (i < lines.size() && !starts_new_block(lines[i]) &&
               !(i + 1 < lines.size() &&
                 std::regex_match(lines[i + 1], std::regex(R"(^\s{0,3}(=+|-+)\s*$)")))) {
            if (has_unescaped_pipe(lines[i]) && i + 1 < lines.size())
                break;  // could be a table header
            text += " " + trim(lines[i]);
            ++i;
        }
        Block block;
        block.type = BlockType::Paragraph;
        block.runs = inline_parser.parse(text);
        return block;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Fonts: family catalog with base-14 defaults plus fonts discovered from the
// machine, and per-script fallback resolution driven by
// Font::is_text_representable.
// ─────────────────────────────────────────────────────────────────────────────

std::string normalize_font_key(const std::string& name)
{
    std::string out;
    for (char c : name) {
        if (ascii_alnum(static_cast<unsigned char>(c))) {
            if (c >= 'A' && c <= 'Z')
                c = static_cast<char>(c - 'A' + 'a');
            out.push_back(c);
        }
    }
    return out;
}

struct FontFamily {
    std::string display_name;
    std::string regular;
    std::string bold;
    std::string italic;
    std::string bold_italic;

    const std::string& face(bool want_bold, bool want_italic) const
    {
        if (want_bold && want_italic && !bold_italic.empty())
            return bold_italic;
        if (want_bold && !bold.empty())
            return bold;
        if (want_italic && !italic.empty())
            return italic;
        if (want_bold && !bold_italic.empty())
            return bold_italic;
        return regular;
    }
};

class FontCatalog {
public:
    FontCatalog()
    {
        add_core("Times", {"Times", "Times-Roman", "Times-Bold", "Times-Italic", "Times-BoldItalic"},
                 {"timesroman", "serif"});
        add_core("Helvetica",
                 {"Helvetica", "Helvetica", "Helvetica-Bold", "Helvetica-Oblique",
                  "Helvetica-BoldOblique"},
                 {"sans", "sansserif"});
        add_core("Courier",
                 {"Courier", "Courier", "Courier-Bold", "Courier-Oblique", "Courier-BoldOblique"},
                 {"mono", "monospace"});
        discover_installed_fonts();
    }

    const FontFamily* find(const std::string& name) const
    {
        auto it = families_.find(normalize_font_key(name));
        if (it != families_.end())
            return &it->second;
        auto alias = aliases_.find(normalize_font_key(name));
        if (alias != aliases_.end())
            return &families_.at(alias->second);
        return nullptr;
    }

    const FontFamily& require(const std::string& name) const
    {
        const FontFamily* family = find(name);
        if (!family)
            throw CommandLineException(
                "Unknown font family: " + name +
                ". Use --list-font-families to see what is available.");
        return *family;
    }

    void print_families() const
    {
        std::cout << "Core PDF fonts:" << std::endl;
        std::cout << "  Times" << std::endl << "  Helvetica" << std::endl
                  << "  Courier" << std::endl;
        std::vector<std::string> others;
        for (const auto& [key, family] : families_) {
            if (key != "times" && key != "helvetica" && key != "courier")
                others.push_back(family.display_name);
        }
        std::sort(others.begin(), others.end());
        if (!others.empty()) {
            std::cout << "Other fonts available to APDFL on this machine (" << others.size()
                      << "):" << std::endl;
            for (const auto& name : others)
                std::cout << "  " << name << std::endl;
        }
        std::cout << "Pass any of these to --font-family, --heading-font-family, or\n"
                     "--code-font-family. Times, Helvetica, and Courier remain the most\n"
                     "portable choices." << std::endl;
    }

private:
    std::map<std::string, FontFamily> families_;
    std::map<std::string, std::string> aliases_;

    void add_core(const std::string& display,
                  const std::vector<std::string>& display_and_faces,
                  const std::vector<std::string>& alias_names)
    {
        FontFamily family;
        family.display_name = display;
        family.regular = display_and_faces[1];
        family.bold = display_and_faces[2];
        family.italic = display_and_faces[3];
        family.bold_italic = display_and_faces[4];
        const std::string key = normalize_font_key(display);
        families_[key] = family;
        for (const auto& alias : alias_names)
            aliases_[alias] = key;
    }

    // Groups every font APDFL can see into families keyed by a name with the
    // style suffix stripped, so "--font-family Arial" resolves Arial-BoldMT
    // and friends.
    void discover_installed_fonts()
    {
        std::vector<Font> installed;
        try {
            installed = Font::get_font_list();
        } catch (const std::exception&) {
            return;  // font enumeration is best-effort
        }
        for (const auto& font : installed) {
            std::string name;
            try {
                name = font.get_name();
            } catch (const std::exception&) {
                continue;
            }
            if (name.empty())
                continue;

            std::string base = name;
            bool bold = false, italic = false;
            strip_style_suffix(base, bold, italic);

            const std::string key = normalize_font_key(base);
            if (key.empty())
                continue;
            auto [it, inserted] = families_.emplace(key, FontFamily{});
            FontFamily& family = it->second;
            if (inserted)
                family.display_name = display_name_for(base);
            std::string& slot = bold && italic ? family.bold_italic
                                : bold         ? family.bold
                                : italic       ? family.italic
                                               : family.regular;
            if (slot.empty())
                slot = name;
        }
        // Families discovered without a regular face fall back to any face.
        for (auto& [key, family] : families_) {
            if (family.regular.empty()) {
                family.regular = !family.bold.empty()          ? family.bold
                                 : !family.italic.empty()      ? family.italic
                                                               : family.bold_italic;
            }
        }
    }

    static void strip_style_suffix(std::string& base, bool& bold, bool& italic)
    {
        static const std::pair<const char*, std::pair<bool, bool>> suffixes[] = {
            {"-BoldOblique", {true, true}}, {"-BoldItalic", {true, true}},
            {"BoldOblique", {true, true}},  {"BoldItalic", {true, true}},
            {"-Oblique", {false, true}},    {"-Italic", {false, true}},
            {"-Bold", {true, false}},       {"-Regular", {false, false}},
            {"Oblique", {false, true}},     {"Italic", {false, true}},
            {"Bold", {true, false}},        {"Regular", {false, false}},
        };
        // Trim foundry adornments first.
        for (const char* adornment : {"PSMT", "PS", "MT"}) {
            const size_t len = std::strlen(adornment);
            if (base.size() > len && base.compare(base.size() - len, len, adornment) == 0)
                base.erase(base.size() - len);
        }
        for (const auto& [suffix, style] : suffixes) {
            const size_t len = std::strlen(suffix);
            if (base.size() > len && base.compare(base.size() - len, len, suffix) == 0) {
                base.erase(base.size() - len);
                bold = style.first;
                italic = style.second;
                break;
            }
        }
        while (!base.empty() && (base.back() == '-' || base.back() == ','))
            base.pop_back();
        std::replace(base.begin(), base.end(), '-', ' ');
    }

    static std::string display_name_for(const std::string& base)
    {
        std::string out;
        for (size_t i = 0; i < base.size(); ++i) {
            const char c = base[i];
            if (i > 0 && c >= 'A' && c <= 'Z' && base[i - 1] >= 'a' &&
                base[i - 1] <= 'z')
                out.push_back(' ');
            out.push_back(c);
        }
        return out;
    }
};

enum class FontRole { Body, Heading, Code };

class FontSet {
public:
    FontSet(const FontCatalog& catalog, const ConversionOptions& options) : catalog_(catalog)
    {
        body_ = &catalog.require(options.font_family);
        heading_ = &catalog.require(options.heading_font_family);
        code_ = &catalog.require(options.code_font_family);

        fallback_names_ = options.fallback_fonts;
        for (const char* name :
             {"Arial", "Times New Roman", "Calibri", "Segoe UI", "DejaVu Sans", "Noto Sans",
              "Microsoft YaHei", "SimSun", "Microsoft JhengHei", "Malgun Gothic", "Yu Gothic",
              "Noto Sans CJK SC", "Noto Sans CJK JP", "Noto Sans CJK KR", "Arial Unicode MS",
              "Helvetica Neue", "PingFang SC", "Hiragino Sans", "Apple SD Gothic Neo"}) {
            fallback_names_.push_back(name);
        }
    }

    // Resolves the font for one styled piece of text. `sample` is a UTF-8
    // string used to check glyph coverage for fallback scripts.
    Font& resolve(const InlineRun& run, FontRole role, const std::string& sample)
    {
        if (!sample.empty()) {
            size_t index = 0;
            const unsigned int cp = next_code_point(sample, index);
            if (is_cjk_code_point(cp) || is_cyrillic_or_greek_code_point(cp)) {
                Font* fallback = resolve_fallback(sample, run);
                if (fallback)
                    return *fallback;
            }
        }
        if (run.code || role == FontRole::Code)
            return get_face(code_->face(false, false));
        const FontFamily* family = role == FontRole::Heading ? heading_ : body_;
        return get_face(family->face(run.bold, run.italic));
    }

private:
    const FontCatalog& catalog_;
    const FontFamily* body_;
    const FontFamily* heading_;
    const FontFamily* code_;
    std::vector<std::string> fallback_names_;
    std::map<std::string, std::unique_ptr<Font>> cache_;
    std::map<std::string, bool> failed_;

    Font* try_get_face(const std::string& face_name)
    {
        if (face_name.empty() || failed_.count(face_name))
            return nullptr;
        auto it = cache_.find(face_name);
        if (it != cache_.end())
            return it->second.get();
        for (FontCreateFlags flags :
             {FontCreateFlags::Embedded | FontCreateFlags::Subset, FontCreateFlags::Subset}) {
            try {
                auto font = std::make_unique<Font>(face_name, flags);
                Font* result = font.get();
                cache_.emplace(face_name, std::move(font));
                return result;
            } catch (const std::exception&) {
                continue;
            }
        }
        failed_[face_name] = true;
        return nullptr;
    }

    Font& get_face(const std::string& face_name)
    {
        Font* font = try_get_face(face_name);
        if (!font)
            throw CommandLineException(
                "Could not create font \"" + face_name +
                "\". Choose another family with --font-family or --list-font-families.");
        return *font;
    }

    Font* resolve_fallback(const std::string& sample, const InlineRun& run)
    {
        // Prefer the configured face if it already covers the text.
        const FontFamily* preferred = run.code ? code_ : body_;
        if (Font* face = try_get_face(preferred->face(run.bold, run.italic))) {
            try {
                if (face->is_text_representable(sample))
                    return face;
            } catch (const std::exception&) {
            }
        }
        for (const auto& name : fallback_names_) {
            const FontFamily* family = catalog_.find(name);
            Font* font = family ? try_get_face(family->face(run.bold, run.italic)) : nullptr;
            if (!font && family)
                font = try_get_face(family->regular);
            if (!font)
                font = try_get_face(name);
            if (!font)
                continue;
            try {
                if (font->is_text_representable(sample))
                    return font;
            } catch (const std::exception&) {
            }
        }
        return nullptr;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Theme
// ─────────────────────────────────────────────────────────────────────────────

struct Theme {
    double page_width = 612;
    double page_height = 792;
    double margin = 72;

    static constexpr double body_size = 11;
    static constexpr double body_line_height = 15;
    static constexpr double paragraph_space_after = 7;

    static constexpr double heading_sizes[6] = {24, 20, 17, 14, 12, 11};
    static constexpr double heading_space_before[6] = {0, 14, 12, 10, 8, 8};
    static constexpr double heading_space_after[6] = {10, 8, 7, 6, 5, 5};

    static constexpr double list_indent_per_level = 18;
    static constexpr double list_marker_width = 22;
    static constexpr double list_item_space_after = 2;
    static constexpr double list_space_after = 5;

    static constexpr double code_size = 9.5;
    static constexpr double code_line_height = 12.5;
    static constexpr double code_title_size = 9;
    static constexpr double code_title_line_height = 14;
    static constexpr double code_padding = 8;
    static constexpr double code_background_gray = 0.94;
    static constexpr double inline_code_background_gray = 0.92;
    static constexpr double inline_code_padding = 2;
    static constexpr double code_space_before = 6;
    static constexpr double code_space_after = 8;

    static constexpr double quote_indent = 18;
    static constexpr double quote_space_before = 5;
    static constexpr double quote_space_after = 8;

    static constexpr double rule_space_before = 8;
    static constexpr double rule_space_after = 10;

    static constexpr double table_size = 9.5;
    static constexpr double table_line_height = 12;
    static constexpr double table_cell_padding = 4;
    static constexpr double table_border_width = 0.5;
    static constexpr double table_space_before = 8;
    static constexpr double table_space_after = 9;

    double content_width() const { return page_width - 2 * margin; }

    static Theme create(const ConversionOptions& options)
    {
        Theme theme;
        auto [w, h] = resolve_page_size(options.page_size);
        if (options.orientation == Orientation::Landscape && h > w)
            std::swap(w, h);
        else if (options.orientation == Orientation::Portrait && w > h)
            std::swap(w, h);
        theme.page_width = w;
        theme.page_height = h;
        theme.margin = options.margin;
        if (theme.content_width() < 72 || theme.page_height - 2 * theme.margin < 72)
            throw CommandLineException("Page size leaves less than 72 points of usable space.");
        return theme;
    }
};

constexpr double Theme::heading_sizes[6];
constexpr double Theme::heading_space_before[6];
constexpr double Theme::heading_space_after[6];

// ─────────────────────────────────────────────────────────────────────────────
// Renderer
// ─────────────────────────────────────────────────────────────────────────────

// One wrapped piece of a line: a styled run fragment with a resolved width.
struct LinePiece {
    InlineRun run;
    double width = 0;
};

struct WrappedLine {
    std::vector<LinePiece> pieces;
    double width = 0;
};

class Renderer {
public:
    Renderer(Document& document, const Theme& theme, const ConversionOptions& options,
             FontSet& fonts)
        : document_(document), theme_(theme), options_(options), fonts_(fonts)
    {
        root_ = std::make_unique<StructTreeRoot>(document.create_struct_tree_root());
        document_element_ = std::make_unique<StructElement>(root_->add_child("Document"));

        PDFString lang(options.language, document_, false);
        document_.get_root()->put("Lang", lang);

        new_page();
    }

    void render(const std::vector<Block>& blocks)
    {
        for (const auto& block : blocks) {
            switch (block.type) {
            case BlockType::Heading: render_heading(block); break;
            case BlockType::Paragraph: render_paragraph(block); break;
            case BlockType::CodeBlock: render_code_block(block); break;
            case BlockType::List: render_list(block); break;
            case BlockType::BlockQuote: render_blockquote(block); break;
            case BlockType::Table: render_table(block); break;
            case BlockType::HorizontalRule: render_horizontal_rule(); break;
            }
        }
        finish_page();
    }

    int page_count() const { return page_count_; }

private:
    Document& document_;
    const Theme& theme_;
    const ConversionOptions& options_;
    FontSet& fonts_;
    std::unique_ptr<StructTreeRoot> root_;
    std::unique_ptr<StructElement> document_element_;

    std::optional<Page> page_;
    std::optional<Content> content_;
    double cursor_y_ = 0;
    int page_count_ = 0;

    // ── Page management ────────────────────────────────────────

    void finish_page()
    {
        if (page_)
            page_->update_content();
    }

    void new_page()
    {
        finish_page();
        const int insert_after = page_count_ == 0 ? Document::before_first_page : page_count_ - 1;
        page_.emplace(document_.create_page(insert_after,
                                            Rect{0, 0, theme_.page_width, theme_.page_height}));
        content_.emplace(page_->get_content());
        cursor_y_ = theme_.page_height - theme_.margin;
        ++page_count_;
    }

    bool at_top() const { return cursor_y_ >= theme_.page_height - theme_.margin - 0.01; }

    void ensure_space(double points)
    {
        if (cursor_y_ - points < theme_.margin && !at_top())
            new_page();
    }

    void move_down(double points)
    {
        if (at_top())
            return;
        if (cursor_y_ - points < theme_.margin)
            new_page();
        else
            cursor_y_ -= points;
    }

    // ── Tagged and artifact content ────────────────────────────

    void add_tagged(Element& element, const std::string& tag, StructElement& owner)
    {
        Content inner;
        inner.add_element(element);
        PDFDict props(document_, false);  // empty: the MCID is assigned by the library
        Container container(tag, props, false);
        container.set_content(inner);
        content_->add_element(container);
        owner.add_marked_content_ref(*page_, container);
    }

    void add_artifact(Element& element)
    {
        Content inner;
        inner.add_element(element);
        PDFDict props(document_, false);
        PDFName layout("Layout", document_, false);
        props.put("Type", layout);
        Container container("Artifact", props, false);
        container.set_content(inner);
        content_->add_element(container);
    }

    // ── Measurement and wrapping ───────────────────────────────

    double measure(const InlineRun& run, FontRole role, const std::string& text, double size)
    {
        if (text.empty())
            return 0;
        Font& font = fonts_.resolve(run, role, text);
        double measured;
        try {
            measured = font.measure_text_width(text, size);
        } catch (const std::exception&) {
            measured = static_cast<double>(text.size()) * size * 0.55;
        }
        // Cap advances that some fonts report too wide for bullets and
        // non-Latin scripts, mirroring the .NET sample's corrections.
        double cap = 0;
        size_t index = 0;
        size_t code_points = 0;
        bool needs_cap = true;
        while (index < text.size()) {
            const unsigned int cp = next_code_point(text, index);
            ++code_points;
            if (cp == 0x2022)
                cap += size * 0.35;
            else if (is_cjk_code_point(cp))
                cap += size;
            else if (is_cyrillic_or_greek_code_point(cp))
                cap += estimate_cyrillic_or_greek_advance(cp, size);
            else
                needs_cap = false;
        }
        if (needs_cap && code_points > 0)
            return std::min(measured, cap);
        return measured;
    }

    static double estimate_cyrillic_or_greek_advance(unsigned int cp, double size)
    {
        const bool upper = (cp >= 0x0400 && cp <= 0x042F) || (cp >= 0x0391 && cp <= 0x03A9);
        static const unsigned int wide[] = {0x0436, 0x043C, 0x0448, 0x0449, 0x044E, 0x044B};
        static const unsigned int narrow[] = {0x0456, 0x0457, 0x0458, 0x03B9};
        for (unsigned int w : wide)
            if (cp == w)
                return size * 0.74;
        for (unsigned int n : narrow)
            if (cp == n)
                return size * 0.32;
        return upper ? size * 0.70 : size * 0.60;
    }

    struct Token {
        InlineRun run;   // style + token text
        bool space = false;
        bool cjk = false;
    };

    static std::vector<Token> tokenize(const std::vector<InlineRun>& runs, bool preformatted)
    {
        std::vector<Token> tokens;
        for (const auto& run : runs) {
            std::string word;
            auto flush_word = [&]() {
                if (word.empty())
                    return;
                Token token;
                token.run = run;
                token.run.text = word;
                tokens.push_back(token);
                word.clear();
            };
            size_t i = 0;
            while (i < run.text.size()) {
                size_t next = i;
                const unsigned int cp = next_code_point(run.text, next);
                if (cp == ' ' || cp == '\t' || cp == '\n' || cp == '\r') {
                    flush_word();
                    // Preformatted text keeps every space; prose collapses runs.
                    if (preformatted || tokens.empty() || !tokens.back().space) {
                        Token space;
                        space.run = run;
                        space.run.text = " ";
                        space.space = true;
                        tokens.push_back(space);
                    }
                } else if (is_cjk_code_point(cp)) {
                    flush_word();
                    Token token;
                    token.run = run;
                    token.run.text = run.text.substr(i, next - i);
                    token.cjk = true;
                    tokens.push_back(token);
                } else {
                    word += run.text.substr(i, next - i);
                }
                i = next;
            }
            flush_word();
        }
        return tokens;
    }

    // Splits a too-wide token into the widest prefix that fits, returning the
    // prefix; `token_text` keeps the remainder.
    std::string split_to_width(const InlineRun& run, FontRole role, std::string& token_text,
                               double size, double max_width)
    {
        std::vector<size_t> boundaries;
        size_t i = 0;
        while (i < token_text.size()) {
            next_code_point(token_text, i);
            boundaries.push_back(i);
        }
        size_t low = 1, high = boundaries.size(), best = 1;
        while (low <= high) {
            const size_t mid = (low + high) / 2;
            const double width =
                measure(run, role, token_text.substr(0, boundaries[mid - 1]), size);
            if (width <= max_width) {
                best = mid;
                low = mid + 1;
            } else {
                if (mid == 1)
                    break;
                high = mid - 1;
            }
        }
        const std::string prefix = token_text.substr(0, boundaries[best - 1]);
        token_text.erase(0, boundaries[best - 1]);
        return prefix;
    }

    std::vector<WrappedLine> wrap(const std::vector<InlineRun>& runs, FontRole role, double size,
                                  double max_width, bool preformatted = false)
    {
        std::vector<WrappedLine> lines;
        WrappedLine current;

        auto push_piece = [&](const InlineRun& run, double width) {
            if (!current.pieces.empty() && current.pieces.back().run.same_style(run)) {
                current.pieces.back().run.text += run.text;
                current.pieces.back().width += width;
            } else {
                current.pieces.push_back({run, width});
            }
            current.width += width;
        };
        auto break_line = [&]() {
            // Drop a trailing space.
            if (!current.pieces.empty()) {
                auto& last = current.pieces.back();
                while (!last.run.text.empty() && last.run.text.back() == ' ') {
                    last.run.text.pop_back();
                    const double space_width = measure(last.run, role, " ", size);
                    last.width -= space_width;
                    current.width -= space_width;
                }
                if (last.run.text.empty())
                    current.pieces.pop_back();
            }
            lines.push_back(std::move(current));
            current = WrappedLine();
        };

        for (auto& token : tokenize(runs, preformatted)) {
            if (token.space) {
                if (current.pieces.empty() && !preformatted)
                    continue;  // no leading spaces in prose
                push_piece(token.run, measure(token.run, role, " ", size));
                continue;
            }
            double width = measure(token.run, role, token.run.text, size);
            if (current.width + width > max_width && !current.pieces.empty())
                break_line();
            if (width > max_width) {
                std::string remaining = token.run.text;
                while (!remaining.empty()) {
                    InlineRun piece_run = token.run;
                    piece_run.text = split_to_width(token.run, role, remaining, size, max_width);
                    if (piece_run.text.empty())
                        break;  // not even one character fits; give up on splitting
                    const double piece_width = measure(piece_run, role, piece_run.text, size);
                    push_piece(piece_run, piece_width);
                    if (!remaining.empty())
                        break_line();
                }
                continue;
            }
            push_piece(token.run, width);
        }
        if (!current.pieces.empty())
            break_line();
        if (lines.empty())
            lines.push_back(WrappedLine());
        return lines;
    }

    // ── Drawing primitives ─────────────────────────────────────

    // Draws one styled piece at (x, baseline); returns the piece width.
    double draw_piece(const LinePiece& piece, FontRole role, double size, double x,
                      double baseline, StructElement* owner, const std::string& tag)
    {
        if (piece.run.text.empty())
            return 0;
        Font& font = fonts_.resolve(piece.run, role, piece.run.text);

        if (piece.run.code && !piece.run.text.empty() && tag != "Code") {
            // Inline code: light gray backdrop behind the text.
            Path backdrop;
            GraphicState bg;
            bg.set_fill_color(Color(Theme::inline_code_background_gray));
            backdrop.set_graphic_state(bg);
            backdrop.set_paint_op(PathPaintOp::Fill);
            backdrop.add_rect(Point(x - Theme::inline_code_padding, baseline - 0.28 * size),
                              piece.width + 2 * Theme::inline_code_padding, 1.20 * size);
            add_artifact(backdrop);
        }

        GraphicState gs;
        gs.set_fill_color(piece.run.link ? Color(0.0, 0.0, 1.0) : Color(0.0));
        TextState ts;
        Text text;
        Matrix matrix(size, 0, 0, size, x, baseline);
        TextRun run(piece.run.text, font, gs, ts, matrix);
        text.add_run(run);

        if (owner)
            add_tagged(text, tag, *owner);
        else
            add_artifact(text);

        if (piece.run.strike) {
            Path stroke;
            GraphicState sg;
            sg.set_stroke_color(Color(0.0));
            sg.set_width(0.5);
            stroke.set_graphic_state(sg);
            stroke.set_paint_op(PathPaintOp::Stroke);
            stroke.move_to(Point(x, baseline + 0.35 * size));
            stroke.add_line(Point(x + piece.width, baseline + 0.35 * size));
            add_artifact(stroke);
        }

        if (piece.run.link && !piece.run.url.empty()) {
            LinkAnnotation annotation(
                *page_, Rect{x, baseline - 0.25 * size, x + piece.width, baseline + size});
            URIAction action(piece.run.url, false);
            annotation.set_action(action);
            annotation.set_border_style_width(0.0);
            annotation.set_highlight(HighlightStyle::Invert);
            annotation.set_contents(piece.run.text);
        }
        return piece.width;
    }

    void draw_wrapped(const std::vector<InlineRun>& runs, FontRole role, double size,
                      double line_height, double x, double max_width, StructElement& owner,
                      const std::string& tag)
    {
        for (const auto& line : wrap(runs, role, size, max_width)) {
            ensure_space(line_height);
            const double baseline = cursor_y_ - size;
            double pen_x = x;
            for (const auto& piece : line.pieces)
                pen_x += draw_piece(piece, role, size, pen_x, baseline, &owner, tag);
            cursor_y_ -= line_height;
        }
    }

    void draw_rule(double x1, double x2, double y, double width, double gray = 0.0)
    {
        Path line;
        GraphicState gs;
        gs.set_stroke_color(Color(gray));
        gs.set_width(width);
        line.set_graphic_state(gs);
        line.set_paint_op(PathPaintOp::Stroke);
        line.move_to(Point(x1, y));
        line.add_line(Point(x2, y));
        add_artifact(line);
    }

    void draw_filled_rect(double x, double y, double width, double height, double gray)
    {
        Path rect;
        GraphicState gs;
        gs.set_fill_color(Color(gray));
        rect.set_graphic_state(gs);
        rect.set_paint_op(PathPaintOp::Fill);
        rect.add_rect(Point(x, y), width, height);
        add_artifact(rect);
    }

    // ── Blocks ─────────────────────────────────────────────────

    static std::vector<InlineRun> force_bold(const std::vector<InlineRun>& runs)
    {
        std::vector<InlineRun> out = runs;
        for (auto& run : out) {
            if (!run.code)
                run.bold = true;
        }
        return out;
    }

    void render_heading(const Block& block)
    {
        const int index = std::clamp(block.heading_level, 1, 6) - 1;
        StructElement heading =
            document_element_->add_child("H" + std::to_string(index + 1));
        move_down(Theme::heading_space_before[index]);
        const double size = Theme::heading_sizes[index];
        draw_wrapped(force_bold(block.runs), FontRole::Heading, size, size * 1.25, theme_.margin,
                     theme_.content_width(), heading, "H" + std::to_string(index + 1));
        move_down(Theme::heading_space_after[index]);
    }

    void render_paragraph(const Block& block)
    {
        StructElement paragraph = document_element_->add_child("P");
        draw_wrapped(block.runs, FontRole::Body, Theme::body_size, Theme::body_line_height,
                     theme_.margin, theme_.content_width(), paragraph, "P");
        move_down(Theme::paragraph_space_after);
    }

    void render_list(const Block& block)
    {
        StructElement list = document_element_->add_child("L");
        for (const auto& item : block.items) {
            StructElement li = list.add_child("LI");
            StructElement label = li.add_child("Lbl");
            StructElement body = li.add_child("LBody");

            std::string marker;
            if (item.task_state == 1)
                marker = "[x]";
            else if (item.task_state == 0)
                marker = "[ ]";
            else if (block.ordered)
                marker = std::to_string(std::max(1, item.number)) + ".";
            else
                marker = u8"•";

            const double marker_x =
                theme_.margin + item.level * Theme::list_indent_per_level;
            const double text_x = marker_x + Theme::list_marker_width;
            const double text_width = theme_.page_width - theme_.margin - text_x;

            const auto lines =
                wrap(item.runs, FontRole::Body, Theme::body_size, text_width);
            bool first = true;
            for (const auto& line : lines) {
                ensure_space(Theme::body_line_height);
                const double baseline = cursor_y_ - Theme::body_size;
                if (first) {
                    InlineRun marker_run;
                    marker_run.text = marker;
                    LinePiece marker_piece{
                        marker_run,
                        measure(marker_run, FontRole::Body, marker, Theme::body_size)};
                    draw_piece(marker_piece, FontRole::Body, Theme::body_size, marker_x, baseline,
                               &label, "Lbl");
                    first = false;
                }
                double pen_x = text_x;
                for (const auto& piece : line.pieces)
                    pen_x += draw_piece(piece, FontRole::Body, Theme::body_size, pen_x, baseline,
                                        &body, "LBody");
                cursor_y_ -= Theme::body_line_height;
            }
            move_down(Theme::list_item_space_after);
        }
        move_down(Theme::list_space_after);
    }

    static std::string language_label(const std::string& language)
    {
        static const std::map<std::string, std::string> labels = {
            {"cs", "C#"},         {"csharp", "C#"},     {"cpp", "C++"},       {"c++", "C++"},
            {"c", "C"},           {"py", "Python"},     {"python", "Python"}, {"js", "JavaScript"},
            {"javascript", "JavaScript"}, {"ts", "TypeScript"}, {"typescript", "TypeScript"},
            {"ps", "PowerShell"}, {"pwsh", "PowerShell"}, {"powershell", "PowerShell"},
            {"sh", "Shell"},      {"bash", "Shell"},    {"shell", "Shell"},   {"json", "JSON"},
            {"xml", "XML"},       {"html", "HTML"},     {"yaml", "YAML"},     {"yml", "YAML"},
            {"sql", "SQL"},       {"java", "Java"},     {"kotlin", "Kotlin"}, {"go", "Go"},
            {"rust", "Rust"},     {"rb", "Ruby"},       {"ruby", "Ruby"},     {"md", "Markdown"},
            {"markdown", "Markdown"},
        };
        auto it = labels.find(to_lower(language));
        return it != labels.end() ? it->second : language;
    }

    void render_code_block(const Block& block)
    {
        StructElement code = document_element_->add_child("Code");
        move_down(Theme::code_space_before);

        const double width = theme_.content_width();
        const double text_x = theme_.margin + Theme::code_padding;
        const double text_width = width - 2 * Theme::code_padding;

        if (!block.language.empty()) {
            ensure_space(Theme::code_title_line_height);
            draw_filled_rect(theme_.margin, cursor_y_ - Theme::code_title_line_height, width,
                             Theme::code_title_line_height, Theme::code_background_gray);
            InlineRun title;
            title.text = "</> " + language_label(block.language);
            title.bold = true;
            const double baseline = cursor_y_ - Theme::code_title_size;
            LinePiece piece{title, measure(title, FontRole::Code, title.text,
                                           Theme::code_title_size)};
            draw_piece(piece, FontRole::Code, Theme::code_title_size, text_x, baseline, &code,
                       "Code");
            cursor_y_ -= Theme::code_title_line_height;
        }

        // Top padding strip.
        ensure_space(Theme::code_padding);
        draw_filled_rect(theme_.margin, cursor_y_ - Theme::code_padding, width,
                         Theme::code_padding, Theme::code_background_gray);
        cursor_y_ -= Theme::code_padding;

        const std::vector<std::string> lines =
            block.code_lines.empty() ? std::vector<std::string>{""} : block.code_lines;
        for (const auto& source_line : lines) {
            InlineRun run;
            run.text = source_line;
            run.code = true;
            std::vector<InlineRun> runs{run};
            for (auto& line :
                 wrap(runs, FontRole::Code, Theme::code_size, text_width, true)) {
                ensure_space(Theme::code_line_height);
                draw_filled_rect(theme_.margin, cursor_y_ - Theme::code_line_height, width,
                                 Theme::code_line_height, Theme::code_background_gray);
                const double baseline = cursor_y_ - Theme::code_size;
                double pen_x = text_x;
                for (const auto& piece : line.pieces)
                    pen_x += draw_piece(piece, FontRole::Code, Theme::code_size, pen_x, baseline,
                                        &code, "Code");
                cursor_y_ -= Theme::code_line_height;
            }
        }

        // Bottom padding strip.
        ensure_space(Theme::code_padding);
        draw_filled_rect(theme_.margin, cursor_y_ - Theme::code_padding, width,
                         Theme::code_padding, Theme::code_background_gray);
        cursor_y_ -= Theme::code_padding;

        move_down(Theme::code_space_after);
    }

    void render_blockquote(const Block& block)
    {
        StructElement quote = document_element_->add_child("BlockQuote");
        move_down(Theme::quote_space_before);

        std::vector<InlineRun> runs = block.runs;
        for (auto& run : runs) {
            if (!run.code)
                run.italic = true;
        }

        const double text_x = theme_.margin + Theme::quote_indent;
        const double text_width = theme_.page_width - theme_.margin - text_x;
        bool first = true;
        for (const auto& line : wrap(runs, FontRole::Body, Theme::body_size, text_width)) {
            ensure_space(Theme::body_line_height);
            const double baseline = cursor_y_ - Theme::body_size;
            if (first) {
                InlineRun marker;
                marker.text = ">";
                marker.italic = true;
                LinePiece piece{marker,
                                measure(marker, FontRole::Body, ">", Theme::body_size)};
                draw_piece(piece, FontRole::Body, Theme::body_size, theme_.margin, baseline,
                           nullptr, "");
                first = false;
            }
            double pen_x = text_x;
            for (const auto& piece : line.pieces)
                pen_x += draw_piece(piece, FontRole::Body, Theme::body_size, pen_x, baseline,
                                    &quote, "BlockQuote");
            cursor_y_ -= Theme::body_line_height;
        }
        move_down(Theme::quote_space_after);
    }

    void render_table(const Block& block)
    {
        if (block.header_cells.empty())
            return;
        StructElement table = document_element_->add_child("Table");
        move_down(Theme::table_space_before);

        const size_t columns = block.header_cells.size();
        const double column_width = theme_.content_width() / static_cast<double>(columns);
        const double cell_text_width = column_width - 2 * Theme::table_cell_padding;

        bool first_row_on_page = true;
        auto draw_row = [&](const std::vector<std::vector<InlineRun>>& cells, bool header,
                            StructElement& row_parent) {
            StructElement row = row_parent.add_child("TR");

            // Wrap every cell first so the row height is known.
            std::vector<std::vector<WrappedLine>> wrapped;
            size_t max_lines = 1;
            for (size_t c = 0; c < columns; ++c) {
                std::vector<InlineRun> runs =
                    c < cells.size() ? cells[c] : std::vector<InlineRun>{};
                if (header)
                    runs = force_bold(runs);
                wrapped.push_back(
                    wrap(runs, FontRole::Body, Theme::table_size, cell_text_width));
                max_lines = std::max(max_lines, wrapped.back().size());
            }
            const double row_height = static_cast<double>(max_lines) * Theme::table_line_height +
                                      2 * Theme::table_cell_padding;
            if (cursor_y_ - row_height < theme_.margin && !at_top()) {
                new_page();
                first_row_on_page = true;
            }
            if (first_row_on_page) {
                draw_rule(theme_.margin, theme_.margin + column_width * columns, cursor_y_,
                          Theme::table_border_width);
                first_row_on_page = false;
            }

            const double row_top = cursor_y_;
            for (size_t c = 0; c < columns; ++c) {
                StructElement cell = row.add_child(header ? "TH" : "TD");
                const double cell_x = theme_.margin + column_width * static_cast<double>(c);
                double line_y = row_top - Theme::table_cell_padding;
                for (const auto& line : wrapped[c]) {
                    const double baseline = line_y - Theme::table_size;
                    double start_x = cell_x + Theme::table_cell_padding;
                    const ColumnAlignment alignment =
                        c < block.alignments.size() ? block.alignments[c]
                                                    : ColumnAlignment::Left;
                    if (alignment == ColumnAlignment::Right)
                        start_x = cell_x + column_width - Theme::table_cell_padding - line.width;
                    else if (alignment == ColumnAlignment::Center)
                        start_x = cell_x + (column_width - line.width) / 2;
                    double pen_x = start_x;
                    for (const auto& piece : line.pieces)
                        pen_x += draw_piece(piece, FontRole::Body, Theme::table_size, pen_x,
                                            baseline, &cell, header ? "TH" : "TD");
                    line_y -= Theme::table_line_height;
                }
            }

            // Grid: verticals for this row plus the bottom border.
            for (size_t c = 0; c <= columns; ++c) {
                const double x = theme_.margin + column_width * static_cast<double>(c);
                Path line;
                GraphicState gs;
                gs.set_stroke_color(Color(0.0));
                gs.set_width(Theme::table_border_width);
                line.set_graphic_state(gs);
                line.set_paint_op(PathPaintOp::Stroke);
                line.move_to(Point(x, row_top));
                line.add_line(Point(x, row_top - row_height));
                add_artifact(line);
            }
            draw_rule(theme_.margin, theme_.margin + column_width * columns,
                      row_top - row_height, Theme::table_border_width);
            cursor_y_ = row_top - row_height;
        };

        draw_row(block.header_cells, true, table);
        for (const auto& row : block.rows)
            draw_row(row, false, table);

        move_down(Theme::table_space_after);
    }

    void render_horizontal_rule()
    {
        move_down(Theme::rule_space_before);
        ensure_space(12);
        draw_rule(theme_.margin, theme_.page_width - theme_.margin, cursor_y_ - 4, 0.75);
        move_down(Theme::rule_space_after + 8);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Self-tests: exercise the parser and option handling without creating a PDF.
// ─────────────────────────────────────────────────────────────────────────────

int run_self_tests()
{
    int failures = 0;
    auto require = [&](bool condition, const std::string& what) {
        if (!condition) {
            std::cout << "FAILED: " << what << std::endl;
            ++failures;
        }
    };

    MarkdownParser parser;
    const auto find_run = [](const std::vector<Block>& blocks,
                             const std::function<bool(const InlineRun&)>& predicate)
        -> const InlineRun* {
        for (const auto& block : blocks) {
            for (const auto& run : block.runs)
                if (predicate(run))
                    return &run;
            for (const auto& item : block.items)
                for (const auto& run : item.runs)
                    if (predicate(run))
                        return &run;
        }
        return nullptr;
    };

    {
        const auto blocks = parser.parse(
            "# Title\n\nSome **bold**, *italic*, ***both***, ~~gone~~, and `code`.\n", false);
        require(blocks.size() == 2, "heading + paragraph block count");
        require(blocks[0].type == BlockType::Heading && blocks[0].heading_level == 1,
                "ATX heading level 1");
        require(find_run(blocks, [](const InlineRun& r) { return r.bold && !r.italic; }) != nullptr,
                "bold run parsed");
        require(find_run(blocks, [](const InlineRun& r) { return r.italic && !r.bold; }) != nullptr,
                "italic run parsed");
        require(find_run(blocks, [](const InlineRun& r) { return r.bold && r.italic; }) != nullptr,
                "bold-italic run parsed");
        require(find_run(blocks, [](const InlineRun& r) { return r.strike; }) != nullptr,
                "strikethrough run parsed");
        require(find_run(blocks, [](const InlineRun& r) { return r.code; }) != nullptr,
                "inline code run parsed");
    }
    {
        const auto blocks = parser.parse(
            "See [Community Guide][guide] and <https://example.com> or "
            "https://example.org/trail.\n\n[guide]: https://docs.example.com\n",
            false);
        const InlineRun* reference = find_run(
            blocks, [](const InlineRun& r) { return r.link && r.text == "Community Guide"; });
        require(reference && reference->url == "https://docs.example.com",
                "reference link resolves URL, shows anchor text");
        require(find_run(blocks, [](const InlineRun& r) {
                    return r.link && r.url == "https://example.com";
                }) != nullptr,
                "autolink parsed");
        require(find_run(blocks, [](const InlineRun& r) {
                    return r.link && r.url == "https://example.org/trail";
                }) != nullptr,
                "bare URL parsed with trailing punctuation trimmed");
    }
    {
        const auto blocks = parser.parse("- [x] done\n- [ ] todo\n\n1. one\n2. two\n", false);
        require(blocks.size() == 2, "task list and ordered list separate blocks");
        require(blocks[0].type == BlockType::List && !blocks[0].ordered &&
                    blocks[0].items.size() == 2 && blocks[0].items[0].task_state == 1,
                "task list checked state");
        require(blocks[1].type == BlockType::List && blocks[1].ordered &&
                    blocks[1].items.size() == 2,
                "ordered list items");
    }
    {
        const auto blocks =
            parser.parse("> A quote with a [link](https://example.net) inside.\n", false);
        require(blocks.size() == 1 && blocks[0].type == BlockType::BlockQuote,
                "blockquote block");
        require(find_run(blocks, [](const InlineRun& r) {
                    return r.link && r.url == "https://example.net";
                }) != nullptr,
                "link inside blockquote");
    }
    {
        const auto blocks =
            parser.parse("```PowerShell\nGet-ChildItem\nWrite-Output done\n```\n", false);
        require(blocks.size() == 1 && blocks[0].type == BlockType::CodeBlock &&
                    blocks[0].language == "PowerShell" && blocks[0].code_lines.size() == 2,
                "fenced code block with language");
    }
    {
        const auto blocks = parser.parse(
            "| Name | Count |\n| --- | ---: |\n| a | 1 |\n| b | 2 |\n", false);
        require(blocks.size() == 1 && blocks[0].type == BlockType::Table,
                "table recognized");
        if (!blocks.empty() && blocks[0].type == BlockType::Table) {
            require(blocks[0].header_cells.size() == 2, "table column count");
            require(blocks[0].rows.size() == 2, "table row count");
            require(blocks[0].alignments[0] == ColumnAlignment::Left &&
                        blocks[0].alignments[1] == ColumnAlignment::Right,
                    "table alignments");
        }
    }
    {
        const auto blocks = parser.parse("Intro\n\n---\n\nSetext\n------\n", false);
        bool saw_rule = false, saw_setext = false;
        for (const auto& block : blocks) {
            if (block.type == BlockType::HorizontalRule)
                saw_rule = true;
            if (block.type == BlockType::Heading && block.heading_level == 2)
                saw_setext = true;
        }
        require(saw_rule, "horizontal rule");
        require(saw_setext, "setext heading level 2");
    }
    {
        const auto blocks =
            parser.parse("Use TRAIL_MAP_VERSION and _italic_ plus **bold**.\n", false);
        require(find_run(blocks, [](const InlineRun& r) {
                    return !r.italic && r.text.find("TRAIL_MAP_VERSION") != std::string::npos;
                }) != nullptr,
                "underscore identifier not italicized");
        require(find_run(blocks, [](const InlineRun& r) {
                    return r.italic && r.text == "italic";
                }) != nullptr,
                "underscore emphasis still works");
    }
    {
        const auto blocks = parser.parse(
            "<h2>HTML heading</h2>\n<p><strong>Bold</strong> &amp; <em>italic</em>, "
            "<code>mono</code>, <del>gone</del>, <a href=\"https://example.com\">site</a>."
            "<img src=\"x.png\" alt=\"badge\"><aside>note</aside></p>\n",
            false);
        require(!blocks.empty() && blocks[0].type == BlockType::Heading &&
                    blocks[0].heading_level == 2,
                "HTML h2 becomes heading");
        require(find_run(blocks, [](const InlineRun& r) { return r.bold; }) != nullptr,
                "HTML strong becomes bold");
        require(find_run(blocks, [](const InlineRun& r) { return r.code; }) != nullptr,
                "HTML code becomes inline code");
        require(find_run(blocks, [](const InlineRun& r) { return r.strike; }) != nullptr,
                "HTML del becomes strikethrough");
        require(find_run(blocks, [](const InlineRun& r) {
                    return r.link && r.url == "https://example.com";
                }) != nullptr,
                "HTML anchor becomes link");
        require(find_run(blocks, [](const InlineRun& r) {
                    return r.text.find("[Image omitted: badge]") != std::string::npos;
                }) != nullptr,
                "HTML img becomes omission note");
        require(find_run(blocks, [](const InlineRun& r) {
                    return r.text.find('&') != std::string::npos;
                }) != nullptr,
                "HTML entity decoded");
        require(find_run(blocks, [](const InlineRun& r) {
                    return r.text.find("<aside>") != std::string::npos;
                }) == nullptr,
                "unsupported tag stripped by default");
    }
    {
        const auto blocks =
            parser.parse("Config: <setting name=\"depth\"> here.\n", true);
        require(find_run(blocks, [](const InlineRun& r) {
                    return r.text.find("<setting name=\"depth\">") != std::string::npos;
                }) != nullptr,
                "--include-unrendered-html keeps raw tags");
    }
    {
        const auto blocks = parser.parse(u8"**阅读**的意**义**\n", false);
        require(find_run(blocks, [](const InlineRun& r) {
                    return r.bold && r.text == u8"阅读";
                }) != nullptr,
                "CJK bold at line start");
        require(find_run(blocks, [](const InlineRun& r) {
                    return r.bold && r.text == u8"义";
                }) != nullptr,
                "CJK bold mid-line");
        const InlineRun* literal = find_run(blocks, [](const InlineRun& r) {
            return r.text.find("**") != std::string::npos;
        });
        require(literal == nullptr, "no literal asterisks remain in CJK emphasis");
    }
    {
        const auto options = parse_options(
            {"--page-size", "A4", "--orientation", "Landscape", "--font-family", "serif",
             "--code-font-family", "mono", "--fallback-fonts", "Noto Sans CJK SC, DejaVu Sans",
             "--margin", "54", "--include-unrendered-html", "in.md", "out.pdf"});
        require(options.page_size == "A4" && options.orientation == Orientation::Landscape,
                "page size and orientation options");
        require(options.margin == 54 && options.include_unrendered_html,
                "margin and raw HTML options");
        require(options.fallback_fonts.size() == 2 &&
                    options.fallback_fonts[1] == "DejaVu Sans",
                "fallback fonts split on commas");
        const Theme theme = Theme::create(options);
        require(theme.page_width > theme.page_height, "A4 landscape swaps dimensions");
    }
    {
        const auto options = parse_options({});
        require(options.input_path == "sample.md" && options.overwrite,
                "defaults with no arguments");
        bool threw = false;
        try {
            parse_options({"--margin", "10", "a", "b"});
        } catch (const CommandLineException&) {
            threw = true;
        }
        require(threw, "margin out of range rejected");
        const Theme custom = Theme::create(parse_options({"--page-size", "500x700", "a", "b"}));
        require(custom.page_width == 500 && custom.page_height == 700, "custom page size");
    }

    if (failures == 0) {
        std::cout << "Self-tests passed." << std::endl;
        return 0;
    }
    std::cout << failures << " self-test(s) failed." << std::endl;
    return 1;
}

}  // namespace

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    std::cout << "MarkdownToPDF Sample:" << std::endl;

    std::vector<std::string> args(argv + 1, argv + argc);
    for (const auto& arg : args) {
        const std::string lower = to_lower(arg);
        if (lower == "--help" || lower == "-h" || lower == "/?") {
            print_usage();
            return 0;
        }
    }
    try {
        if (const char* key = std::getenv("APDFL_LICENSE_KEY"); key && *key)
            Library::set_license_key(key);

        Library library;

        // Run self-tests with the library initialized so they see the same
        // process state (e.g. locale) as a real conversion.
        if (args.size() == 1 && to_lower(args[0]) == "--self-test")
            return run_self_tests();

        if (args.size() == 1 && to_lower(args[0]) == "--list-font-families") {
            FontCatalog catalog;
            catalog.print_families();
            return 0;
        }

        const ConversionOptions options = parse_options(args);

        namespace fs = std::filesystem;
        if (!fs::exists(options.input_path)) {
            std::cout << "Input Markdown file was not found: " << options.input_path
                      << std::endl;
            return 3;
        }
        if (fs::exists(options.output_path) && !options.overwrite)
            throw CommandLineException("Output file already exists: " + options.output_path +
                                       ". Use --overwrite to replace it.");

        std::ifstream input(options.input_path, std::ios::binary);
        std::stringstream buffer;
        buffer << input.rdbuf();
        const std::string markdown = buffer.str();

        MarkdownParser parser;
        const std::vector<Block> blocks =
            parser.parse(markdown, options.include_unrendered_html);
        const Theme theme = Theme::create(options);

        Document document;
        const std::string title = !options.title.empty()
                                      ? options.title
                                      : fs::path(options.input_path).stem().string();
        document.set_title(title);
        document.set_producer("MarkdownToPDF tagged sample using Datalogics APDFL");

        FontCatalog catalog;
        FontSet fonts(catalog, options);
        Renderer renderer(document, theme, options, fonts);
        renderer.render(blocks);

        document.embed_fonts();
        document.save(SaveFlags::Full, options.output_path);
        std::cout << "Created tagged PDF: " << options.output_path << std::endl;

        if (options.verbose) {
            std::cout << "  Blocks: " << blocks.size() << std::endl;
            std::cout << "  Pages: " << renderer.page_count() << std::endl;
            std::cout << "  Language: " << options.language << std::endl;
            std::cout << "  Page size: " << options.page_size << " (" << theme.page_width << " x "
                      << theme.page_height << " pt)" << std::endl;
            std::cout << "  Margin: " << theme.margin << " pt" << std::endl;
            std::cout << "  Body font: " << options.font_family << std::endl;
            std::cout << "  Heading font: " << options.heading_font_family << std::endl;
            std::cout << "  Code font: " << options.code_font_family << std::endl;
            std::cout << "  Include unrendered HTML: "
                      << (options.include_unrendered_html ? "yes" : "no") << std::endl;
        }
    } catch (const CommandLineException& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        std::cout << "Run with --help for usage." << std::endl;
        return 2;
    } catch (const std::exception& e) {
        std::cout << "ERROR: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
