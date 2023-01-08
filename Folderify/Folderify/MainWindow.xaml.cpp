// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#include "pch.h"
#include "MainWindow.xaml.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

using namespace winrt;
using namespace Microsoft::UI::Xaml;

// To learn more about WinUI, the WinUI project structure,
// and more about our project templates, see: http://aka.ms/winui-project-info.

namespace winrt::Folderify::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();
    }

    int32_t MainWindow::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainWindow::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}


void Folderify::implementation::MainWindow::MainMenu_ItemInvoked(winrt::Microsoft::UI::Xaml::Controls::NavigationView const& sender, winrt::Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs const& args)
{
    //First check if Settings was tapped
    if (args.IsSettingsInvoked())
    {
        //Navigate to Settings Page
        ContentFrame().Navigate(xaml_typename<Folderify::SettingsPage>());
    }

    //Handle navigations to every page
    if (args.InvokedItemContainer() != nullptr)
    {
        IInspectable tag = args.InvokedItemContainer().Tag();
        hstring tagValue = unbox_value<hstring>(tag);

        if (tagValue == L"Queue")
        {
			ContentFrame().Navigate(xaml_typename<Folderify::QueuePage>());
		}
        else if (tagValue == L"Playlists")
        {
            ContentFrame().Navigate(xaml_typename<Folderify::PlaylistSelectionPage>());
        }
        else if (tagValue == L"Songs")
        {
            ContentFrame().Navigate(xaml_typename<Folderify::SongsPage>());
        }
		else if (tagValue == L"History")
		{
			ContentFrame().Navigate(xaml_typename<Folderify::HistoryPage>());
		}
    }
}
