// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "QueuePage.g.h"

namespace winrt::Folderify::implementation
{
    struct QueuePage : QueuePageT<QueuePage>
    {
        QueuePage();

        int32_t MyProperty();
        void MyProperty(int32_t value);

        //Events
        void OnNavigatedTo(Microsoft::UI::Xaml::Navigation::NavigationEventArgs const& e);

    };
}

namespace winrt::Folderify::factory_implementation
{
    struct QueuePage : QueuePageT<QueuePage, implementation::QueuePage>
    {
    };
}
