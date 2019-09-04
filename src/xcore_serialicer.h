#ifndef _XCORE_SERIALIZER_H
#define _XCORE_SERIALIZER_H
#pragma once

namespace xcore::serialicer
{
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
class xserialfile
{
public:

    X_DEFBITS( mem_type, u8, 0,
        X_DEFBITS_ARG( bool,  UNIQUE            ,   1  ),  // -> On  - Unique is memory is that allocated by it self and there for could be free
                                                           //    Off - Common memory which can't be freed for the duration of the object.
        X_DEFBITS_ARG( bool,  TEMP_MEMORY       ,   1  ),  // -> TODO: this is memory that will be freed after the object constructor returns
                                                           //          However you can overwrite this functionality by taking ownership of the temp pointer.
                                                           //          The good thing of using this memory type is that multiple allocations are combine into a single one.
                                                           //          This flag will override the UNIQUE and VRAM flags, they are exclusive.
                                                           //
        X_DEFBITS_ARG( bool,  VRAM              ,   1  )   // -> On  - This memory is to be allocated in vram if the hardware has it.
                                                           //    Off - Main system memory.
    );

    using   allocate_memory_fn =  void*( xuptr Size, mem_type MemType );
    using   err                = xfile::err;

public:

                                xserialfile                 ( void )                                                                                            noexcept;
    template< class T > 
    err                         Serialize                   ( const T& A )                                                                                      noexcept;
    template< class T, s32 C > 
    err                         SerializeEnum               ( const xarray<T, C>& A )                                                                           noexcept;
    template< class T > 
    err                         SerializeEnum               ( const T& A )                                                                                      noexcept;
    template< class T > 
    err                         SerializeEnum               ( const T& A,                   s32 Count, mem_type MemoryFlags = mem_type::MASK_ZERO )             noexcept;
    template< class T >  
    err                         Serialize                   ( const T& A,                   s32 Count, mem_type MemoryFlags = mem_type::MASK_ZERO )             noexcept;
    template< class T >  
    err                         Serialize                   ( const xdataptr<T>& A,         s32 Count, mem_type MemoryFlags = mem_type::MASK_ZERO )             noexcept;
    template< class T > 
    err                         Serialize                   ( const xdataptr<const T>& A,   s32 Count, mem_type MemoryFlags = mem_type::MASK_ZERO )             noexcept;
    template< class T, s32 C > 
    err                         Serialize                   ( const xarray<T,C>& A )                                                                            noexcept;

    template< class T > 
    err                         Save                        ( const xwstring& FileName, const T& Object, mem_type ObjectFlags = mem_type::MASK_ZERO, bool bSwapEndian = false )   noexcept;
    template< class T > 
    err                         Save                        ( xfile& File, const T& Object, mem_type ObjectFlags, bool bSwapEndian = false )                    noexcept;

    template< class T > 
    err                         Load                        ( xfile& File, T*& pObject )                                                                        noexcept;
    template< class T > 
    err                         Load                        ( const xwstring& FileName, T*& pObject )                                                           noexcept;

    err                         LoadHeader                  ( xfile& File, s32 SizeOfT )                                                                        noexcept;
    void*                       LoadObject                  ( xfile& File )                                                                                     noexcept;    
    template< class T > 
    void                        ResolveObject               ( T*& pObject )                                                                                     noexcept;

    void                        setResourceVersion          ( u16 ResourceVersion )                                                                             noexcept;
    void                        setSwapEndian               ( bool SwapEndian )                                                                                 noexcept;

    bool                        SwapEndian                  ( void )                                                                                    const   noexcept;
    u16                         getResourceVersion          ( void )                                                                                    const   noexcept;

    void                        setAllocateMemoryCallBack   ( allocate_memory_fn Function )                                                                     noexcept;

    void                        DontFreeTempData            ( void )                                                                                            noexcept { m_bFreeTempData = false; }
    void*                       getTempData                 ( void )                                                                                    const   noexcept { x_assert( m_bFreeTempData == false );  return m_pTempBlockData; }

protected:

    enum version : u32
    {
        VERSION_ID          = 00010111_bin,
        MAX_BLOCK_SIZE      = 1024*127
    };
    
    // This structure wont save to file
    struct decompress_block
    {
        xarray<xbyte,xserialfile::MAX_BLOCK_SIZE> m_Buff;
    };

    // This structure will save to file
    struct ref
    {
        u32                     m_PointingAT            {}; // What part of the file is this pointer pointing to
        u32                     m_OffSet                {}; // Byte offset where the pointer lives
        u32                     m_Count                 {}; // Count of entries that this pointer is pointing to
        u16                     m_OffsetPack            {}; // Offset pack where the pointer is located
        u16                     m_PointingATPack        {}; // Pack location where we are pointing to
    };

    // This structure will save to file
    struct pack
    {
        mem_type                m_PackFlags             {}; // Flags which tells what type of memory this pack is            
        u32                     m_UncompressSize        {}; // How big is this pack uncompress
    };

    // This structure wont save to file
    struct pack_writting : public pack
    {
        xfile                   m_Data                  {}; // raw Data for this block
        s32                     m_BlockSize             {}; // size of the block for compressing this pack
        s32                     m_CompressSize          {}; // How big is this pack compress
        xafptr<xbyte>           m_CompressData          {}; // Data in compress form
    };

    // This structure wont save to file
    struct writting
    {
        s32                     AllocatePack            ( mem_type DefaultPackFlags ) noexcept;

        xvector<s32>            m_CSizeStream           {}; // a in order List of compress sizes for packs and blocks
        xvector<ref>            m_PointerTable          {}; // Table of all the pointer written
        xvector<pack_writting>  m_Packs                 {}; // Freeable memory + VRam/Core
        bool                    m_bEndian               {};
        xfile*                  m_pFile                 {};
    };

    // This structure will save to file
    struct header
    {
        u32                     m_SizeOfData            {}; // Size of this hold data in disk excluding header
        u16                     m_SerialFileVersion     {}; // Version generated by this system
        u16                     m_PackSize              {}; // Pack size
        u16                     m_nPointers             {}; // How big is the table with pointers
        u16                     m_nPacks                {}; // How many packs does it contain
        u16                     m_nBlockSizes           {}; // How many block sizes do we have
        u16                     m_ResourceVersion       {}; // User version of this data
        u16                     m_MaxQualities          {}; // Maximum number of qualities for this resource
        u16                     m_AutomaticVersion      {}; // The size of the main structure as a simple version of the file
    };

protected:

    template< class T > 
    err                         Handle              ( const T& A )                                                                                              noexcept;
    template< class T > 
    void                        Array               ( const T& A, s32 Count )                                                                                   noexcept;
    template< class T > 
    void                        HandlePtr           ( const T& A, s32 Count, mem_type MemoryFlags )                                                             noexcept;

    xfile::err                  SaveFile            ( void )                                                                                                    noexcept;
    xfile&                      getW                ( void )                                                                                            const   noexcept;
    xfile&                      getTable            ( void )                                                                                            const   noexcept;
    bool                        isLocalVariable     ( xbyte* pRange )                                                                                           noexcept;
    s32                         ComputeLocalOffset  ( xbyte* pItem )                                                                                            noexcept;
    err                         HandlePtrDetails    ( xbyte* pA, s32 SizeofA, s32 Count, mem_type MemoryFlags )                                                 noexcept;

    err                         Handle              ( const s8&          A )                                                                                    noexcept;
    err                         Handle              ( const s16&         A )                                                                                    noexcept;
    err                         Handle              ( const s32&         A )                                                                                    noexcept;
    err                         Handle              ( const s64&         A )                                                                                    noexcept;
                                                                         
    err                         Handle              ( const u8&          A )                                                                                    noexcept;
    err                         Handle              ( const u16&         A )                                                                                    noexcept;
    err                         Handle              ( const u32&         A )                                                                                    noexcept;
    err                         Handle              ( const u64&         A )                                                                                    noexcept;
                                                                         
    err                         Handle              ( const f32&         A )                                                                                    noexcept;
    err                         Handle              ( const f64&         A )                                                                                    noexcept;

    err                         Handle              ( const xmatrix4&    A )                                                                                    noexcept;
    err                         Handle              ( const xvector3&    A )                                                                                    noexcept;
    err                         Handle              ( const xvector3d&   A )                                                                                    noexcept;
    err                         Handle              ( const xbbox&       A )                                                                                    noexcept;
    err                         Handle              ( const xcolor&      A )                                                                                    noexcept;
    err                         Handle              ( const xvector2&    A )                                                                                    noexcept;
    err                         Handle              ( const xvector4&    A )                                                                                    noexcept;
    err                         Handle              ( const xquaternion& A )                                                                                    noexcept;
    template< typename T >
    err                         Handle              ( const xguid<T>&    A )                                                                                    noexcept;

protected:

    // non stack base variables for writing
    writting*               m_pWrite            {};             // Static data for writing

    // Stack base variables for writing
    s32                     m_iPack             {};
    u32                     m_ClassPos          {};
    u8*                     m_pClass            {};
    s32                     m_ClassSize         {};

    // Loading data
    header                  m_Header            {};             // Header of the resource
    allocate_memory_fn*     m_pMemoryCallback   {};             // Callback
    void*                   m_pTempBlockData    { nullptr };    // This is data that was saved with the flag temp_data
    bool                    m_bFreeTempData     { true };
};


}
#endif