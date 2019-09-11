

namespace xcore::serializer
{
    namespace details
    {
        template<typename, typename T>
        struct has_serialize 
        {
            static_assert( std::integral_constant_v<T, false>, "Second template parameter needs to be of function type." );
        };

        // specialization that does the checking

        template<typename C, typename Ret, typename... Args>
        struct has_serialize<C, Ret(Args...)> 
        {
        private:
            template<typename T>
            static constexpr auto check(T*) -> typename 
                std::is_same< decltype(std::declval<T>().SerializeIO( file::stream& )),
                             Ret    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                            >::type;  // attempt to call it and see if the return type is correct
            template<typename>
            static constexpr std::false_type check(...);

            using type = decltype(check<C>(0));

        public:
            static constexpr bool value = type::value;
        };
    }
    //------------------------------------------------------------------------------

    constexpr
    file::stream& stream::getW(void) const noexcept
    {
        xassert(m_pWrite);
        return m_pWrite->m_Packs[m_iPack].m_Data;
    }

    //------------------------------------------------------------------------------

    constexpr
    bool stream::isLocalVariable( std::byte* pRange ) const noexcept
    {
        return (pRange >= m_pClass) && (pRange < (m_pClass + m_ClassSize));
    }

    //------------------------------------------------------------------------------

    constexpr
    std::int32_t stream::ComputeLocalOffset( std::byte* pItem ) const noexcept
    {
        xassert(isLocalVariable(pItem));
        return static_cast<std::int32_t>(pItem - m_pClass);
    }

    //------------------------------------------------------------------------------

    template< class T > inline
    xcore::err stream::Save( const string::view<const wchar_t> FileName, const T& Object, mem_type ObjectFlags, bool bEndianSwap ) noexcept
    {
        // Open the file to make sure we are okay
        err     Error;
        file    File;

        // 
        if( File.Open(FileName, "wb").isError(Error) ) return Error;

        // Call the actual save
        if( Save(File, Object, ObjectFlags, bEndianSwap).isError(Error) ) return Error;

        // done with the file
        File.Close();

        return Error;
    }

    //------------------------------------------------------------------------------

    template< class T > inline
    xcore::err stream::Save( xcore::file& File, const T& Object, mem_type ObjectFlags, bool bSwapEndian ) noexcept
    {
        xcore::err Error;

        //
        // Allocate the writing structure
        //
        std::unique_ptr<writting> Write{ new writting };

        // Assign it so it is accessible for other functions
        // but we keep the owner
        m_pWrite = Write;

        // back up the pointer
        Write->m_pFile = &File;

        //
        // Initialize class members
        //
        m_iPack             = Write->AllocatePack( ObjectFlags );
        m_ClassPos          = 0;
        m_pClass            = const_cast<std::byte*>(reinterpret_cast<const std::byte*>(&Object));
        m_ClassSize         = sizeof(Object);
        Write->m_bEndian    = bSwapEndian;

        // Save the initial class
        getW().putC( ' ', m_ClassSize, true );

        // Start the saving 
        Object.SerializeIO( *this );

        // Save the file
        if (SaveFile().isError(Error)) return Error;

        // clean up
        m_pWrite = nullptr;

        return Error;
    }

    //------------------------------------------------------------------------------

    template< class T > inline
    xcore::err stream::Serialize( const T& A ) noexcept
    {
        xcore::err Error;

        // Reading or writing?
        xassert(m_pWrite);

        if constexpr ( std::is_integral_v<T>
                    || std::is_floating_point_v<T>
                    || std::is_enum_v<T> )
        {
            if(Handle({&const_cast<std::byte&>(reinterpret_cast<const std::byte&>(A)), sizeof(T)}).isError(Error)) return Error;
        }
        else if constexpr (std::is_array_v<T>)
        {
            // Handle C arrays
            for (int i = 0; i < std::extent_v<T>; ++i) if (Serialize(A[i]).isError(Error)) return Error;
        }
        else if constexpr (xcore::is_array_v<T> || xcore::is_span_v<T> )
        {
            static_assert(std::is_object_v<T>);
            for (auto& X : A) if (Serialize(X).isError(Error)) return Error;
        }
        else if constexpr (details::has_serialize<T, xcore::err(stream&)>::value)
        {
            static_assert(std::is_object_v<T>);
            static_assert(false == std::is_polymorphic_v<T>);

            // Copy most of the data
            stream File(*this);

            File.m_iPack = m_iPack;
            File.m_ClassPos = static_cast<std::uint32_t>(getW().Tell());
            File.m_pClass = &const_cast<std::byte&>(reinterpret_cast<const std::byte&>(A));
            File.m_ClassSize = sizeof(A);
            A.SerializeIO(File);

            // Something very strange happen. We allocated more memory space than we expected
            //xassert( getW().Tell() <= (File.m_ClassPos + sizeof( A )) );

            // Go the end of the structure 
            return getW().SeekOrigin(File.m_ClassPos + sizeof(A));
        }
        else if constexpr (std::is_trivially_copyable_v<T>)
        {
            /*
            || std::is_same<T, matrix4>
            || std::is_same<T, vector3>
            || std::is_same<T, vector3d>
            || std::is_same<T, bbox>
            || std::is_same<T, vector2>
            || std::is_same<T, vector4>
            || std::is_same<T, quaternion>
            */
            if (Handle({ &const_cast<std::byte&>(reinterpret_cast<const std::byte&>(A)), sizeof(T) }).isError(Error)) return Error;
        }
        else
        {
            static_assert(xcore::types::always_false_v<T>);
        }

        return Error;
    }

    //------------------------------------------------------------------------------

    template< class T > inline
    xcore::err stream::Serialize(const std::span<const T> View, mem_type MemoryFlags) noexcept
    {
        xcore       Error;
        const auto  BackupPackIndex = m_iPack;
        auto        pA = reinterpret_cast<const std::byte*>(View.data());

        // Handle pointer details
        HandlePtrDetails(pA, sizeof(T), View.size(), MemoryFlags);

        //
        // Loop throw all the items
        //

        // Now loop
        for (std::size_t i = 0; i < View.size(); i++)
        {
            if (Serialize(pA[i]).isError(Error)) return Error;
            xassert(getW().Tell() >= i * sizeof(pA[0]));
        }

        //
        // Restore the old pack
        //
        m_iPack = BackupPackIndex;
        return Error;
    }


    //------------------------------------------------------------------------------

    template< class T > inline
    xcore::err stream::Handle( const std::span<const T> View ) noexcept
    {
        xcore   Error;
        auto    pA = reinterpret_cast<const std::byte*>(View.data());////(u8*)((void*)A);

        xassert(View.size() >= 0);
        
        // If it is not a local variable then you must pass the memory type
        xassert(isLocalVariable(pA));

        // Make sure that we are at the right offset
        getW().SeekOrigin(m_ClassPos + ComputeLocalOffset(pA));

        // Is an array of structures of items so we want to preallocate the space
        // Since it is an array we don't need to put the "* Count"
        getW().putC(' ', sizeof(A), false);

        // Loop throw all the items
        for (std::size_t i = 0; i < View.size(); i++)
        {
            if (Handle(pA[i]).isError(Error)) return Error;
        }
        return Error;
    }

    //------------------------------------------------------------------------------

    template< class T >
    void xserialfile::ResolveObject(T*& pObject) noexcept
    {
        // Initialize all
        (void)x_construct(T, pObject, { *this });

        // deal with temp data
        if (m_bFreeTempData && m_pTempBlockData)
            x_delete((xbyte*)m_pTempBlockData);
    }


    //------------------------------------------------------------------------------

    template< class T > inline
        xfile::err xserialfile::Load(xfile& File, T*& pObject) noexcept
    {
        auto Err = LoadHeader(File, sizeof(*pObject));
        pObject = (T*)LoadObject(File);
        ResolveObject(pObject);
        return Err;
    }

    //------------------------------------------------------------------------------

    template< class T > inline
        xfile::err xserialfile::Load(const xwstring& FileName, T*& pObject) noexcept
    {
        // Open the file
        xfile File;

        auto Err = File.Open(FileName, "rb");
        if (Err) return Err;

        // Load the object
        Load(File, pObject);

        // Close the file
        File.Close();

        return Err;
    }

    //------------------------------------------------------------------------------
    inline
        void xserialfile::setResourceVersion(u16 ResourceVersion) noexcept
    {
        m_Header.m_ResourceVersion = ResourceVersion;
    }

    //------------------------------------------------------------------------------
    inline
        u16 xserialfile::getResourceVersion(void) const noexcept
    {
        return m_Header.m_ResourceVersion;
    }

    //------------------------------------------------------------------------------
    inline
        bool xserialfile::SwapEndian(void) const noexcept
    {
        return m_pWrite->m_bEndian;
    }
}