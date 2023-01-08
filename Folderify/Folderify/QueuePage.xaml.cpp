// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "QueuePage.xaml.h"
#if __has_include("QueuePage.g.cpp")
#include "QueuePage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

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
