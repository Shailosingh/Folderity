// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "QueuePage.xaml.h"
#if __has_include("QueuePage.g.cpp")
#include "QueuePage.g.cpp"
#endif

#include "MusicController.h"
#include "SharedWindowVariables.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::Folderify::implementation
{
    QueuePage::QueuePage()
    {
        InitializeComponent();
    }

    int32_t QueuePage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void QueuePage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}
