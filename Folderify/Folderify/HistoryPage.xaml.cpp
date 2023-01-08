// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "HistoryPage.xaml.h"
#if __has_include("HistoryPage.g.cpp")
#include "HistoryPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Folderify::implementation
{
    HistoryPage::HistoryPage()
    {
        InitializeComponent();
    }

    int32_t HistoryPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void HistoryPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}
