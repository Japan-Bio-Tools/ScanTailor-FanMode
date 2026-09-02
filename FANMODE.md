# ScanTailor FanMode project notes

## Provenance

ScanTailor FanMode is an independent community fork of
[ImageProcessing-ElectronicPublications/scantailor-advanced](https://github.com/ImageProcessing-ElectronicPublications/scantailor-advanced).

- FanMode version: `0.1.0-alpha.0`
- Upstream baseline commit: `5e20d076cb6168fe65ebab0e1139e7ea6924d27e`
- License: GNU General Public License version 3, inherited from ScanTailor Advanced
- Primary platform: local Windows x64 desktop
- Product executable: `scantailor-fanmode.exe`
- Upstream remote: `upstream`
- FanMode development branch: `fanmode`

The separate ScanFan application was used as a requirements and interaction prototype. FanMode is implemented in
the ScanTailor Advanced C++/Qt codebase; prototype code is not copied into the fork without deliberate review.

FanMode is not an official release of the original ScanTailor or ScanTailor Advanced maintainers. Names and links
are retained to give clear credit and make the source lineage auditable.

## Implemented FanMode features

### Project creation and ordering

- Import a PDF from the New Project dialog.
- Choose conversion DPI and convert pages to lossless LZW TIFF with Poppler.
- Create a separate TIFF input directory and select it automatically when conversion completes.
- Use left-to-right or right-to-left natural page ordering for English, Arabic, and Hebrew books.
- Open and migrate older compatible version-3 `.ScanTailor` projects.
- Use Windows high-DPI scaling consistent with ScanTailor Advanced.

### Page Layout

- Automatic page sizing.
- Custom physical page width and height.
- A front-cover reference mode based on the marked cover's final content and margins.
- Independent top, bottom, left, and right margins, including automatic margins.
- Automatic, manual nine-position, or original-position alignment.
- Page-specific settings with ScanTailor's Apply To selection workflow.

### Select Content

- The inherited standard automatic and manual modes.
- An Accurate automatic mode that runs an additional high-resolution analysis for isolated content.
- Separate per-page controls to include a header/title and include a page number.
- A one-page re-detection action.
- Saved detection choices and dependency tracking so stale cached results are recalculated.

### Output and covers

- Mark a page as the front cover or back cover.
- Order marked covers at the beginning and end of the final book.
- Stretch each marked cover to its full output page when requested.
- Preserve FanMode cover, sizing, content, and dewarping settings in the project.

### Dewarping

- A book-text automatic method intended for Arabic and English text lines.
- A table/technical-page automatic method that also uses long ruled lines and borders.
- The inherited hybrid and marginal experimental methods.
- A simple manual editor for the top and bottom curves, plus left and right border controls.
- The full advanced grid for difficult pages.
- Preview correction directly in the main page view.
- Reset to a normal grid and re-detect only the current page.
- Manual edits change the page to Manual mode and are saved immediately.

### OCR and PDF export

- A final OCR & PDF filter after Output.
- Arabic, English, or combined Arabic/English Tesseract OCR.
- Logical-order correction for Arabic/RTL searchable text using QPDF.
- Safe staging of pages whose paths contain Arabic or other non-ASCII characters.
- PDF compression presets and an approximate output-size display.
- Image-only PDF export when no OCR language is selected.
- Native loading of final TIFF pages for image-only export, without relying on an optional Qt TIFF plugin.

### Windows delivery

- Native MSYS2 UCRT64 build script.
- Portable x64 package script that recursively bundles required DLLs, Qt plugins, OCR data, and licenses.
- Inno Setup installer definition and build script.
- Regression tests for content selection, dewarping options, table-line dewarping, and image-only PDF writing.

## Compatibility and data safety

FanMode retains the `.ScanTailor` project structure and adds namespaced settings needed by its new functions. It can
migrate the supported older project version, but alpha files should still be treated as forward-moving: always keep
an untouched copy of important images and projects before opening them in a new build.

All processing is local. Project source images and generated Output TIFF pages remain in user-selected folders. The
repository ignore rules intentionally reject PDFs, `.ScanTailor` files, converted `*_tiff` directories, local QA
folders, build directories, installers, and archives so scanned books are not accidentally committed.

## Repository workflow

`upstream` tracks ScanTailor Advanced and is fetch-only. FanMode work is committed to `fanmode` and published through
`origin`. Upstream changes should be fetched, reviewed, and integrated deliberately because processing and project
serialization changes may conflict with FanMode behavior.

New contributions must retain upstream copyright notices, follow GPL-3.0, include focused tests where practical,
and never add copyrighted or private scanned-book material.
