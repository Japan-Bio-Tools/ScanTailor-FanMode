# Changelog

All notable FanMode changes are recorded here. The inherited ScanTailor Advanced history remains available in Git.

## 0.1.0-alpha.0

### Added

- FanMode product identity and Windows high-DPI behavior.
- PDF-to-lossless-TIFF import in New Project with automatic input-folder selection.
- Front-cover and back-cover page roles, natural reordering, and full-page cover stretching.
- Automatic, custom, and front-cover-reference output page sizes.
- Accurate content detection with explicit header/title and page-number retention.
- Book-text and table/technical-page automatic dewarping methods.
- Simplified manual dewarping curves, border controls, normal-grid reset, page re-detection, and in-view preview.
- Final Arabic/English OCR filter, bilingual searchable PDF export, compression choices, and size estimation.
- Image-only PDF export when no OCR language is selected.
- Native Windows portable and installer packaging.

### Fixed

- Arabic searchable-PDF text order and mixed Arabic/English handling.
- OCR/export failures for page paths containing Arabic or other non-ASCII characters.
- Image-only PDF reads of final TIFF pages when a Qt TIFF image plugin is unavailable.
- Page preview order refresh after output settings change.
- Dewarping cache invalidation when automatic methods or manual models change.
- Opening and safely migrating supported version-3 project files.
- Missing Windows runtime DLLs in portable and installer packages.

### Tests

- Added regression coverage for Accurate content detection and isolated page elements.
- Added dewarping option persistence and table-line model tests.
- Added real TIFF image-only PDF creation coverage.
