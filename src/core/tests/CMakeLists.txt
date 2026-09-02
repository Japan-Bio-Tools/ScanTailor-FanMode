set(sources
    main.cpp
    TestContentSpanFinder.cpp
    TestDeskewParams.cpp
    TestDewarpingOptions.cpp
    TestObliqueFinder.cpp
    TestOcrInputStager.cpp
    TestOcrSettings.cpp
    TestOutputCover.cpp
    TestImageId.cpp
    TestImagePdfWriter.cpp
    TestMargins.cpp
    TestPageLayoutPageSize.cpp
    TestPageId.cpp
    TestPageRange.cpp
    TestPageSequence.cpp
    TestProjectReaderCompatibility.cpp
    TestRtlPdfFixer.cpp
    TestSelectContentAccurate.cpp
    TestSelectContentApply.cpp
    TestSmartFilenameOrdering.cpp
    TestTableLineDistortionModelBuilder.cpp
    TestUnits.cpp)

add_executable(core_tests ${sources})
target_link_libraries(
    core_tests
    PRIVATE core dewarping Boost::unit_test_framework ${EXTRA_LIBS})

add_test(NAME core_tests COMMAND core_tests --log_level=message)

# Small developer utility for exercising the production RTL text-layer rewrite
# against real Tesseract PDF / TSV pairs.
add_executable(rtl_pdf_fixer_tool RtlPdfFixerTool.cpp)
target_link_libraries(rtl_pdf_fixer_tool PRIVATE output Qt::Core)
