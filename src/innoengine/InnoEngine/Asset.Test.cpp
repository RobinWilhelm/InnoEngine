#include "Asset.h"
#include <gtest/gtest.h>

// Demonstrate some basic assertions.
namespace InnoEngine
{
    using T = int;

    class AssetUID_TestFactory
    {
    public:
        static AssetUID<T> makeUID( InternalAssetUID id )
        {
            return AssetUID<T>(id);
        }
    };

    TEST( AssetUIDTest, smallerGreater )
    {
        AssetUID<int> a1, a2;
        a1 = AssetUID_TestFactory::makeUID( 1 );
        a2 = AssetUID_TestFactory::makeUID( 2 );
        EXPECT_EQ( a1 < a2, true );
        EXPECT_EQ( a1 > a2, false );
    }

    TEST( AssetUIDTest, notEqual )
    {
        AssetUID<int> a1, a2;
        a1 = AssetUID_TestFactory::makeUID( 1 );
        a2 = AssetUID_TestFactory::makeUID( 2 );
        EXPECT_EQ( a1 != a2, true );
        EXPECT_EQ( a1 == a2, false );
    }

    TEST( AssetUIDTest, equal )
    {
        AssetUID<int> a1, a2;
        a1 = AssetUID_TestFactory::makeUID( 1 );
        a2 = AssetUID_TestFactory::makeUID( 1 );
        EXPECT_EQ( a1 == a2, true );
        EXPECT_EQ( a1 != a2, false );
    }

    TEST( AssetUIDTest, invalid )
    {
        AssetUID<int> a1, a2;
        EXPECT_EQ( a1.valid(), false );
        EXPECT_EQ( a2.valid(), false );
        EXPECT_EQ( a1 == a2, true );

        a1 = AssetUID_TestFactory::makeUID( 1 );
        a2 = AssetUID_TestFactory::makeUID( 2 );
        EXPECT_EQ( a1 == a2, false );
        EXPECT_EQ( a1.valid(), true );
        EXPECT_EQ( a2.valid(), true );
    }
}


