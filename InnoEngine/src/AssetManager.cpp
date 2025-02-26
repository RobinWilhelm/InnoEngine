#include "iepch.h"
#include "AssetManager.h"

#include "Asset.h"
#include "AssetView.h"
#include "AssetRepository.h"

AssetManager::~AssetManager()
{
    if ( m_multithreaded )
        stop_async_loader();

    for ( auto& repo : m_assetRepos )
        repo.second.reset();

    m_assetRepos.clear();
}

std::optional<std::unique_ptr<AssetManager>> AssetManager::create( std::filesystem::path assetDir, bool multithreaded )
{
    if ( std::filesystem::exists( assetDir ) && std::filesystem::is_directory( assetDir ) ) {
        std::unique_ptr<AssetManager> assetManager( new AssetManager() );
        assetManager->m_assetDirectoryPath = assetDir;

        if ( multithreaded ) {
            assetManager->start_async_loader();
            IE_LOG_INFO( "AssetManager: Enabled multithreading" );
        }

        IE_LOG_INFO("Set asset directory to: \"%s\"", assetDir.string().c_str());
        return assetManager;
    } else {
        IE_LOG_ERROR( "Asset directory not found!" );
        return std::nullopt;
    }
}

void AssetManager::start_async_loader()
{
    if ( m_run == true )
        return;

    m_multithreaded = true;
    m_run           = true;
    m_loaderThread  = std::thread( &AssetManager::loader_loop, this );
}

void AssetManager::stop_async_loader()
{
    m_run.store( false, std::memory_order_relaxed );
    m_loaderThread.join();
    m_loadQueue.clear();
}

void AssetManager::loader_loop()
{
    while ( m_run.load( std::memory_order_relaxed ) ) {
        std::pair<std::string, std::pair<std::shared_ptr<AssetRepositoryBase>, std::shared_ptr<AssetHandle>>> queueEntry;
        {
            std::unique_lock<std::mutex> ulock( m_loadQueueMutex );
            m_loadQueueCV.wait( ulock, [this] { return m_loadQueue.empty() == false; } );
            queueEntry = m_loadQueue.back();
        }

        // load the Asset using the correct AssetRepository, which will invoke LoadFromFile in Asset
        queueEntry.second.first->load( queueEntry.second.second.get(), queueEntry.first );
    }
}

void AssetManager::add_to_loader_queue( std::string_view name, std::shared_ptr<AssetRepositoryBase> repository, std::shared_ptr<AssetHandle> handle )
{
    {
        std::lock_guard lguard( m_loadQueueMutex );
        handle->m_loadStatus.store( AssetLoadStatus::Queued, std::memory_order::relaxed );
        m_loadQueue.emplace_back( std::pair( std::string( name ), std::pair( repository, handle ) ) );
    }
    m_loadQueueCV.notify_one();
}