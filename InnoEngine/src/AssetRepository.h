#pragma once
#include "SDL3/SDL_log.h"

#include "Asset.h"
#include "AssetView.h"

#include <filesystem>
#include <map>
#include <string>
#include <optional>
#include <type_traits>
#include <unordered_map>

class AssetRepositoryBase
{
    friend class AssetManager;

protected:
    virtual ~AssetRepositoryBase() = default;
    AssetRepositoryBase( std::filesystem::path directory ) :
        m_directory( directory ) { };
    virtual bool load( AssetHandle* handle, std::string_view name ) = 0;

protected:
    std::filesystem::path m_directory;
};

template <typename T>
class AssetRepository : public AssetRepositoryBase
{
    static_assert( std::is_base_of<AssetBase, T>::value == true );
    friend class AssetView<T>;
    friend class AssetManager;

public:
    AssetRepository( std::filesystem::path directory ) :
        AssetRepositoryBase( directory )
    {
        AssetView<T>::ms_repository = this;

        // create first entry as a dummy entry so we dont have to -1 on all AssetUIDs
        // because the the first valid AssetUID is 1
        m_loaded.emplace_back( nullptr );
    }

    virtual ~AssetRepository() { }

    bool is_available( std::string_view name )
    {
        return m_lookupByName.find( name ) != m_lookupByName.end();
    }

    bool is_available( AssetUID<T> uid )
    {
        return ( static_cast<InternalAssetUID>( uid ) != 0 && m_loaded.size() > static_cast<InternalAssetUID>( uid ) );
    }

    auto require_asset( std::string_view name ) -> std::optional<AssetView<T>>
    {
        auto nameIt = m_lookupByName.find( name );
        if ( nameIt != m_lookupByName.end() ) {
            return *nameIt->second;
        } else {
            std::optional<std::shared_ptr<AssetView<T>>> optAsset = create_and_load_asset( name );
            if ( optAsset.has_value() == false )
                return std::nullopt;

            return *( *optAsset ).get();
        }
    }

    std::shared_ptr<T> get_asset( const AssetView<T>&& handle )
    {
        IE_ASSERT( is_available( handle.m_assetUID ) );
        return m_loaded[handle.m_assetUID];
    }

    std::shared_ptr<T> get_asset( AssetUID<T> uid )
    {
        IE_ASSERT( is_available( uid ) );
        return m_loaded[static_cast<InternalAssetUID>( uid )];
    }

private:
    auto create_empty( std::string_view name ) -> std::optional<std::shared_ptr<AssetView<T>>>
    {
        if ( is_available( name ) || static_cast<InternalAssetUID>( m_nextFree ) == ( std::numeric_limits<InternalAssetUID>::max )() )
            return std::nullopt;

        std::filesystem::path fullPath = m_directory / name;
        if ( std::filesystem::exists( fullPath ) && std::filesystem::is_regular_file( fullPath ) ) {
            std::shared_ptr<AssetView<T>> view = std::make_shared<AssetView<T>>();
            view->m_loadStatus.store( AssetLoadStatus::Empty, std::memory_order_relaxed );
            view->m_assetUID = m_nextFree;
            if ( static_cast<InternalAssetUID>( m_nextFree ) < ( std::numeric_limits<InternalAssetUID>::max )() ) {
                m_nextFree++;
            }

            m_lookupByName.insert( { std::string( name ), view } );
            m_loaded.emplace_back( std::make_shared<T>() );
            m_loaded[view->m_assetUID]->m_assetUID = view->m_assetUID;
            return view;
        } else {
            IE_LOG_ERROR( "Asset not found! %s", fullPath.string().c_str() );
            return std::nullopt;
        }
    }

    bool load( AssetHandle* handle, std::string_view name ) override
    {
        AssetView<T>*              view   = static_cast<AssetView<T>*>( handle );
        std::shared_ptr<AssetBase> pAsset = get_asset( view->getUID() );
        if ( pAsset && pAsset->load_from_file( m_directory / name ) ) {
            std::atomic_thread_fence( std::memory_order_release );
            view->m_loadStatus.store( AssetLoadStatus::Finished, std::memory_order_relaxed );
            return true;
        }
        return false;
    }

    auto create_and_load_asset( std::string_view name ) -> std::optional<std::shared_ptr<AssetView<T>>>
    {
        std::optional<std::shared_ptr<AssetView<T>>> opt = create_empty( name );
        if ( opt.has_value() == false )
            return std::nullopt;

        std::shared_ptr<AssetView<T>> assetView = opt.value();
        return load( assetView.get(), name ) ? opt : std::nullopt;
    }

private:
    AssetUID<T>                                                       m_nextFree = 1;
    std::map<std::string, std::shared_ptr<AssetView<T>>, std::less<>> m_lookupByName;
    std::vector<std::shared_ptr<T>>                                   m_loaded;
};
