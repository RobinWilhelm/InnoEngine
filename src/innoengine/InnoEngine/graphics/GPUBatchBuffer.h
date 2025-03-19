#pragma once
#include "SDL3/SDL_gpu.h"

#include "InnoEngine/BaseTypes.h"
#include "InnoEngine/graphics/GPUDeviceRef.h"

namespace InnoEngine
{
    template <typename BufferLayout, typename BatchCustomData>
    class GPUBatchStorageBuffer
    {
        GPUBatchStorageBuffer( GPUDeviceRef device, uint32_t batch_size );

    public:
        struct BatchData
        {
            SDL_GPUBuffer*  GPUBuffer  = nullptr;
            uint32_t        Count      = 0;
            BatchCustomData CustomData = {};
        };

        using BatchDataList = std::vector<BatchData>;

        ~GPUBatchStorageBuffer();

        static auto create( GPUDeviceRef device, uint32_t batch_size ) -> Ref<GPUBatchStorageBuffer>;

        bool             current_batch_full();
        BatchCustomData* upload_and_add_batch( SDL_GPUCopyPass* copy_pass );
        BufferLayout*    next_data();

        void upload_last( SDL_GPUCopyPass* copy_pass );

        size_t size() const;
        void   clear();

        const BatchDataList& get_batchlist() const;

        size_t get_current_batch_remaining_size() const;

    private:
        int32_t       find_free_gpubuffer_index();
        BufferLayout* map_transferbuffer();
        void          unmap_and_upload_transferbuffer( SDL_GPUCopyPass* copy_pass );

    private:
        GPUDeviceRef                m_Device         = nullptr;
        SDL_GPUTransferBuffer*      m_TransferBuffer = nullptr;
        std::vector<SDL_GPUBuffer*> m_GPUBuffer;
        std::vector<BatchData>      m_Batches;

        uint32_t m_BatchSize          = 0;
        int32_t  m_CurrentBatchIndex  = -1;
        int32_t  m_CurrentBufferIndex = -1;

        uint32_t      m_CurrentDataCount     = 0;
        BufferLayout* m_CurrentBufferPointer = nullptr;
    };

    template <typename BufferLayout, typename BatchCustomData>
    inline GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::GPUBatchStorageBuffer( GPUDeviceRef device, uint32_t batch_size ) :
        m_Device( device ), m_BatchSize( batch_size )
    {
    }

    template <typename BufferLayout, typename BatchCustomData>
    inline GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::~GPUBatchStorageBuffer()
    {
        if ( m_Device ) {
            if ( m_TransferBuffer ) {
                SDL_ReleaseGPUTransferBuffer( m_Device, m_TransferBuffer );
                m_TransferBuffer = nullptr;
            }

            for ( auto gpubuffer : m_GPUBuffer ) {
                SDL_ReleaseGPUBuffer( m_Device, gpubuffer );
            }
            m_GPUBuffer.clear();
            m_Batches.clear();
            m_Device = nullptr;
        }
    }

    template <typename BufferLayout, typename BatchCustomData>
    inline auto GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::create( GPUDeviceRef device, uint32_t batch_size ) -> Ref<GPUBatchStorageBuffer>
    {
        Ref<GPUBatchStorageBuffer<BufferLayout, BatchCustomData>> buffer =
            Ref<GPUBatchStorageBuffer<BufferLayout, BatchCustomData>>( new GPUBatchStorageBuffer<BufferLayout, BatchCustomData>( device, batch_size ) );

        SDL_GPUTransferBufferCreateInfo tbufferCreateInfo = {};
        tbufferCreateInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
        tbufferCreateInfo.size                            = batch_size * sizeof( BufferLayout );

        buffer->m_TransferBuffer = SDL_CreateGPUTransferBuffer( device, &tbufferCreateInfo );
        if ( buffer->m_TransferBuffer == nullptr ) {
            IE_LOG_ERROR( "Failed to create GPUTransferBuffer! {}", SDL_GetError() );
            return nullptr;
        }

        return buffer;
    }

    template <typename BufferLayout, typename BatchCustomData>
    inline bool GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::current_batch_full()
    {
        return m_CurrentBatchIndex == -1 || m_CurrentDataCount >= m_BatchSize;
    }

    template <typename BufferLayout, typename BatchCustomData>
    inline BatchCustomData* GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::upload_and_add_batch( SDL_GPUCopyPass* copy_pass )
    {
        // Unmap and upload previous batch (if there is one) data before changing to new batch
        if ( m_CurrentBatchIndex != -1 ) {
            unmap_and_upload_transferbuffer( copy_pass );
        }

        m_CurrentBatchIndex  = static_cast<int32_t>( m_Batches.size() );
        m_CurrentBufferIndex = find_free_gpubuffer_index();

        BatchData& newbatch = m_Batches.emplace_back();
        newbatch.GPUBuffer  = m_GPUBuffer[ m_CurrentBufferIndex ];
        newbatch.Count      = 0;

        m_CurrentBufferPointer = map_transferbuffer();
        return &newbatch.CustomData;
    }

    template <typename BufferLayout, typename BatchCustomData>
    inline BufferLayout* GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::next_data()
    {
        IE_ASSERT( m_CurrentBufferPointer != nullptr && m_CurrentDataCount < m_BatchSize );
        return &m_CurrentBufferPointer[ m_CurrentDataCount++ ];
    }

    template <typename BufferLayout, typename BatchCustomData>
    inline void GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::upload_last( SDL_GPUCopyPass* copy_pass )
    {
        // unmap and upload last batch data
        if ( m_CurrentBatchIndex != -1 && m_CurrentDataCount > 0 ) {
            unmap_and_upload_transferbuffer( copy_pass );
        }
    }

    template <typename BufferLayout, typename BatchCustomData>
    inline size_t GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::size() const
    {
        return m_Batches.size();
    }

    template <typename BufferLayout, typename BatchCustomData>
    inline void GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::clear()
    {
        m_Batches.clear();
        m_CurrentBatchIndex  = -1;
        m_CurrentDataCount   = 0;
        m_CurrentBufferIndex = -1;
    }

    template <typename BufferLayout, typename BatchCustomData>
    inline const GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::BatchDataList& GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::get_batchlist() const
    {
        return m_Batches;
    }

    template<typename BufferLayout, typename BatchCustomData>
    inline size_t GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::get_current_batch_remaining_size() const
    {
        return m_BatchSize - m_CurrentDataCount;
    }

    template <typename BufferLayout, typename BatchCustomData>
    inline int32_t GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::find_free_gpubuffer_index()
    {
        if ( m_CurrentBufferIndex + 1 < m_GPUBuffer.size() ) {
            // just return the next one in case it exits
            return ++m_CurrentBufferIndex;
        }
        else {
            // doesnt exist -> create new and return that
            SDL_GPUBuffer*& buffer = m_GPUBuffer.emplace_back();

            SDL_GPUBufferCreateInfo createInfo = {};
            createInfo.usage                   = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ;
            createInfo.size                    = m_BatchSize * sizeof( BufferLayout );

            buffer = SDL_CreateGPUBuffer( m_Device, &createInfo );
            if ( buffer == nullptr ) {
                // TODO: this needs to be handled better
                IE_LOG_ERROR( "SDL_CreateGPUBuffer failed : {0}", SDL_GetError() );
                return -1;
            }
            return ++m_CurrentBufferIndex;
        }
    }

    template <typename BufferLayout, typename BatchCustomData>
    inline BufferLayout* GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::map_transferbuffer()
    {
        return static_cast<BufferLayout*>( SDL_MapGPUTransferBuffer( m_Device, m_TransferBuffer, true ) );
    }

    template <typename BufferLayout, typename BatchCustomData>
    inline void GPUBatchStorageBuffer<BufferLayout, BatchCustomData>::unmap_and_upload_transferbuffer( SDL_GPUCopyPass* copy_pass )
    {
        SDL_UnmapGPUTransferBuffer( m_Device, m_TransferBuffer );
        SDL_GPUTransferBufferLocation tranferBufferLocation { .transfer_buffer = m_TransferBuffer, .offset = 0 };
        SDL_GPUBufferRegion           bufferRegion { .buffer = m_GPUBuffer[ m_CurrentBufferIndex ],
                                                     .offset = 0,
                                                     .size   = static_cast<uint32_t>( m_CurrentDataCount * sizeof( BufferLayout ) ) };
        SDL_UploadToGPUBuffer( copy_pass, &tranferBufferLocation, &bufferRegion, true );

        m_Batches[ m_CurrentBatchIndex ].Count = m_CurrentDataCount;
        m_CurrentBufferPointer                 = nullptr;
        m_CurrentBatchIndex                    = -1;
        m_CurrentDataCount                     = 0;
    }
}    // namespace InnoEngine
