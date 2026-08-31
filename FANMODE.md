# ScanTailor FanMode

## Baseline

- FanMode version: `0.1.0-alpha.0`
- Source engine: `ImageProcessing-ElectronicPublications/scantailor-advanced`
- Baseline commit: `5e20d076cb6168fe65ebab0e1139e7ea6924d27e`
- License: GNU GPL version 3, inherited from ScanTailor Advanced
- Primary platform: local Windows desktop
- Project compatibility: existing `.ScanTailor` project format is preserved

The existing ScanFan Beta project remains a separate requirements and interaction prototype. Code is not copied
from ScanFan into this C++/Qt fork without a deliberate port and review.

## Repository workflow

The `upstream` Git remote tracks the maintained ScanTailor Advanced source. The `fanmode` branch contains FanMode
changes. A future GitHub repository should be added as `origin`; day-to-day FanMode work is pushed only to
`origin`, while upstream changes are fetched from `upstream` and integrated deliberately.

## First milestone

The first baseline changes only product identity and documentation. Processing behavior remains inherited from
ScanTailor Advanced until FanMode requirements are added and approved.
