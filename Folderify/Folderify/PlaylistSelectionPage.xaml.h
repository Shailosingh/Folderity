// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "PlaylistSelectionPage.g.h"

namespace winrt::Folderify::implementation
{
    struct PlaylistSelectionPage : PlaylistSelectionPageT<PlaylistSelectionPage>
    {
        PlaylistSelectionPage();

        int32_t MyProperty();
        void MyProperty(int32_t value);
    };
}

namespace winrt::Folderify::factory_implementation
{
    struct PlaylistSelectionPage : PlaylistSelectionPageT<PlaylistSelectionPage, implementation::PlaylistSelectionPage>
    {
    };
}
