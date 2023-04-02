#pragma once
#include "QueuePageViewModel.g.h"
#include "SongInfo.h"

namespace winrt::Folderify::implementation
{
    struct QueuePageViewModel : QueuePageViewModelT<QueuePageViewModel>
    {
        QueuePageViewModel();

        winrt::Windows::Foundation::Collections::IObservableVector<winrt::Folderify::SongInfo> Songs();

    private:
		winrt::Windows::Foundation::Collections::IObservableVector<winrt::Folderify::SongInfo> m_songs;
    };
}
namespace winrt::Folderify::factory_implementation
{
    struct QueuePageViewModel : QueuePageViewModelT<QueuePageViewModel, implementation::QueuePageViewModel>
    {
    };
}
