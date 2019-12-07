
#if 0

namespace xcore::serializer
{
    /*
    namespace details
    {
        template<typename, typename T>
        struct has_serialize 
        {
            static_assert( std::integral_constant<T, false>::value, "Second template parameter needs to be of function type." );
        };

        // specialization that does the checking

        template<typename C, typename Ret, typename... Args>
        struct has_serialize<C, Ret(Args...)> 
        {
        private:

            template<typename T>
            static constexpr auto check(T*) -> typename 
                std::is_same< decltype( std::declval<T>().SerializeIO( xcore::file::stream& ) ),
                             Ret    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                            >::type;  // attempt to call it and see if the return type is correct
            
			template<typename>
            static constexpr std::false_type check(...);

            using type = decltype(check<C>(0));

        public:
            static constexpr bool value = type::value;
        };
    }
    */

    namespace details
    {
        template<class T>
        using has_serialization = decltype(std::declval<T&>().SerializeIO(std::declval<xcore::file::stream>()));

        template<class T>
        constexpr static bool has_serialization_v = xcore::types::is_detected_exact_v<xcore::err, has_serialization, T>;
    }

    //------------------------------------------------------------------------------

    inline
    file::stream& stream::getW(void) noexcept
    {
        xassert(m_pWrite);
        return m_pWrite->m_Packs[m_iPack].m_Data;
    }

    //------------------------------------------------------------------------------

    constexpr
    bool stream::isLocalVariable( const std::byte* pRange ) const noexcept
    {
        return (pRange >= m_pClass) && (pRange < (m_pClass + m_ClassSize));
    }

    //------------------------------------------------------------------------------

    constexpr
    std::int32_t stream::ComputeLocalOffset( const std::byte* pItem ) const noexcept
    {
        xassert(isLocalVariable(pItem));
        return static_cast<std::int32_t>(pItem - m_pClass);
    }

    //------------------------------------------------------------------------------

    template< class T > inline
    xcore::err stream::Save( const string::view<const wchar_t> FileName, const T& Object, mem_type ObjectFlags, bool bEndianSwap ) noexcept
    {
        // Open the file to make sure we are okay
        xcore::err              Error;
        xcore::file::stream     File;

        // 
        if( File.open(FileName, "wb").isError(Error) ) return Error;

        // Call the actual save
        if( Save(File, Object, ObjectFlags, bEndianSwap).isError(Error) ) return Error;

        // done with the file
        File.close();

        return Error;
    }

    //------------------------------------------------------------------------------

    template< class T > inline
    xcore::err stream::Save( xcore::file::stream& File, const T& Object, mem_type ObjectFlags, bool bSwapEndian ) noexcept
    {
        xcore::err Error;

        //
        // Allocate the writing structure
        //
        std::unique_ptr<writting> Write{ new writting };

        // Assign it so it is accessible for other functions
        // but we keep the owner
        m_pWrite = Write.get();

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
        if (getW().putC(' ', m_ClassSize, true).isError(Error)) return Error;

        // Start the saving 
        if (xcore::serializer::io_functions::SerializeIO(*this, Object).isError(Error)) return Error;

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
            if(Handle(std::span<const std::byte>{&reinterpret_cast<const std::byte&>(A), sizeof(T)}).isError(Error)) return Error;
        }
        else if constexpr (std::is_array_v<T>)
        {
            // Handle C arrays
            for (int i = 0; i < std::extent_v<T>; ++i) if (Serialize(A[i]).isError(Error)) return Error;
        }
        else if constexpr (types::is_array_v<T> || types::is_span_v<T> )
        {
            static_assert(std::is_object_v<T>);
            for (auto& X : A) if (Serialize(X).isError(Error)) return Error;
        }
        else if constexpr (details::has_serialization_v<T>)
        {
            static_assert(std::is_object_v<T>);
            static_assert(false == std::is_polymorphic_v<T>);

            // Copy most of the data
            stream File(*this);

            File.m_iPack        = m_iPack;
            File.m_ClassPos     = static_cast<std::uint32_t>(getW().Tell());
            File.m_pClass       = &const_cast<std::byte&>(reinterpret_cast<const std::byte&>(A));
            File.m_ClassSize    = sizeof(A);

            if (xcore::serializer::io_functions::SerializeIO(File, A).isError(Error)) return Error;

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
            if (Handle(std::span<const std::byte>{ &reinterpret_cast<const std::byte&>(A), sizeof(T) }).isError(Error)) return Error;
        }
        else
        {
            static_assert(xcore::types::always_false_v<T>);
        }

        return Error;
    }

    //------------------------------------------------------------------------------

    template< class T, typename T_SIZE  > inline
    xcore::err stream::Serialize( T* const& pView, T_SIZE Size, mem_type MemoryFlags) noexcept
    {
        xcore::err  Error;
        const auto  BackupPackIndex = m_iPack;

        // Handle pointer details
        if( HandlePtrDetails
        (
              reinterpret_cast<const std::byte*>(&pView)
            , sizeof(T)
            , Size
            , MemoryFlags
        ).isError(Error)) return Error;

        //
        // Loop throw all the items
        //

        // Now loop
        const units::bytes Count{ static_cast<std::int64_t>(Size * sizeof(T)) };

        for (std::int64_t i = 0; i < Count.m_Value; i++)
        {
            if (Serialize(pView[i]).isError(Error)) return Error;
            xassert_block_basic()
            {
                units::bytes X;
                getW().Tell(X).CheckError();
                xassert(X.m_Value >= i );
            }
        }

        //
        // Restore the old pack
        //
        m_iPack = BackupPackIndex;
        return Error;
    }

    //------------------------------------------------------------------------------
    inline
    xcore::err stream::Handle( const std::span<const std::byte> View ) noexcept
    {
        xassert(View.size() >= 0);
        
        // If it is not a local variable then you must pass the memory type
        xassert(isLocalVariable(View.data()));

        // Make sure that we are at the right offset
        if (auto Error = getW().SeekOrigin(units::bytes{ m_ClassPos + ComputeLocalOffset(View.data()) }); Error ) return Error;

        // Write the data
        return getW().WriteView(View);
    }

    //------------------------------------------------------------------------------

    template< class T > inline
    void stream::ResolveObject(T*& pObject) noexcept
    {
        // Initialize all
		(void)new(pObject) T{ *this };

        // deal with temp data
        if (m_bFreeTempData && m_pTempBlockData)
            m_pTempBlockData->~T();
    }


    //------------------------------------------------------------------------------

    template< class T > inline
	xcore::err stream::Load(file::stream& File, T*& pObject) noexcept
    {
		xcore::err   Error;
		if (LoadHeader(File, sizeof(*pObject)).isError(Error)) return Error;
        pObject = (T*)LoadObject(File);
        ResolveObject(pObject);
        return Error;
    }

    //------------------------------------------------------------------------------

    template< class T > inline
	xcore::err stream::Load(const string::view<wchar_t> FileName, T*& pObject) noexcept
    {
		xcore::err   Error;
        file::stream File;

        if (File.open(FileName, "rb").isError(Error)) return Error;

        // Load the object
        Load(File, pObject);

        // Close the file
        File.close();

        return Error;
    }

    //------------------------------------------------------------------------------
    inline
    void stream::setResourceVersion(std::uint16_t ResourceVersion) noexcept
    {
        m_Header.m_ResourceVersion = ResourceVersion;
    }

    //------------------------------------------------------------------------------
    constexpr
    std::uint16_t stream::getResourceVersion(void) const noexcept
    {
        return m_Header.m_ResourceVersion;
    }

    //------------------------------------------------------------------------------
    constexpr
    bool stream::SwapEndian(void) const noexcept
    {
        return m_pWrite->m_bEndian;
    }
}

#endif