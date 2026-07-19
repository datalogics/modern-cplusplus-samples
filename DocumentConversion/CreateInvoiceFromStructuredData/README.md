# CreateInvoiceFromStructuredData

Builds a tagged PDF invoice from structured input files with the Datalogics
C++ APDFL API: JSON metadata (seller, customer, dates, tax rate), a CSV of
line items, a JSON style sheet (fonts, colors, page dimensions), and a local
PNG logo. Everything is drawn directly with APDFL — text positioned with real
font metrics, boxes and rules as vector paths, and the logo placed with
`Image` — while the logical structure tree is built alongside the content so
the invoice is accessible.

The generated invoice includes:

- A header with the scaled logo (tagged `Figure` with alt text), the invoice
  number, and issue/due dates
- "From" and "Bill To" cards for the seller and customer
- A line-item table (`Table` → `TR` → `TH`/`TD`) whose header row repeats on
  every continuation page, with right-aligned quantities and currency values
- A totals box with subtotal, tax, and grand total
- Payment terms and notes sections, and a decorative footer emitted as
  artifact content outside the structure tree

When `applyRestrictionPassword` is enabled in the metadata (the default), the
document is secured with `Document::secure` using 256-bit AES: it opens,
prints, and remains accessible without a password, but editing requires the
owner password from the metadata file.

## Building and running

Run from this directory so the default `data/` paths resolve:

```sh
make          # or open CreateInvoiceFromStructuredData.vcxproj in Visual Studio
./create_invoice_from_structured_data             # uses the bundled data files
./create_invoice_from_structured_data --help      # all options
./create_invoice_from_structured_data --self-test # validate inputs, no PDF written
```

Options: `--metadata <path>`, `--line-items <path>`, `--style <path>`,
`--output <path>`. The logo path in the metadata file is resolved relative to
the metadata file's directory.

Set `APDFL_LICENSE_KEY` to provide a Datalogics APDFL activation key before
the library initializes.

## Input files

- `data/metadata.json` — invoice number, dates, currency, tax rate, seller
  and customer contact blocks, logo path, and the optional restriction
  password. All data is fictional.
- `data/line-items.csv` — `itemCode,description,quantity,unitPrice` rows;
  quoted fields may contain commas.
- `data/style.json` — fonts, font sizes, page size, margin, logo bounds, and
  `#RRGGBB` colors.
- `images/northstar-logo.png` — the sample logo.

## Differences from the .NET sample

This is a rewrite of the `apdfl-csharp-dotnet-samples` sample of the same
name using the Modern C++ interface. The Modern C++ API assigns marked-content
IDs and maintains the structure parent tree automatically
(`StructElement::add_marked_content_ref`), so the tagging code is
substantially simpler than the .NET version. JSON and CSV parsing is done
with a small parser inside the sample, so APDFL remains the only dependency.
