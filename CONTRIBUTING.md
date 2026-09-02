# Contributing

ScanTailor FanMode is a GPL-3.0 community fork of ScanTailor Advanced. Contributions should be focused, preserve
existing copyright notices, and explain any change to processing output or `.ScanTailor` project serialization.

Before submitting a change:

1. Build the application and run the CTest suite.
2. Add or update a focused regression test when practical.
3. Test both left-to-right and right-to-left project ordering when page order is affected.
4. Test paths containing spaces and non-ASCII characters when file handling is affected.
5. Update the README or changelog for user-visible behavior.

Never commit scanned books, PDFs, `.ScanTailor` projects, converted TIFF folders, personal test pages, OCR output,
build products, portable ZIPs, or installers. Use synthetic fixtures that you have the right to redistribute for any
test data that truly must enter the repository.

The `upstream` remote is reserved for fetching ScanTailor Advanced. FanMode changes belong on a feature branch and
should be merged into the FanMode development branch only after review and tests.
