// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "SongsPage.g.h"

namespace winrt::Folderify::implementation
{
    struct SongsPage : SongsPageT<SongsPage>
    {
        SongsPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);
    };
}

namespace winrt::Folderify::factory_implementation
{
    struct SongsPage : SongsPageT<SongsPage, implementation::SongsPage>
    {
    };
}
