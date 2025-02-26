#pragma once
#include <filesystem>
#include <memory>

using InternalAssetUID = uint16_t;

template <typename T>
class AssetView;

template <typename T>
class AssetRepository;

template <typename T>
class AssetUID
{
    friend class AssetView<T>;
    friend class AssetRepository<T>;
    friend class AssetManager;

    // can only be constructed with a valid UID by AssetView<T>
    AssetUID( InternalAssetUID internalUID ) :
        m_uid( internalUID )
    { }

public:
    AssetUID() :
        m_uid( 0 )
    { }

    AssetUID( const AssetUID& other )
    {
        m_uid = other.m_uid;
    }

    AssetUID& operator=( const AssetUID& other )
    {
        if ( this == &other )
            return *this;

        m_uid = other.m_uid;
        return *this;
    }

    inline friend bool operator==( const AssetUID& lhs, const AssetUID& rhs )
    {
        return lhs.m_uid == rhs.m_uid;
    }

    inline friend bool operator!=( const AssetUID& lhs, const AssetUID& rhs )
    {
        return !( lhs == rhs );
    }

    inline friend bool operator<( const AssetUID& lhs, const AssetUID& rhs )
    {
        return lhs.m_uid < rhs.m_uid;
    }

    inline friend bool operator>( const AssetUID& lhs, const AssetUID& rhs )
    {
        return rhs < lhs;
    }

    inline friend bool operator<=( const AssetUID& lhs, const AssetUID& rhs )
    {
        return !( lhs > rhs );
    }

    inline friend bool operator>=( const AssetUID& lhs, const AssetUID& rhs )
    {
        return !( lhs < rhs );
    }

    bool valid() const
    {
        return m_uid != 0;
    }

    void invalidate()
    {
        m_uid = 0;
    }

private:
    operator InternalAssetUID() const
    {
        return m_uid;
    }

    AssetUID operator++( int )
    {
        // returned value should be a copy of the object before increment
        AssetUID temp = *this;
        ++m_uid;
        return temp;
    }

private:
    InternalAssetUID m_uid = 0;
};

class AssetBase
{
public:
    virtual ~AssetBase()                                          = default;
    virtual bool load_from_file( std::filesystem::path fullPath ) = 0;
};

template <typename T>
class Asset : public AssetBase
{
    friend class AssetRepository<T>;

public:
    virtual ~Asset() = default;

    AssetUID<T> get_uid() const
    {
        return m_assetUID;
    }

private:
    AssetUID<T> m_assetUID;
};
