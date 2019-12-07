#ifndef _XCORE_SERIALIZER_H
#define _XCORE_SERIALIZER_H
#pragma once

namespace xcore::serializer
{
#if 0
    //------------------------------------------------------------------------------
    // Description:
    //     The xserialfile class is design for binary resources files. It is design
    //     to be super fast loading, super memory efficient and very friendly to the user.
    //     The class will save the data in the final format for the machine. The loading 
    //     structure layouts should be identical to the saving structures. That allows the 
    //     class to save and load in place without the need of any loading function from
    //     the user. Note that if we ever move to windows 64bits we will have to solved the 
    //     case where pointers sizes will be different from the consoles. 
    //
    //<P>  One of the requirements is that the user provides a SerializeIO function per structure 
    //     that needs saving. There are certain cases where you can avoid this thought, check the example.
    //     Having the function allows this class to recurse across the hierarchy of the user classes/structures/buffer and arrays.
    //     Ones the class has finish loading there will be only one pointer that it is return which contains a 
    //     the pointer of the main structure. The hold thing will have been allocated as a
    //     single block of memory. This means that the pointer return is the only pointer that
    //     needs to be deleted.
    //
    //<P>  Loading is design to be broken up into 3 stages. The Loading The Header which load minimum information
    //     about resource, load object which loads the file into memory and finally resolve which calls an specific 
    //     constructor which allows the object to deal with any special cases and talking to other systems.
    //     The reason for it is that the constructor will be call. This constructor is a special
    //     one which accepts a xserialfile structure. Here the user may want to register data with 
    //     ingame managers such vram managers, or animation managers, etc. By having that executed in the
    //     main thread relaxes the constrains of thread safety without sacrificing much performance at all.
    //     
    //<P>  There are two kinds of data the can be save/load. Unique and non-unique. When the user selects
    //     pointers to be unique the system will allocate that memory as a separate buffer. Any pointer
    //     which does not specify unique will be group as a single allocation. This allows the system to
    //     be very efficient with memory allocations. Additional to the Unique Flag you can also set the
    //     vram flag. The system recognizes two kinds these two types of ram, but the only things that it does
    //     internally with this flag is to separate non-unique memory into these two groups. When deleting the 
    //     object all memory mark as non-unique doesn't need to be free up but everything mark as unique does
    //     need to be free up by the user. This can happen in the destructor of the main structure. 
    //
    //<P>  There is only 2 types of functions to save your data. Serialize and SerializeEnum. Serialize is use
    //     to safe atomic types as well as arrays and pointers. SerializeEnum is design to work for enumerations.
    //     Note that endian issues will be automatically resolve as long as you use those two functions. If the 
    //     user decides to save a blob of data he will need to deal with endian swamping. Please use the SwapEndian 
    //     function to determine weather the system is swamping endians. When saving the data you don't need to 
    //     worry about saving the data members in order. The class take care of that internally. 
    // 
    //<P>  Dealings with 64 vs 32 bit pointers. For now there will be two sets of crunchers the one set
    //     will compile data with 64 bits this are specifically for the PC. The others will be compiled for
    //     32bits and there for the pointer sizes will remain 32. It is possible however that down the road
    //     we may want to compile crunchers in 64 bits yet output 32 pointers. In this case there are two 
    //     solution and both relay on a macro that looks like this: X_EXPTR( type, name) This macro will have 
    //     to be used when ever the user wants to declare a pointer inside an structure. So in solution
    //     one it that the pointers will remain 64 even in the target machine. The macro will create an 
    //     additional 32bit dummy variable in the structure. In the other solution the macro will contain 
    //     a smart pointer for 64 bits environments. That smart pointer class will have a global array of real
    //     pointers where it will allocate its entries. 
    //
    //<P><B>Physical File layout in disk</B>
    //<CODE>
    //                          +----------------+      <-+
    //                          | File Header    |        | File header is never allocated.
    //                          +----------------+ <-+  <-+
    //                          | BlockSizes +   |   |
    //                          | PointerInfo +  |   |  This is temporary allocated and it gets deleted 
    //                          | PackInfo       |   |  before the LoadObject function returns.
    //                          |                |   |
    //                          +----------------+ <-+  <-+ 
    //                          |                |        | Here are a list of blocks which contain the real
    //                          | Blocks         |        | data that the user saved. Blocks are compress
    //                          |                |        | by the system and decompress at load time.
    //                          |                |        | The system will call a user function to allocate the memory.
    //                          +----------------+      <-+
    //
    //</CODE>
    // Example:
    //<CODE>
    //    struct data1
    //    {
    //        void SerializeIO( xserialfile& SerialFile ) const
    //        {
    //            SerialFile.Serialize( m_A );
    //        }
    //        s16     m_A;
    //    };
    //    
    //    struct data3
    //    {
    //        void SerializeIO( xserialfile& SerialFile ) const
    //        {
    //            SerialFile.Serialize( m_Count );
    //            SerialFile.Serialize( m_pData, m_Count );
    //        }
    //
    //        s32     m_Count;
    //        data1*  m_pData;
    //    };
    //
    //    struct data2 : public data1
    //    {
    //        // data
    //        data3   m_GoInStatic;
    //        data3   m_DontDynamic;
    //    
    //        enum
    //        {
    //            DYNAMIC_COUNT = 100,
    //            STATIC_COUNT  = 100,
    //            STATIC_MAX    = 0xffffffff
    //        };
    //    
    //       // Initialize all the data
    //        data2( void )
    //        {
    //            m_A = 100;
    //    
    //            m_DontDynamic.m_Count = DYNAMIC_COUNT;
    //            m_DontDynamic.m_pData = (data1*)x_malloc( sizeof(data1), m_DontDynamic.m_Count, 0 );
    //            for( s32 i=0; i<m_DontDynamic.m_Count; i++ ) { m_DontDynamic.m_pData[i].m_A = 22+i; }
    //    
    //            m_GoInStatic.m_Count = STATIC_COUNT;
    //            m_GoInStatic.m_pData = (data1*)x_malloc( sizeof(data1), m_GoInStatic.m_Count, 0 );
    //            for( s32 i=0; i<m_GoInStatic.m_Count; i++ ) { m_GoInStatic.m_pData[i].m_A = 23+i; }
    //        }
    //    
    //        // Save the data
    //        void SerializeIO( xserialfile& SerialFile ) const
    //        {
    //            // Make sure that it is the first version
    //            SerialFile.SetResourceVersion( 1 );
    //
    //            // Tell the structure to save it self
    //            SerialFile.Serialize( m_GoInStatic );
    //    
    //            // Don't always need to go into structures
    //            SerialFile.Serialize ( m_DontDynamic.m_Count );
    //            SerialFile.Serialize( m_DontDynamic.m_pData, m_DontDynamic.m_Count, xserialfile::FLAGS_UNIQUE );
    //    
    //            // Tell our parent to save it self
    //            data1::SerializeIO( SerialFile );
    //        }
    //    
    //        // This is the loading constructor by the time is call the file already loaded
    //        data2( xserialfile& SerialFile )
    //        {
    //            x_assert( SerialFile.GetResourceVersion() == 1 );
    //    
    //            // *** Only reason to have a something inside this constructor is to deal with dynamic data
    //            // We move the memory to some other random place
    //            data1*  pData = (data1*)x_malloc( sizeof(data1),m_DontDynamic.m_Count, 0);
    //            x_memcpy( pData, m_DontDynamic.m_pData, m_DontDynamic.m_Count*sizeof(data1) );
    //    
    //            // Now we can overwrite the dynamic pointer without a worry
    //            x_free(m_DontDynamic.m_pData);
    //            m_DontDynamic.m_pData = pData;
    //        }
    //    
    //        ~data2( void )
    //        {
    //            // Here to deal with dynamic stuff 
    //            if(m_DontDynamic.m_pData) x_free( m_DontDynamic.m_pData );
    //        }
    //    };
    //      
    //    void main( void )
    //    {
    //        xstring     FileName( X_STR("SerialFile.bin") );
    //
    //        // Save
    //        {
    //            xserialfile SerialFile;
    //            data2     TheData;
    //            TheData.SanityCheck();
    //            SerialFile.Save( FileName, TheData );
    //        }
    //    
    //        // Load
    //        {
    //            xserialfile  SerialFile;
    //            data2*     pTheData;
    //    
    //            // This hold thing could happen in one thread
    //            SerialFile.Load( FileName, pTheData );
    //    
    //            pTheData->SanityCheck();
    //    
    //            // Okay one pointer to nuke
    //            x_delete( pTheData );
    //        }
    //    }
    //</CODE>
    //------------------------------------------------------------------------------
    
    template< typename T >
    struct alignas(std::uint64_t) data_ptr
    {
        T* m_pValue;
    };

    union mem_type
    {
        std::uint8_t    m_Value{ 0 };
        struct
        {
            bool    m_bUnique:1;        // -> On  - Unique is memory is that allocated by it self and there for could be free
                                        //    Off - Common memory which can't be freed for the duration of the object.
            bool    m_bTempMemory:1;    // -> TODO: this is memory that will be freed after the object constructor returns
                                        //          However you can overwrite this functionality by taking ownership of the temp pointer.
                                        //          The good thing of using this memory type is that multiple allocations are combine into a single one.
                                        //          This flag will override the UNIQUE and VRAM flags, they are exclusive.
                                        //
            bool    m_bVRam:1;          // -> On  - This memory is to be allocated in vram if the hardware has it.
                                        //    Off - Main system memory.
        };

        enum class flags : std::uint8_t
        {
              UNIQUE        = (1<<0)
            , TEMP_MEMORY   = (1<<1)
            , VRAM          = (1<<2)
        };

        template< typename...T_ARGS >
        static constexpr mem_type Flags(T_ARGS... Args) noexcept
        {
            mem_type D;
            if constexpr (sizeof...(T_ARGS) > 0)
            {
                xcore::types::tuple_visit([&](auto&& V) noexcept
                {
                    switch (V)
                    {
                    case flags::UNIQUE:         D.m_bUnique = true; break;
                    case flags::TEMP_MEMORY:    D.m_bTempMemory = true; break;
                    case flags::VRAM:           D.m_bVRam = true; break;
                    default: xassert(false);
                    }

                }, std::tuple{ std::forward<T_ARGS>(Args)... });
            }
            return D;
        }
    };

    using   allocate_memory_fn =  void* (units::bytes Size, mem_type MemType);

    class stream
    {
    public:

                                    stream                      (void)                                                                                      noexcept;
        template< class T >
        inline      xcore::err      Serialize                   (const T& A)                                                                                noexcept;
        template< class T, typename T_SIZE >
        inline      xcore::err      Serialize                   ( T*const & pView, T_SIZE Size, mem_type MemoryFlags)                                             noexcept;

        template< class T >
        inline      err             Save                        ( const string::view<const wchar_t> FileName
                                                                , const T& Object, mem_type ObjectFlags={}, bool bSwapEndian = false)                       noexcept;
        
        template< class T >
        inline      err             Save                        ( file::stream& File, const T& Object, mem_type ObjectFlags, bool bSwapEndian = false)      noexcept;

        template< class T >
        err                         Load                        (file::stream& File, T*& pObject)                                                           noexcept;
        template< class T >
        err                         Load                        (const string::view<wchar_t> FileName, T*& pObject )                                        noexcept;

        err                         LoadHeader                  (file::stream& File, std::size_t SizeOfT)                                                   noexcept;
        void*                       LoadObject                  (file::stream& File)                                                                        noexcept;
        template< class T >
        void                        ResolveObject               (T*& pObject)                                                                               noexcept;

        void                        setResourceVersion          (std::uint16_t ResourceVersion)                                                             noexcept;
        void                        setSwapEndian               (bool SwapEndian)                                                                           noexcept;

        constexpr   bool            SwapEndian                  (void)                                                                              const   noexcept;
        constexpr   std::uint16_t   getResourceVersion          (void)                                                                              const   noexcept;

        void                        setAllocateMemoryCallBack   (allocate_memory_fn Function)                                                               noexcept;

        void                        DontFreeTempData            (void)                                                                                      noexcept { m_bFreeTempData = false; }
        void*                       getTempData                 (void)                                                                              const   noexcept { xassert(m_bFreeTempData == false);  return m_pTempBlockData; }

    protected:

        static constexpr std::uint32_t  version_id_v        = 00010111_xbin;
        static constexpr std::uint32_t  max_block_size_v    = 1024 * 127;

        // This structure wont save to file
        struct decompress_block
        {
            std::array<std::byte, max_block_size_v> m_Buff;
        };

        // This structure will save to file
        struct ref
        {
            std::uint32_t                       m_PointingAT        {}; // What part of the file is this pointer pointing to
            std::uint32_t                       m_OffSet            {}; // Byte offset where the pointer lives
            std::uint32_t                       m_Count             {}; // Count of entries that this pointer is pointing to
            std::uint16_t                       m_OffsetPack        {}; // Offset pack where the pointer is located
            std::uint16_t                       m_PointingATPack    {}; // Pack location where we are pointing to
        };

        // This structure will save to file
        struct pack
        {
            mem_type                            m_PackFlags         {}; // Flags which tells what type of memory this pack is            
            std::uint32_t                       m_UncompressSize    {}; // How big is this pack uncompress
        };

        // This structure wont save to file
        struct pack_writting : public pack
        {
            xcore::file::stream                 m_Data              {}; // raw Data for this block
            std::uint32_t                       m_BlockSize         {}; // size of the block for compressing this pack
            std::uint32_t                       m_CompressSize      {}; // How big is this pack compress
            xcore::unique_span<std::byte>       m_CompressData      {}; // Data in compress form
        };

        // This structure wont save to file
        struct writting
        {
            std::uint32_t                     AllocatePack            (mem_type DefaultPackFlags) noexcept;

            xcore::vector<std::uint32_t>        m_CSizeStream       {}; // a in order List of compress sizes for packs and blocks
            xcore::vector<ref>                  m_PointerTable      {}; // Table of all the pointer written
            xcore::vector<pack_writting>        m_Packs             {}; // Free-able memory + VRam/Core
            xcore::file::stream*                m_pFile             {};
            bool                                m_bEndian           {};
        };

        // This structure will save to file
        struct header
        {
            std::uint32_t                       m_SizeOfData        {}; // Size of this hold data in disk excluding header
            std::uint16_t                       m_SerialFileVersion {}; // Version generated by this system
            std::uint16_t                       m_PackSize          {}; // Pack size
            std::uint16_t                       m_nPointers         {}; // How big is the table with pointers
            std::uint16_t                       m_nPacks            {}; // How many packs does it contain
            std::uint16_t                       m_nBlockSizes       {}; // How many block sizes do we have
            std::uint16_t                       m_ResourceVersion   {}; // User version of this data
            std::uint16_t                       m_MaxQualities      {}; // Maximum number of qualities for this resource
            std::uint16_t                       m_AutomaticVersion  {}; // The size of the main structure as a simple version of the file
        };

    protected:

                    xcore::err      SaveFile            (void)                                                                                              noexcept;
        inline      file::stream&   getW                (void)                                                                                              noexcept;
//                    file::stream&   getTable            (void)                                                                                      const   noexcept;
        constexpr   bool            isLocalVariable     (const std::byte* pRange)                                                                   const   noexcept;
        constexpr   std::int32_t    ComputeLocalOffset  (const std::byte* pItem)                                                                    const   noexcept;
                    xcore::err      HandlePtrDetails    (const std::byte* pA, std::size_t SizeofA, std::size_t Count, mem_type MemoryFlags)                 noexcept;
        inline      xcore::err      Handle              (const std::span<const std::byte> View)                                                             noexcept;

    protected:

        // non stack base variables for writing
        writting*               m_pWrite            {};             // Static data for writing

        // Stack base variables for writing
        std::uint32_t           m_iPack             {};
        std::uint32_t           m_ClassPos          {};
        mutable std::byte*      m_pClass            {};
        std::uint32_t           m_ClassSize         {};

        // Loading data
        header                  m_Header            {};             // Header of the resource
        allocate_memory_fn*     m_pMemoryCallback   {};             // Callback
        void*                   m_pTempBlockData    { nullptr };    // This is data that was saved with the flag temp_data
        bool                    m_bFreeTempData     { true };
    };

#endif
}
#endif