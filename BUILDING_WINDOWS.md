# Building ScanTailor FanMode on Windows

FanMode's tested native toolchain is 64-bit MSYS2 UCRT with CMake and Ninja. The helper scripts expect MSYS2 under
`%LOCALAPPDATA%\Programs\MSYS2`; adjust the scripts if your installation is elsewhere.

## Prerequisites

Install the UCRT64 variants of the C/C++ toolchain, CMake, Ninja, Qt, Boost, JPEG, PNG, TIFF, zlib, Poppler, Tesseract
with Arabic and English trained data, Leptonica, and QPDF. Inno Setup 6 is needed only to build the installer.

The Windows package uses these external components:

- Poppler `pdftoppm` for PDF-to-TIFF project import.
- Tesseract and `ara` / `eng` trained data for searchable PDFs.
- QPDF for Arabic logical-order correction and image-only PDF generation.
- Qt and the inherited ScanTailor Advanced image-processing libraries for the desktop application.

## Build and test

From PowerShell in the repository root:

```powershell
.\scripts\build-windows-native.ps1 -Configuration Release
```

The script configures `build-fanmode`, builds `scantailor-fanmode.exe`, and runs the CTest suite. For a quicker local
compile without tests, add `-SkipTests`.

## Create a portable package

After a successful Release build:

```powershell
.\scripts\package-windows-native.ps1 -Version 0.1.0-alpha.0
```

The versioned folder and ZIP are created under `artifacts`. The script validates PDF/OCR tools and language data,
copies Qt plugins, discovers dependent MinGW DLLs recursively, and includes third-party license files.

## Create the installer

With Inno Setup 6 installed and the portable package already built:

```powershell
.\scripts\package-windows-installer.ps1 -Version 0.1.0-alpha.0
```

The installer is written to `artifacts`. Build products are ignored by Git and are not stored in the source
repository.

## Linux and other platforms

The inherited CMake project remains cross-platform, but FanMode's complete PDF/OCR bundle and current QA workflow
are tested primarily on Windows x64. Linux contributors need equivalent development packages and runtime programs.
Platform-specific packaging inherited from ScanTailor Advanced may still refer to upstream product names until it
is fully adapted and tested.
