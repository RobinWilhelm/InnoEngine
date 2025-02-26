#pragma once
#include "Asset.h"

#include <filesystem>
#include <memory>

enum class AssetLoadStatus : uint16_t
{
    Empty = 0,
    NotFound,
    Queued,
    Loading,
    LoadingFailed,
    Finished,
    Ready,
};

class AssetHandle
{
    friend class AssetManager;

public:
    AssetHandle() = default;

    AssetHandle( const AssetHandle& other )
    {
        m_loadStatus.store( other.m_loadStatus.load( std::memory_order_relaxed ), std::memory_order_relaxed );
        m_assetUID = other.m_assetUID;
    }

    AssetHandle& operator=( const AssetHandle& other )
    {
        if ( this == &other )
            return *this;

        m_loadStatus.store( other.m_loadStatus.load( std::memory_order_relaxed ), std::memory_order_relaxed );
        m_assetUID = other.m_assetUID;
        return *this;
    }

    ~AssetHandle() = default;

    bool loaded()
    {
        switch ( m_loadStatus.load( std::memory_order_relaxed ) ) {
        case AssetLoadStatus::Ready: return true;
        case AssetLoadStatus::Finished:
        {    // force a happend-before synchronization with the loader thread
            std::atomic_thread_fence( std::memory_order_acquire );
            m_loadStatus.store( AssetLoadStatus::Ready, std::memory_order_relaxed );
            return true;
        }
        default: return false;
        }
        return false;
    }

protected:
    InternalAssetUID             m_assetUID   = 0;
    std::atomic<AssetLoadStatus> m_loadStatus = AssetLoadStatus::Empty;
};

template <typename T>
class AssetView : public AssetHandle
{
    friend class AssetRepository<T>;

public:
    std::shared_ptr<T> get()
    {
        return ms_repository->get_asset( getUID() );
    }

    AssetUID<T> getUID() const
    {
        return AssetUID<T>( m_assetUID );
    }

private:
    static inline AssetRepository<T>* ms_repository = nullptr;
};
