/*
 * This sample builds a tagged PDF invoice from structured input files — JSON
 * metadata, a CSV of line items, a JSON style sheet, and a local PNG logo —
 * using the Datalogics C++ APDFL API directly. It lays out a styled invoice
 * (header with logo, seller/customer cards, a line-item table that repeats
 * its header across pages, a totals box, and payment notes), emits logical
 * structure (Document, Sect, H1/H2, P, Figure with alt text, Table/TR/TH/TD)
 * so the output is accessible, and optionally applies an owner-password
 * restriction so the invoice can be viewed and printed but not edited.
 *
 * Copyright (c) Datalogics, Inc. All rights reserved.
 */

#include <datalogics_interface/datalogics_interface.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
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

std::string to_lower(const std::string& s)
{
    std::string out = s;
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

struct CommandLineOptions {
    std::string metadata_path = "data/metadata.json";
    std::string line_items_path = "data/line-items.csv";
    std::string style_path = "data/style.json";
    std::string output_path = "CreateInvoiceFromStructuredData-out.pdf";
};

void print_usage()
{
    std::cout <<
        "Usage:\n"
        "  create_invoice_from_structured_data [options]\n"
        "\n"
        "With no options the sample reads data/metadata.json, data/line-items.csv,\n"
        "and data/style.json, then writes CreateInvoiceFromStructuredData-out.pdf.\n"
        "\n"
        "Options:\n"
        "  --metadata <path>    JSON file with seller, customer, and invoice metadata.\n"
        "  --line-items <path>  CSV file with invoice line items.\n"
        "  --style <path>       JSON file with fonts, colors, and page dimensions.\n"
        "  --output <path>      Output PDF path.\n"
        "  --self-test          Validate parsing and totals without creating a PDF.\n"
        "  --help               Show this help.\n"
        "\n"
        "Set APDFL_LICENSE_KEY to provide a Datalogics APDFL activation key before\n"
        "Library initialization.\n";
}

CommandLineOptions parse_options(const std::vector<std::string>& args)
{
    CommandLineOptions options;
    auto require_value = [&](size_t& i, const std::string& option) -> const std::string& {
        if (i + 1 >= args.size())
            throw CommandLineException(option + " requires a value.");
        return args[++i];
    };
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string lower = to_lower(args[i]);
        if (lower == "--metadata" || lower == "--invoice")
            options.metadata_path = require_value(i, args[i]);
        else if (lower == "--line-items")
            options.line_items_path = require_value(i, args[i]);
        else if (lower == "--style")
            options.style_path = require_value(i, args[i]);
        else if (lower == "--output")
            options.output_path = require_value(i, args[i]);
        else
            throw CommandLineException("Unknown option: " + args[i]);
    }
    return options;
}

// ─────────────────────────────────────────────────────────────────────────────
// Minimal JSON parser — just enough for the sample's flat metadata and style
// files: objects, arrays, strings, numbers, booleans, and null.
// ─────────────────────────────────────────────────────────────────────────────

class JsonValue {
public:
    enum class Type { Null, Boolean, Number, String, Object, Array };

    Type type = Type::Null;
    bool boolean = false;
    double number = 0;
    std::string string;
    std::map<std::string, JsonValue> object;
    std::vector<JsonValue> array;

    const JsonValue* find(const std::string& key) const
    {
        // Keys are matched case-insensitively, like the .NET sample.
        for (const auto& [name, value] : object) {
            if (to_lower(name) == to_lower(key))
                return &value;
        }
        return nullptr;
    }

    std::string get_string(const std::string& key, const std::string& fallback = "") const
    {
        const JsonValue* value = find(key);
        return value && value->type == Type::String ? value->string : fallback;
    }

    double get_number(const std::string& key, double fallback = 0) const
    {
        const JsonValue* value = find(key);
        return value && value->type == Type::Number ? value->number : fallback;
    }

    bool get_boolean(const std::string& key, bool fallback = false) const
    {
        const JsonValue* value = find(key);
        return value && value->type == Type::Boolean ? value->boolean : fallback;
    }
};

class JsonParser {
public:
    static JsonValue parse(const std::string& text)
    {
        JsonParser parser(text);
        JsonValue value = parser.parse_value();
        parser.skip_whitespace();
        if (parser.pos_ != text.size())
            throw std::invalid_argument("Unexpected trailing content in JSON input.");
        return value;
    }

private:
    explicit JsonParser(const std::string& text) : text_(text) {}

    const std::string& text_;
    size_t pos_ = 0;

    [[noreturn]] void fail(const std::string& message) const
    {
        throw std::invalid_argument("JSON error at offset " + std::to_string(pos_) + ": " +
                                    message);
    }

    void skip_whitespace()
    {
        while (pos_ < text_.size()) {
            const char c = text_[pos_];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos_;
            } else if (c == '/' && pos_ + 1 < text_.size() && text_[pos_ + 1] == '/') {
                while (pos_ < text_.size() && text_[pos_] != '\n')
                    ++pos_;
            } else {
                break;
            }
        }
    }

    char peek()
    {
        skip_whitespace();
        if (pos_ >= text_.size())
            fail("unexpected end of input");
        return text_[pos_];
    }

    void expect(char c)
    {
        if (peek() != c)
            fail(std::string("expected '") + c + "'");
        ++pos_;
    }

    JsonValue parse_value()
    {
        const char c = peek();
        if (c == '{')
            return parse_object();
        if (c == '[')
            return parse_array();
        if (c == '"')
            return parse_string_value();
        if (c == 't' || c == 'f') {
            JsonValue value;
            value.type = JsonValue::Type::Boolean;
            value.boolean = c == 't';
            expect_literal(value.boolean ? "true" : "false");
            return value;
        }
        if (c == 'n') {
            expect_literal("null");
            return JsonValue{};
        }
        return parse_number();
    }

    void expect_literal(const std::string& literal)
    {
        skip_whitespace();
        if (text_.compare(pos_, literal.size(), literal) != 0)
            fail("expected \"" + literal + "\"");
        pos_ += literal.size();
    }

    JsonValue parse_object()
    {
        JsonValue value;
        value.type = JsonValue::Type::Object;
        expect('{');
        if (peek() == '}') {
            ++pos_;
            return value;
        }
        while (true) {
            if (peek() == '}') {  // trailing comma
                ++pos_;
                return value;
            }
            const std::string key = parse_string();
            expect(':');
            value.object[key] = parse_value();
            if (peek() == ',') {
                ++pos_;
                continue;
            }
            expect('}');
            return value;
        }
    }

    JsonValue parse_array()
    {
        JsonValue value;
        value.type = JsonValue::Type::Array;
        expect('[');
        if (peek() == ']') {
            ++pos_;
            return value;
        }
        while (true) {
            if (peek() == ']') {  // trailing comma
                ++pos_;
                return value;
            }
            value.array.push_back(parse_value());
            if (peek() == ',') {
                ++pos_;
                continue;
            }
            expect(']');
            return value;
        }
    }

    JsonValue parse_string_value()
    {
        JsonValue value;
        value.type = JsonValue::Type::String;
        value.string = parse_string();
        return value;
    }

    std::string parse_string()
    {
        expect('"');
        std::string out;
        while (pos_ < text_.size()) {
            const char c = text_[pos_++];
            if (c == '"')
                return out;
            if (c != '\\') {
                out.push_back(c);
                continue;
            }
            if (pos_ >= text_.size())
                fail("unterminated escape");
            const char escape = text_[pos_++];
            switch (escape) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                if (pos_ + 4 > text_.size())
                    fail("truncated \\u escape");
                const unsigned int cp =
                    static_cast<unsigned int>(std::stoul(text_.substr(pos_, 4), nullptr, 16));
                pos_ += 4;
                // Encode the basic multilingual plane code point as UTF-8;
                // surrogate pairs are not needed by the sample inputs.
                if (cp < 0x80) {
                    out.push_back(static_cast<char>(cp));
                } else if (cp < 0x800) {
                    out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                } else {
                    out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                    out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                    out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
                }
                break;
            }
            default: fail("unsupported escape");
            }
        }
        fail("unterminated string");
    }

    JsonValue parse_number()
    {
        skip_whitespace();
        const size_t start = pos_;
        while (pos_ < text_.size()) {
            const char c = text_[pos_];
            if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' ||
                c == 'E')
                ++pos_;
            else
                break;
        }
        if (pos_ == start)
            fail("expected a value");
        JsonValue value;
        value.type = JsonValue::Type::Number;
        std::istringstream stream(text_.substr(start, pos_ - start));
        stream.imbue(std::locale::classic());
        if (!(stream >> value.number))
            fail("invalid number");
        return value;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Invoice model
// ─────────────────────────────────────────────────────────────────────────────

struct CompanyInfo {
    std::string name;
    std::string tax_id;
    std::string address_line1;
    std::string address_line2;
    std::string city;
    std::string region;
    std::string postal_code;
    std::string country;
    std::string email;
    std::string phone;

    std::vector<std::string> address_lines() const
    {
        std::string locality = city;
        if (!region.empty())
            locality += (locality.empty() ? "" : ", ") + region;
        if (!postal_code.empty())
            locality += (locality.empty() ? "" : " ") + postal_code;
        std::vector<std::string> lines{address_line1, address_line2, locality, country};
        lines.erase(std::remove_if(lines.begin(), lines.end(),
                                   [](const std::string& line) { return line.empty(); }),
                    lines.end());
        return lines;
    }
};

struct InvoiceInput {
    std::string invoice_number;
    std::string issue_date;
    std::string due_date;
    std::string currency = "USD";
    double tax_rate = 0;
    std::string logo_path;
    std::string payment_terms;
    std::string notes;
    bool apply_restriction_password = true;
    std::string restriction_password = "NSS-Restrict-2026-ReviewOnly!";
    CompanyInfo seller;
    CompanyInfo customer;
};

struct InvoiceLineItem {
    std::string item_code;
    std::string description;
    double quantity = 0;
    double unit_price = 0;

    double amount() const { return quantity * unit_price; }
};

struct RgbColor {
    double r = 0, g = 0, b = 0;

    static RgbColor from_hex(const std::string& value)
    {
        std::string hex = value;
        if (!hex.empty() && hex[0] == '#')
            hex.erase(0, 1);
        const bool valid = hex.size() == 6 &&
                           hex.find_first_not_of("0123456789abcdefABCDEF") == std::string::npos;
        if (!valid)
            throw std::invalid_argument("Color value must be in #RRGGBB form: " + value);
        auto channel = [&](size_t at) {
            return static_cast<double>(std::stoul(hex.substr(at, 2), nullptr, 16)) / 255.0;
        };
        return {channel(0), channel(2), channel(4)};
    }

    Color to_color() const { return Color(r, g, b); }
};

struct InvoiceStyle {
    std::string body_font = "Helvetica";
    std::string bold_font = "Helvetica-Bold";
    double body_font_size = 9.5;
    double small_font_size = 8.0;
    double heading_font_size = 20.0;
    double table_header_font_size = 8.5;
    double page_width = 612.0;
    double page_height = 792.0;
    double margin = 54.0;
    double logo_max_width = 172.0;
    double logo_max_height = 54.0;
    RgbColor primary_color = RgbColor::from_hex("#24536A");
    RgbColor accent_color = RgbColor::from_hex("#E8F1F4");
    RgbColor text_color = RgbColor::from_hex("#222222");
    RgbColor muted_text_color = RgbColor::from_hex("#666666");
    RgbColor border_color = RgbColor::from_hex("#B8C7CE");
};

// ─────────────────────────────────────────────────────────────────────────────
// Input loading
// ─────────────────────────────────────────────────────────────────────────────

std::string read_file(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input)
        throw std::invalid_argument("File was not found: " + path);
    std::stringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

CompanyInfo load_company(const JsonValue& root, const std::string& key)
{
    CompanyInfo company;
    const JsonValue* value = root.find(key);
    if (!value)
        return company;
    company.name = value->get_string("name");
    company.tax_id = value->get_string("taxId");
    company.address_line1 = value->get_string("addressLine1");
    company.address_line2 = value->get_string("addressLine2");
    company.city = value->get_string("city");
    company.region = value->get_string("region");
    company.postal_code = value->get_string("postalCode");
    company.country = value->get_string("country");
    company.email = value->get_string("email");
    company.phone = value->get_string("phone");
    return company;
}

InvoiceInput load_invoice(const std::string& path)
{
    const JsonValue root = JsonParser::parse(read_file(path));
    InvoiceInput invoice;
    invoice.invoice_number = root.get_string("invoiceNumber");
    invoice.issue_date = root.get_string("issueDate");
    invoice.due_date = root.get_string("dueDate");
    invoice.currency = root.get_string("currency", "USD");
    invoice.tax_rate = root.get_number("taxRate", 0);
    invoice.logo_path = root.get_string("logoPath");
    invoice.payment_terms = root.get_string("paymentTerms");
    invoice.notes = root.get_string("notes");
    invoice.apply_restriction_password = root.get_boolean("applyRestrictionPassword", true);
    invoice.restriction_password =
        root.get_string("restrictionPassword", invoice.restriction_password);
    invoice.seller = load_company(root, "seller");
    invoice.customer = load_company(root, "customer");

    if (invoice.invoice_number.empty())
        throw std::invalid_argument("Invoice metadata requires a non-empty invoiceNumber.");
    if (invoice.seller.name.empty())
        throw std::invalid_argument("Invoice metadata requires a seller name.");
    if (invoice.customer.name.empty())
        throw std::invalid_argument("Invoice metadata requires a customer name.");
    if (invoice.apply_restriction_password && invoice.restriction_password.empty())
        throw std::invalid_argument(
            "restrictionPassword must be set when applyRestrictionPassword is true.");
    return invoice;
}

// Splits one CSV row on commas, honoring double-quoted fields and "" escapes.
std::vector<std::string> parse_csv_row(const std::string& row)
{
    std::vector<std::string> fields;
    std::string current;
    bool quoted = false;
    for (size_t i = 0; i < row.size(); ++i) {
        const char c = row[i];
        if (quoted) {
            if (c == '"' && i + 1 < row.size() && row[i + 1] == '"') {
                current.push_back('"');
                ++i;
            } else if (c == '"') {
                quoted = false;
            } else {
                current.push_back(c);
            }
        } else if (c == '"') {
            quoted = true;
        } else if (c == ',') {
            fields.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    fields.push_back(current);
    for (auto& field : fields) {
        const auto begin = field.find_first_not_of(" \t\r");
        const auto end = field.find_last_not_of(" \t\r");
        field = begin == std::string::npos ? "" : field.substr(begin, end - begin + 1);
    }
    return fields;
}

double parse_decimal(const std::string& value, int row_number, const std::string& column)
{
    // Invariant parse: '.' decimal separator, thousands separators allowed.
    std::string cleaned;
    for (char c : value) {
        if (c != ',')
            cleaned.push_back(c);
    }
    std::istringstream stream(cleaned);
    stream.imbue(std::locale::classic());
    double out = 0;
    if (!(stream >> out) || !(stream >> std::ws).eof())
        throw std::invalid_argument("Row " + std::to_string(row_number) + " has an invalid " +
                                    column + " value: " + value);
    return out;
}

std::vector<InvoiceLineItem> load_line_items(const std::string& path)
{
    std::stringstream stream(read_file(path));
    std::vector<std::string> rows;
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find_first_not_of(" \t\r") != std::string::npos)
            rows.push_back(line);
    }
    if (rows.size() < 2)
        throw std::invalid_argument("Line item CSV requires a header row and at least one item.");

    std::map<std::string, size_t> columns;
    const std::vector<std::string> header = parse_csv_row(rows[0]);
    for (size_t i = 0; i < header.size(); ++i)
        columns[to_lower(header[i])] = i;
    for (const char* required : {"itemcode", "description", "quantity", "unitprice"}) {
        if (!columns.count(required))
            throw std::invalid_argument(std::string("Line item CSV is missing the ") + required +
                                        " column.");
    }

    std::vector<InvoiceLineItem> items;
    for (size_t r = 1; r < rows.size(); ++r) {
        const std::vector<std::string> fields = parse_csv_row(rows[r]);
        auto field = [&](const char* name) {
            const size_t index = columns.at(name);
            return index < fields.size() ? fields[index] : std::string();
        };
        InvoiceLineItem item;
        item.item_code = field("itemcode");
        item.description = field("description");
        const int row_number = static_cast<int>(r) + 1;
        item.quantity = parse_decimal(field("quantity"), row_number, "quantity");
        item.unit_price = parse_decimal(field("unitprice"), row_number, "unitPrice");
        items.push_back(std::move(item));
    }
    return items;
}

InvoiceStyle load_style(const std::string& path)
{
    const JsonValue root = JsonParser::parse(read_file(path));
    InvoiceStyle style;
    style.body_font = root.get_string("bodyFont", style.body_font);
    style.bold_font = root.get_string("boldFont", style.bold_font);
    style.body_font_size = root.get_number("bodyFontSize", style.body_font_size);
    style.small_font_size = root.get_number("smallFontSize", style.small_font_size);
    style.heading_font_size = root.get_number("headingFontSize", style.heading_font_size);
    style.table_header_font_size =
        root.get_number("tableHeaderFontSize", style.table_header_font_size);
    style.page_width = root.get_number("pageWidth", style.page_width);
    style.page_height = root.get_number("pageHeight", style.page_height);
    style.margin = root.get_number("margin", style.margin);
    style.logo_max_width = root.get_number("logoMaxWidth", style.logo_max_width);
    style.logo_max_height = root.get_number("logoMaxHeight", style.logo_max_height);
    auto color = [&](const char* key, RgbColor fallback) {
        const JsonValue* value = root.find(key);
        return value && value->type == JsonValue::Type::String ? RgbColor::from_hex(value->string)
                                                               : fallback;
    };
    style.primary_color = color("primaryColor", style.primary_color);
    style.accent_color = color("accentColor", style.accent_color);
    style.text_color = color("textColor", style.text_color);
    style.muted_text_color = color("mutedTextColor", style.muted_text_color);
    style.border_color = color("borderColor", style.border_color);
    return style;
}

// ─────────────────────────────────────────────────────────────────────────────
// Formatting (invariant, US-style separators to match the .NET sample)
// ─────────────────────────────────────────────────────────────────────────────

double round_away_from_zero(double value, int decimals)
{
    const double scale = std::pow(10.0, decimals);
    return (value >= 0 ? std::floor(value * scale + 0.5) : std::ceil(value * scale - 0.5)) /
           scale;
}

std::string format_fixed(double value, int decimals, bool thousands)
{
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::fixed << std::setprecision(decimals) << std::abs(value);
    std::string digits = stream.str();
    if (thousands) {
        const size_t point = digits.find('.');
        size_t whole_end = point == std::string::npos ? digits.size() : point;
        for (size_t i = whole_end; i > 3; i -= 3)
            digits.insert(i - 3, ",");
    }
    return (value < 0 ? "-" : "") + digits;
}

// "0.##": up to two decimals, no trailing zeros.
std::string format_number(double value)
{
    std::string out = format_fixed(round_away_from_zero(value, 2), 2, false);
    while (!out.empty() && out.back() == '0')
        out.pop_back();
    if (!out.empty() && out.back() == '.')
        out.pop_back();
    return out;
}

std::string format_currency(double value, const std::string& currency)
{
    const std::string amount = format_fixed(value, 2, true);
    if (to_lower(currency) == "usd")
        return "$" + amount;
    return currency + " " + amount;
}

std::string format_percent(double rate)
{
    return format_number(rate * 100.0) + "%";
}

// ─────────────────────────────────────────────────────────────────────────────
// Invoice renderer
// ─────────────────────────────────────────────────────────────────────────────

class InvoiceRenderer {
public:
    InvoiceRenderer(Document& document, const InvoiceStyle& style)
        : document_(document), style_(style)
    {
        body_font_ = create_font(style.body_font);
        bold_font_ = create_font(style.bold_font);

        root_ = std::make_unique<StructTreeRoot>(document.create_struct_tree_root());
        document_element_ = std::make_unique<StructElement>(root_->add_child("Document"));

        PDFString lang("en-US", document_, false);
        document_.get_root()->put("Lang", lang);
    }

    void render(const InvoiceInput& invoice, const std::vector<InvoiceLineItem>& items,
                const std::string& base_directory)
    {
        invoice_ = &invoice;
        new_page();
        render_header(base_directory);
        render_parties();
        render_line_items(items);
        render_totals(items);
        render_notes();
        page_->update_content();
    }

    int page_count() const { return page_count_; }

    static double subtotal(const std::vector<InvoiceLineItem>& items)
    {
        double total = 0;
        for (const auto& item : items)
            total += item.amount();
        return total;
    }

    static double tax(const std::vector<InvoiceLineItem>& items, double rate)
    {
        return round_away_from_zero(subtotal(items) * rate, 2);
    }

private:
    Document& document_;
    const InvoiceStyle& style_;
    const InvoiceInput* invoice_ = nullptr;
    std::unique_ptr<Font> body_font_;
    std::unique_ptr<Font> bold_font_;
    std::unique_ptr<StructTreeRoot> root_;
    std::unique_ptr<StructElement> document_element_;

    std::optional<Page> page_;
    std::optional<Content> content_;
    double cursor_y_ = 0;
    int page_count_ = 0;

    // ── Fonts / pages / tagging ───────────────────────────────

    static std::unique_ptr<Font> create_font(const std::string& name)
    {
        for (FontCreateFlags flags :
             {FontCreateFlags::Embedded | FontCreateFlags::Subset, FontCreateFlags::Subset}) {
            try {
                return std::make_unique<Font>(name, flags);
            } catch (const std::exception&) {
                continue;
            }
        }
        throw std::invalid_argument("Could not create font \"" + name +
                                    "\". Adjust bodyFont/boldFont in the style file.");
    }

    void new_page()
    {
        if (page_)
            page_->update_content();
        const int insert_after = page_count_ == 0 ? Document::before_first_page : page_count_ - 1;
        page_.emplace(document_.create_page(insert_after,
                                            Rect{0, 0, style_.page_width, style_.page_height}));
        content_.emplace(page_->get_content());
        cursor_y_ = style_.page_height - style_.margin;
        ++page_count_;
    }

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

    // ── Drawing primitives ────────────────────────────────────

    double measure(const std::string& text, Font& font, double size) const
    {
        try {
            return font.measure_text_width(text, size);
        } catch (const std::exception&) {
            return static_cast<double>(text.size()) * size * 0.55;
        }
    }

    void draw_text(const std::string& value, Font& font, double size, double x, double y,
                   const RgbColor& color, StructElement* owner, const std::string& tag)
    {
        if (value.empty())
            return;
        GraphicState gs;
        gs.set_fill_color(color.to_color());
        TextState ts;
        Text text;
        Matrix matrix(size, 0, 0, size, x, y);
        TextRun run(value, font, gs, ts, matrix);
        text.add_run(run);
        if (owner)
            add_tagged(text, tag, *owner);
        else
            add_artifact(text);
    }

    void draw_right_aligned_text(const std::string& value, Font& font, double size,
                                 double right_x, double y, const RgbColor& color,
                                 StructElement* owner, const std::string& tag)
    {
        draw_text(value, font, size, right_x - measure(value, font, size), y, color, owner, tag);
    }

    void draw_box(double x, double top_y, double width, double height, const RgbColor& fill,
                  const RgbColor& border)
    {
        Path box;
        GraphicState gs;
        gs.set_fill_color(fill.to_color());
        gs.set_stroke_color(border.to_color());
        gs.set_width(0.5);
        box.set_graphic_state(gs);
        box.set_paint_op(PathPaintOp::Fill | PathPaintOp::Stroke);
        box.add_rect(Point(x, top_y - height), width, height);
        add_artifact(box);
    }

    void draw_rule(double x1, double x2, double y, double width = 0.75)
    {
        Path line;
        GraphicState gs;
        gs.set_stroke_color(style_.border_color.to_color());
        gs.set_width(width);
        line.set_graphic_state(gs);
        line.set_paint_op(PathPaintOp::Stroke);
        line.move_to(Point(x1, y));
        line.add_line(Point(x2, y));
        add_artifact(line);
    }

    std::vector<std::string> wrap_text(const std::string& text, Font& font, double size,
                                       double max_width) const
    {
        std::vector<std::string> lines;
        std::istringstream stream(text);
        std::string word, current;
        while (stream >> word) {
            const std::string candidate = current.empty() ? word : current + " " + word;
            if (!current.empty() && measure(candidate, font, size) > max_width) {
                lines.push_back(current);
                current = word;
            } else {
                current = candidate;
            }
        }
        if (!current.empty())
            lines.push_back(current);
        if (lines.empty())
            lines.push_back("");
        return lines;
    }

    // Starts a continuation page when less than `needed_height` remains.
    bool ensure_room(double needed_height)
    {
        if (cursor_y_ - needed_height >= style_.margin)
            return false;
        new_page();
        StructElement sect = document_element_->add_child("Sect");
        draw_text("Invoice continued", *bold_font_, 11, style_.margin, cursor_y_,
                  style_.primary_color, &sect, "P");
        draw_rule(style_.margin, style_.page_width - style_.margin, cursor_y_ - 12);
        draw_footer(page_count_);
        cursor_y_ -= 36;
        return true;
    }

    void draw_footer(int page_number)
    {
        draw_rule(style_.margin, style_.page_width - style_.margin, style_.margin - 14);
        const double baseline = style_.margin - 31;
        const double size = 7.2;
        const CompanyInfo& seller = invoice_->seller;
        draw_text(seller.name, *body_font_, size, style_.margin, baseline,
                  style_.muted_text_color, nullptr, "");
        const std::string center = seller.email + " | " + seller.phone;
        draw_text(center, *body_font_, size,
                  (style_.page_width - measure(center, *body_font_, size)) / 2, baseline,
                  style_.muted_text_color, nullptr, "");
        const std::string page_label = "Page " + std::to_string(page_number);
        draw_text(page_label, *body_font_, size,
                  style_.page_width - style_.margin - measure(page_label, *body_font_, size),
                  baseline, style_.muted_text_color, nullptr, "");
    }

    // ── Sections ──────────────────────────────────────────────

    void render_header(const std::string& base_directory)
    {
        StructElement sect = document_element_->add_child("Sect");

        namespace fs = std::filesystem;
        if (!invoice_->logo_path.empty()) {
            const fs::path logo = fs::path(base_directory) / invoice_->logo_path;
            if (fs::exists(logo))
                draw_logo(logo.string(), sect);
        }

        const double right_x = style_.page_width - style_.margin;
        const double top_y = style_.page_height - style_.margin;
        StructElement heading = sect.add_child("H1");
        draw_right_aligned_text("INVOICE", *bold_font_, style_.heading_font_size, right_x,
                                top_y - 10, style_.primary_color, &heading, "H1");
        StructElement details = sect.add_child("P");
        draw_right_aligned_text("Invoice " + invoice_->invoice_number, *body_font_, 11, right_x,
                                top_y - 34, style_.text_color, &details, "P");
        draw_right_aligned_text("Issued " + invoice_->issue_date, *body_font_, 9, right_x,
                                top_y - 50, style_.muted_text_color, &details, "P");
        draw_right_aligned_text("Due " + invoice_->due_date, *body_font_, 9, right_x, top_y - 64,
                                style_.muted_text_color, &details, "P");

        draw_rule(style_.margin, right_x, top_y - 82);
        draw_footer(1);
        cursor_y_ = top_y - 112;
    }

    void draw_logo(const std::string& path, StructElement& sect)
    {
        Image logo(path, document_);
        Matrix design = logo.get_matrix();
        const double scale =
            std::min(style_.logo_max_width / design.a, style_.logo_max_height / design.d);
        logo.scale(scale, scale);
        logo.translate(style_.margin,
                       style_.page_height - style_.margin - style_.logo_max_height);

        StructElement figure = sect.add_child("Figure");
        Content inner;
        inner.add_element(logo);
        PDFDict props(document_, false);
        Container container("Figure", props, false);
        container.set_content(inner);
        content_->add_element(container);
        figure.add_marked_content_ref(*page_, container);

        PDFString alt("Seller logo", document_, false);
        figure.get_pdf_dict().put("Alt", alt);
    }

    void render_parties()
    {
        StructElement sect = document_element_->add_child("Sect");
        const double gutter = 18;
        const double column_width = (style_.page_width - 2 * style_.margin - gutter) / 2;
        const double block_height = 170;
        const double start_y = cursor_y_ + 6;

        draw_party_block(sect, "From", invoice_->seller, style_.margin, start_y, column_width,
                         block_height);
        draw_party_block(sect, "Bill To", invoice_->customer,
                         style_.margin + column_width + gutter, start_y, column_width,
                         block_height);
        cursor_y_ = start_y - block_height - 24;
    }

    void draw_party_block(StructElement& sect, const std::string& label,
                          const CompanyInfo& company, double x, double top_y, double width,
                          double height)
    {
        StructElement block = sect.add_child("P");
        draw_box(x, top_y, width, height, style_.accent_color, style_.border_color);
        const double inset = 14;
        const double text_x = x + inset;

        draw_text(label, *bold_font_, 9, text_x, top_y - 18, style_.primary_color, &block, "P");
        draw_text(company.name, *bold_font_, 12, text_x, top_y - 42, style_.text_color, &block,
                  "P");
        if (!company.tax_id.empty())
            draw_text(company.tax_id, *body_font_, 9, text_x, top_y - 62,
                      style_.muted_text_color, &block, "P");

        double current_y = top_y - 86;
        for (const auto& line : company.address_lines()) {
            draw_text(line, *body_font_, 9, text_x, current_y, style_.text_color, &block, "P");
            current_y -= 13;
        }
        draw_text(company.email, *body_font_, 9, text_x, current_y - 4, style_.text_color,
                  &block, "P");
        draw_text(company.phone, *body_font_, 9, text_x, current_y - 21, style_.text_color,
                  &block, "P");
    }

    struct TableLayout {
        std::vector<double> widths{72, 234, 54, 72, 72};
        std::vector<std::string> headers{"Item", "Description", "Qty", "Unit Price", "Amount"};

        double total_width() const
        {
            double total = 0;
            for (double w : widths)
                total += w;
            return total;
        }
    };

    void draw_table_header(const TableLayout& layout, StructElement& table)
    {
        StructElement row = table.add_child("TR");
        const double height = 22;
        Path box;
        GraphicState gs;
        gs.set_fill_color(style_.primary_color.to_color());
        gs.set_stroke_color(style_.primary_color.to_color());
        gs.set_width(0.5);
        box.set_graphic_state(gs);
        box.set_paint_op(PathPaintOp::Fill | PathPaintOp::Stroke);
        box.add_rect(Point(style_.margin, cursor_y_ + 5 - height), layout.total_width(), height);
        add_artifact(box);

        double x = style_.margin;
        const RgbColor white{1, 1, 1};
        for (size_t c = 0; c < layout.headers.size(); ++c) {
            StructElement cell = row.add_child("TH");
            draw_text(layout.headers[c], *bold_font_, style_.table_header_font_size, x + 6,
                      cursor_y_ - 9, white, &cell, "TH");
            x += layout.widths[c];
        }
        cursor_y_ -= height;
    }

    void draw_line_item_row(const TableLayout& layout, StructElement& table,
                            const InvoiceLineItem& item)
    {
        const double description_width = layout.widths[1] - 12;
        const std::vector<std::string> description_lines =
            wrap_text(item.description, *body_font_, style_.body_font_size, description_width);
        const double row_height =
            std::max(26.0, static_cast<double>(description_lines.size()) * 12.0 + 12.0);

        if (ensure_room(row_height + 44))
            draw_table_header(layout, table);

        StructElement row = table.add_child("TR");
        const RgbColor white{1, 1, 1};
        draw_box(style_.margin, cursor_y_, layout.total_width(), row_height, white,
                 style_.border_color);

        const double text_top = cursor_y_ - 16;
        double x = style_.margin;
        auto draw_cell = [&](size_t column, const std::string& value, Font& font,
                             bool right_aligned) {
            StructElement cell = row.add_child("TD");
            if (right_aligned) {
                draw_right_aligned_text(value, font, style_.body_font_size,
                                        x + layout.widths[column] - 6, text_top,
                                        style_.text_color, &cell, "TD");
            } else {
                draw_text(value, font, style_.body_font_size, x + 6, text_top,
                          style_.text_color, &cell, "TD");
            }
            x += layout.widths[column];
        };

        draw_cell(0, item.item_code, *body_font_, false);
        {
            StructElement cell = row.add_child("TD");
            double line_y = text_top;
            for (const auto& line : description_lines) {
                draw_text(line, *body_font_, style_.body_font_size, x + 6, line_y,
                          style_.text_color, &cell, "TD");
                line_y -= 12;
            }
            x += layout.widths[1];
        }
        draw_cell(2, format_number(item.quantity), *body_font_, true);
        draw_cell(3, format_currency(item.unit_price, invoice_->currency), *body_font_, true);
        draw_cell(4, format_currency(item.amount(), invoice_->currency), *bold_font_, true);

        cursor_y_ -= row_height;
    }

    void render_line_items(const std::vector<InvoiceLineItem>& items)
    {
        ensure_room(120);
        StructElement table = document_element_->add_child("Table");
        TableLayout layout;
        draw_table_header(layout, table);
        for (const auto& item : items)
            draw_line_item_row(layout, table, item);
    }

    void render_totals(const std::vector<InvoiceLineItem>& items)
    {
        ensure_room(132);
        StructElement sect = document_element_->add_child("Sect");

        const double box_width = 214;
        const double box_height = 92;
        const double inset = 14;
        const double box_x = style_.page_width - style_.margin - box_width;
        const double box_top = cursor_y_ - 6;
        draw_box(box_x, box_top, box_width, box_height, style_.accent_color,
                 style_.border_color);

        const double label_x = box_x + inset;
        const double value_right = box_x + box_width - inset;
        const double sub = subtotal(items);
        const double tax_amount = tax(items, invoice_->tax_rate);

        double y = box_top - 24;
        auto total_line = [&](const std::string& label, const std::string& value, Font& font,
                              double size) {
            StructElement line = sect.add_child("P");
            draw_text(label, font, size, label_x, y, style_.text_color, &line, "P");
            draw_right_aligned_text(value, font, size, value_right, y, style_.text_color, &line,
                                    "P");
        };
        total_line("Subtotal", format_currency(sub, invoice_->currency), *body_font_,
                   style_.body_font_size);
        y -= 20;
        total_line("Tax (" + format_percent(invoice_->tax_rate) + ")",
                   format_currency(tax_amount, invoice_->currency), *body_font_,
                   style_.body_font_size);
        y -= 16;
        draw_rule(label_x, value_right, y);
        y -= 20;
        total_line("Total", format_currency(sub + tax_amount, invoice_->currency), *bold_font_,
                   12);

        cursor_y_ = box_top - box_height - 28;
    }

    void draw_paragraph(const std::string& text, StructElement& owner)
    {
        const double width = style_.page_width - 2 * style_.margin;
        for (const auto& line : wrap_text(text, *body_font_, style_.body_font_size, width)) {
            ensure_room(16);
            draw_text(line, *body_font_, style_.body_font_size, style_.margin, cursor_y_,
                      style_.text_color, &owner, "P");
            cursor_y_ -= 13;
        }
    }

    void render_notes()
    {
        ensure_room(96);
        StructElement sect = document_element_->add_child("Sect");

        StructElement terms_heading = sect.add_child("H2");
        draw_text("Payment Terms", *bold_font_, 11, style_.margin, cursor_y_,
                  style_.primary_color, &terms_heading, "H2");
        cursor_y_ -= 16;
        StructElement terms = sect.add_child("P");
        draw_paragraph(invoice_->payment_terms, terms);

        cursor_y_ -= 10;
        StructElement notes_heading = sect.add_child("H2");
        draw_text("Notes", *bold_font_, 11, style_.margin, cursor_y_, style_.primary_color,
                  &notes_heading, "H2");
        cursor_y_ -= 16;
        StructElement notes = sect.add_child("P");
        draw_paragraph(invoice_->notes, notes);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Self-tests: validate parsing and totals against the bundled data files.
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

    try {
        const auto fields = parse_csv_row(
            "\"FIELD-102\",\"Quarterly garden planning, includes layout notes\",2,125.50");
        require(fields.size() == 4, "CSV row field count");
        require(fields[1] == "Quarterly garden planning, includes layout notes",
                "quoted comma preserved");

        const CommandLineOptions defaults = parse_options({});
        require(defaults.metadata_path == "data/metadata.json", "default metadata path");
        require(defaults.line_items_path == "data/line-items.csv", "default line items path");
        require(defaults.style_path == "data/style.json", "default style path");

        const InvoiceInput invoice = load_invoice(defaults.metadata_path);
        const std::vector<InvoiceLineItem> items = load_line_items(defaults.line_items_path);
        const InvoiceStyle style = load_style(defaults.style_path);

        require(!invoice.logo_path.empty(), "logo path present");
        require(invoice.apply_restriction_password && !invoice.restriction_password.empty(),
                "restriction password enabled");
        namespace fs = std::filesystem;
        const fs::path base = fs::absolute(defaults.metadata_path).parent_path();
        require(fs::exists(base / invoice.logo_path), "logo file exists");
        require(items.size() >= 20, "at least 20 line items");

        const double sub = InvoiceRenderer::subtotal(items);
        const double tax = InvoiceRenderer::tax(items, invoice.tax_rate);
        require(sub > 0, "subtotal positive");
        require(tax > 0, "tax positive");
        require(style.page_width > 0 && style.page_height > 0, "style page size positive");

        require(format_currency(1234.5, "USD") == "$1,234.50", "currency formatting");
        require(format_percent(0.0825) == "8.25%", "percent formatting");
        require(format_number(2.0) == "2", "quantity formatting");
    } catch (const std::exception& e) {
        std::cout << "FAILED: " << e.what() << std::endl;
        ++failures;
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
    std::cout << "CreateInvoiceFromStructuredData Sample:" << std::endl;

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

        if (args.size() == 1 && to_lower(args[0]) == "--self-test")
            return run_self_tests();

        const CommandLineOptions options = parse_options(args);
        const InvoiceInput invoice = load_invoice(options.metadata_path);
        const std::vector<InvoiceLineItem> items = load_line_items(options.line_items_path);
        const InvoiceStyle style = load_style(options.style_path);

        namespace fs = std::filesystem;
        const std::string base_directory =
            fs::absolute(options.metadata_path).parent_path().string();

        Document document;
        document.set_title("Invoice " + invoice.invoice_number);
        document.set_producer("CreateInvoiceFromStructuredData sample using Datalogics APDFL");

        InvoiceRenderer renderer(document, style);
        renderer.render(invoice, items, base_directory);

        document.embed_fonts();

        if (invoice.apply_restriction_password) {
            // Viewing, printing, copying, and accessibility stay available;
            // editing requires the owner password.
            document.secure(PermissionFlags::Open | PermissionFlags::Print |
                                PermissionFlags::HighPrint | PermissionFlags::Copy |
                                PermissionFlags::Accessible | PermissionFlags::SaveAs,
                            invoice.restriction_password, "", EncryptionType::AES256_AcroX,
                            true);
        }

        document.save(SaveFlags::Full, options.output_path);
        std::cout << "Created tagged invoice PDF: " << options.output_path << std::endl;
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
