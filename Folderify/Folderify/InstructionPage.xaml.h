// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "InstructionPage.g.h"

namespace winrt::Folderify::implementation
{
    struct InstructionPage : InstructionPageT<InstructionPage>
    {
        InstructionPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);
    };
}

namespace winrt::Folderify::factory_implementation
{
    struct InstructionPage : InstructionPageT<InstructionPage, implementation::InstructionPage>
    {
    };
}
