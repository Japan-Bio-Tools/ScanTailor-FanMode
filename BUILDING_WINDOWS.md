# Building ScanTailor FanMode on Windows

FanMode's tested native toolchain is 64-bit MSYS2 UCRT with CMake and Ninja. The helper scripts expect MSYS2 under
`%LOCALAPPDATA%\Programs\MSYS2`; pass `-MsysRoot C:\path\to\msys64` to the scripts if your installation is elsewhere.

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

## GitHub Actions：WindowsポータブルZIP

[Windows portable](../../actions/workflows/windows-portable.yml) は `ci/windows-portable` ブランチへのpush、
またはActions画面の **Run workflow** で実行する。MSYS2 UCRT64／Qt5で既存スクリプトのReleaseビルドと
全CTestを実行し、既存パッケージング処理でZIPを作成する。インストーラー・Release・タグは作成しない。

ZIPを新しいフォルダーへ展開して、MSYS2をPATHから除いた環境で次を確認してからアップロードする。

- DLLの再帰的な依存解決とx64形式、Qtプラグイン、英語／アラビア語データ、ライセンスの同梱
- Tesseractによる検索可能PDFの作成、QPDFでの検証、PopplerによるTIFFへの再変換
- Windowsプラグインを使ったアプリのメインウィンドウ表示

成功した実行の **Artifacts** から `ScanTailor-FanMode-0.1.0-alpha.0-win-x64` をダウンロードする（保存期間30日）。
Artifactを展開し、中の同名ZIPも展開して `scantailor-fanmode.exe` を起動する。
DLL・サブフォルダーを含め、展開したフォルダー全体を保持する。MSYS2や開発ツールの手動インストールは不要。
`windows-build-diagnostics` には依存パッケージ一覧・テストログ・ZIP検証結果を保存する。

CI上の起動確認は、利用者のWindows PCでの起動や日本語縦書き原稿の補正結果を保証しない。
日本語OCR・指除去機能の追加や、PDF入力・湾曲補正・PDF出力の処理変更は行わない。
