// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "InstructionPage.xaml.h"
#if __has_include("InstructionPage.g.cpp")
#include "InstructionPage.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Folderify::implementation
{
    InstructionPage::InstructionPage()
    {
        InitializeComponent();
    }

    int32_t InstructionPage::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void InstructionPage::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}
