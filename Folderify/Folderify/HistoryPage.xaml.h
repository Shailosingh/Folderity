// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "HistoryPage.g.h"

namespace winrt::Folderify::implementation
{
    struct HistoryPage : HistoryPageT<HistoryPage>
    {
        HistoryPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);
    };
}

namespace winrt::Folderify::factory_implementation
{
    struct HistoryPage : HistoryPageT<HistoryPage, implementation::HistoryPage>
    {
    };
}
