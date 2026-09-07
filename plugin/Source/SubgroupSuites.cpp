// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Vixen420
//
// Subgroup is free software: you may redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version. It comes with ABSOLUTELY NO WARRANTY. See the file LICENSE, or
// <https://www.gnu.org/licenses/>, for the full text.
//
// Additional permission under GPL-3.0 section 7: this file may be combined with
// the Adobe Illustrator SDK, whose sample framework sources are compiled into
// every plugin built from it. See LICENSE-EXCEPTION.

#include "IllustratorSDK.h"
#include "SubgroupSuites.h"

extern "C" {
    SPBlocksSuite*          sSPBlocks           = nullptr;
    AIArtSuite*             sAIArt              = nullptr;
    AIMatchingArtSuite*     sAIMatchingArt      = nullptr;
    AIMdMemorySuite*        sAIMdMemory         = nullptr;
    AIMenuSuite*            sAIMenu             = nullptr;
    AIPreferenceSuite*      sAIPreference       = nullptr;
    AITransformArtSuite*    sAITransformArt     = nullptr;
    AIUnicodeStringSuite*   sAIUnicodeString    = nullptr;
};

ImportSuite gImportSuites[] =
{
    kSPBlocksSuite,         kSPBlocksSuiteVersion,      &sSPBlocks,
    kAIArtSuite,            kAIArtSuiteVersion,         &sAIArt,
    kAIMatchingArtSuite,    kAIMatchingArtSuiteVersion, &sAIMatchingArt,
    kAIMdMemorySuite,       kAIMdMemorySuiteVersion,    &sAIMdMemory,
    kAIMenuSuite,           kAIMenuSuiteVersion,        &sAIMenu,
    kAIPreferenceSuite,     kAIPreferenceSuiteVersion,  &sAIPreference,
    kAITransformArtSuite,   kAITransformArtSuiteVersion, &sAITransformArt,
    kAIUnicodeStringSuite,  kAIUnicodeStringVersion,    &sAIUnicodeString,
    nullptr, 0, nullptr
};
