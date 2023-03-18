#include "pch.h"
#include "MainWindow.xaml.h"
#include "winrt/Windows.UI.Xaml.Interop.h"
#if __has_include("MainWindow.g.cpp")
#include "MainWindow.g.cpp"
#endif

#include "MusicController.h"
#include "SharedWindowVariables.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::Folderify::implementation
{
    MainWindow::MainWindow()
    {
        InitializeComponent();

		//Pass the main window to the MusicController and create instance
        try
        {
            ControllerObject = new MusicController(this);
        }
		catch (const std::exception& e)
		{
			std::string error = e.what();
            OutputDebugStringA(error.c_str());
            exit(1);
		}
    }

    int32_t MainWindow::MyProperty()
    {
        throw hresult_not_implemented();
    }

    void MainWindow::MyProperty(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    void MainWindow::MainMenu_ItemInvoked(winrt::Microsoft::UI::Xaml::Controls::NavigationView const& sender, winrt::Microsoft::UI::Xaml::Controls::NavigationViewItemInvokedEventArgs const& args)
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


    void MainWindow::LoopButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e)
    {
        ControllerObject->LoopToggle();
    }


    void MainWindow::ShuffleButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e)
    {
        //TODO: Shuffle current queue
    }
    
    void MainWindow::PreviousButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e)
    {
		ControllerObject->Previous();
    }


    void MainWindow::PlayPauseButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e)
    {
		if (ControllerObject->GetPlayerState() == MMFSoundPlayerLib::Playing)
		{
			ControllerObject->Pause();
		}
        else if (ControllerObject->GetPlayerState() == MMFSoundPlayerLib::Paused)
        {
            ControllerObject->Play();
        }
    }


    void MainWindow::NextButton_Tapped(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::TappedRoutedEventArgs const& e)
    {
        ControllerObject->Next();
    }

    void MainWindow::Window_Closed(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::WindowEventArgs const& args)
    {
        ControllerObject->CloseController();
        delete ControllerObject;
    }

    void MainWindow::TrackBar_ManipulationStarting(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::ManipulationStartingRoutedEventArgs const& e)
    {
        ControllerObject->SongPositionBarHeld = true;
        ControllerObject->Seek(TrackBar().Value());
        ControllerObject->SongPositionBarHeld = false;
        //Implement ManipulationStarted
    }

    void MainWindow::TrackBar_ManipulationStarted(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::ManipulationStartedRoutedEventArgs const& e)
    {
        ControllerObject->SongPositionBarHeld = true;
    }

    void MainWindow::TrackBar_ManipulationCompleted(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::ManipulationCompletedRoutedEventArgs const& e)
    {
        ControllerObject->Seek(TrackBar().Value());
        ControllerObject->SongPositionBarHeld = false;
    }

    /*
    void MainWindow::TrackBar_PointerPressed(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
		ControllerObject->SongPositionBarHeld = true;
    }


    void MainWindow::TrackBar_PointerReleased(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
        ControllerObject->Seek(TrackBar().Value());
		ControllerObject->SongPositionBarHeld = false;
    }

    void MainWindow::TrackBar_PointerCanceled(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& e)
    {
		if (ControllerObject->SongPositionBarHeld)
		{
            ControllerObject->Seek(TrackBar().Value());
			ControllerObject->SongPositionBarHeld = false;
		}
    }
    */
}
