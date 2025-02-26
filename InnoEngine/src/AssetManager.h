#pragma once

#include "Log.h"

#include "Asset.h"
#include "AssetRepository.h"

#include <filesystem>
#include <map>
#include <string>
#include <thread>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <type_traits>
#include <typeindex>

class AssetHandle;

class AssetManager
{
    AssetManager() = default;

public:
    virtual ~AssetManager();

    [[nodiscard]]
    static auto create( std::filesystem::path assetDir, bool multithreaded = true ) -> std::optional<std::unique_ptr<AssetManager>>;

    template <typename T>
    bool add_repository( std::filesystem::path subDirectory )
    {
        static_assert( std::is_base_of<AssetBase, T>::value == true );
        std::filesystem::path fullDirPath = m_assetDirectoryPath / subDirectory.native();
        if ( std::filesystem::exists( fullDirPath ) == false || std::filesystem::is_directory( fullDirPath ) == false ) {
            IE_LOG_ERROR( "Asset directory not found! %s", fullDirPath.string().c_str() );
            return false;
        }

        std::type_index tyidx = std::type_index( typeid( T ) );
        if ( m_assetRepos.find( tyidx ) != m_assetRepos.end() ) {
            IE_LOG_ERROR( "Asset repository already added! %s", fullDirPath.string().c_str() );
            return true;
        }
        auto assetRepo = std::make_shared<AssetRepository<T>>( fullDirPath );
        m_assetRepos.insert( std::pair( tyidx, std::static_pointer_cast<AssetRepositoryBase>( assetRepo ) ) );
        IE_LOG_INFO( "Added asset repo: \"%s\"", subDirectory.string().c_str() );
        return true;
    }

    template <typename T>
    auto require_asset( std::string_view name, bool immediatly = false ) -> std::optional<AssetView<T>>
    {
        static_assert( std::is_base_of<AssetBase, T>::value == true );

        auto repoIt = m_assetRepos.find( typeid( T ) );
        if ( repoIt == m_assetRepos.end() ) {
            return std::nullopt;
        }

        auto                repoBase = repoIt->second.get();
        AssetRepository<T>* pRepo    = static_cast<AssetRepository<T>*>( repoBase );

        if ( pRepo->is_available( name ) || immediatly || m_multithreaded == false ) {
            return pRepo->require_asset( name );
        } else {
            std::optional<std::shared_ptr<AssetView<T>>> assetViewOpt = pRepo->create_empty( name );
            if ( assetViewOpt.has_value() == false ) {
                return std::nullopt;
            } else {
                std::shared_ptr<AssetHandle> handle = std::static_pointer_cast<AssetHandle>( assetViewOpt.value() );
                add_to_loader_queue( name, repoIt->second, handle );
                return *( assetViewOpt.value() ).get();
            }
        }
    }

    template <typename T>
    auto get_asset_by_uid( AssetUID<T> uid ) -> std::optional<T*>
    {
        static_assert( std::is_base_of<AssetBase, T>::value == true );

        std::shared_ptr<AssetRepository<T>> pRepository = get_repository<T>();
        if ( pRepository ) {
            pRepository->get_asset( uid );
        }
        return std::nullopt;
    }

    template <typename T>
    auto get_repository() -> std::shared_ptr<AssetRepository<T>>
    {
        static_assert( std::is_base_of<AssetBase, T>::value == true );

        std::type_index tyidx  = std::type_index( typeid( T ) );
        auto            repoIt = m_assetRepos.find( tyidx );
        if ( repoIt == m_assetRepos.end() ) {
            return nullptr;
        }
        return std::static_pointer_cast<AssetRepository<T>>( repoIt->second );
    }

private:
    void start_async_loader();
    void stop_async_loader();
    void loader_loop();

    void add_to_loader_queue( std::string_view name, std::shared_ptr<AssetRepositoryBase> repository, std::shared_ptr<AssetHandle> handle );

private:
    bool                    m_multithreaded = false;
    std::atomic_bool        m_run           = true;
    std::mutex              m_loadQueueMutex;
    std::condition_variable m_loadQueueCV;

    std::thread                                                                                                        m_loaderThread;
    std::filesystem::path                                                                                              m_assetDirectoryPath;
    std::map<std::type_index, std::shared_ptr<AssetRepositoryBase>>                                                    m_assetRepos;
    std::vector<std::pair<std::string, std::pair<std::shared_ptr<AssetRepositoryBase>, std::shared_ptr<AssetHandle>>>> m_loadQueue;
};
