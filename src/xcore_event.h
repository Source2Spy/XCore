#ifndef _XCORE_EVENT_H
#define _XCORE_EVENT_H
#pragma once

// Usage Example:
//      #include <iostream>
//      class body
//      {
//      public:
//           void UpdateDelegates( void ) {  m_MyEvent.NotifyAll(22, 0.123f); }    // Call all delegates for this event err
//           xcore::event::event<int,float> m_MyEvent {};                             // Define an event err that we will emitted from this class 
//      };
//      
//      class other
//      {
//      public: 
//                  other           ( body& Body )      { m_MyEventDelegate.Connect( Body.m_MyEvent ); }
//      protected:
//          void    msgHandleEvent  ( int a, float b )  { std::cout << a << " " << b; }                     // Handle my messages here
//          xcore::event<int,float>::delegate m_MyEventDelegate   { this, &other::msgHandleEvent };     // Define the delegate
//      };
//      
//      int main( void )
//      {
//          body    TestBody;
//          other   TestOther{ TestBody };
//          
//          TestBody.UpdateDelegates();
//          
//          return 0;    
//      }

namespace xcore::event
{

    template< typename T_RET, class ...T_ARGS >
    class type_rtn_any;

    namespace details
    {
        //---------------------------------------------------------------------------
        // Delegates that can listen to events
        //---------------------------------------------------------------------------
        template< typename T_RET, class ...T_ARGS >
        class delegate
        {
            template< typename T, class ...T_ARGS >
            using fn    = T_RET(T::*)(T_ARGS...);
            using event = type_rtn_any<T_RET, T_ARGS...>;

        public:

            template< typename T_THIS >
            constexpr delegate(const fn<T_THIS, T_ARGS...> Function)
                : m_Call
                { [Function](void* pThis, T_ARGS ... Args)
                    {
                        (reinterpret_cast<T_THIS*>(pThis)->*Function)(std::forward<T_ARGS>(Args)...);
                    }
                }
                {}

            template< typename T_THIS >
            constexpr delegate(T_THIS* This, const fn<T_THIS, T_ARGS...> Function)
                : m_Call
                { [Function](void* pThis, T_ARGS ... Args) -> T_RET
                    {
                        if constexpr (std::is_same_v<void, T_RET>)  (reinterpret_cast<T_THIS*>(pThis)->*Function)(std::forward<T_ARGS>(Args)...);
                        else                                 return (reinterpret_cast<T_THIS*>(pThis)->*Function)(std::forward<T_ARGS>(Args)...);
                    }
                }
                , m_Offset{ static_cast<int>(reinterpret_cast<size_t>(This) - reinterpret_cast<size_t>(this)) }
                {}

            template< typename T_THIS >
            constexpr delegate(T_THIS* This, const fn<T_THIS, T_ARGS...> Function, event& Event)
                : m_Call
                { [Function](void* pThis, T_ARGS ... Args) -> T_RET
                    {
                        if constexpr (std::is_same_v<void, T_RET>)  (reinterpret_cast<T_THIS*>(pThis)->*Function)(std::forward<T_ARGS>(Args)...);
                        else                                 return (reinterpret_cast<T_THIS*>(pThis)->*Function)(std::forward<T_ARGS>(Args)...);
                    }
                }
                , m_Offset{ static_cast<int>(reinterpret_cast<size_t>(This) - reinterpret_cast<size_t>(this)) }
                {
                    Connect(Event);
                }

            template< typename T_THIS >
            inline void setupFunction(const fn<T_THIS, T_ARGS...> Function)
            {
                m_Call = [Function](void* pThis, T_ARGS ... Args) -> T_RET
                {
                    if constexpr (std::is_same_v<void, T_RET>)            (reinterpret_cast<T_THIS*>(pThis)->*Function)(std::forward<T_ARGS>(Args)...);
                    else                                            return  (reinterpret_cast<T_THIS*>(pThis)->*Function)(std::forward<T_ARGS>(Args)...);
                };
            }

            template< typename T_THIS >
            inline void setupThis(T_THIS& This)
            {
                m_Offset = static_cast<int>(reinterpret_cast<size_t>(&This) - reinterpret_cast<size_t>(this));
            }

            inline bool isConnected(void) const noexcept
            {
                return !!m_pEvent.load(std::memory_order_relaxed);
            }

            inline void Disconnect(void) noexcept
            {
                auto pPtr = lock();

                if (pPtr)
                {
                    // Inside Disconnect it will get unlock
                    pPtr->Disconnect(*this);
                }
                else
                {
                    unlock(nullptr);
                }
            }

            inline void Connect(event& Event) noexcept
            {
                auto pPtr = lock();

                xassert(pPtr == nullptr);
                Event.Connect(*this);

                unlock(&Event);
            }

            inline ~delegate(void) noexcept
            {
                Disconnect();
            }

        protected:

            inline auto lock(void) noexcept
            {
                do
                {
                    auto    local = m_pEvent.load(std::memory_order_relaxed);
                    auto    Lockp = reinterpret_cast<std::size_t>(local);

                    if (Lockp & mask_bit_v) continue;

                    Lockp |= mask_bit_v;
                    if (m_pEvent.compare_exchange_weak(local, reinterpret_cast<event*>(Lockp)))
                        return local;

                } while (true);
            }

            inline void unlock(event* pPtr) noexcept
            {
                xassert((reinterpret_cast<std::size_t>(pPtr)& mask_bit_v) == 0);
                m_pEvent.store(pPtr);
            }

        protected:

            using callback = xcore::function::buffer< 2, T_RET(void*, T_ARGS...) >;

            constexpr static std::size_t                                mask_bit_v = (1 << 0);

            std::atomic<event*>             m_pEvent{ nullptr };
            delegate*                       m_pNext{ nullptr };
            delegate*                       m_pPrev{ nullptr };
            const callback                  m_Call;
            int                             m_Offset{ 0 };
            friend event;
        };
    }

    //---------------------------------------------------------------------------
    // Generic event class able to deal with return types and not returns types
    //---------------------------------------------------------------------------
    template< typename T_RET, class ...T_ARGS >
    class type_rtn_any
    {
    public:

        using self                  = type_rtn_any;
        using delegate              = details::delegate<T_RET, T_ARGS...>;
        using arguments_tuple_t     = std::tuple<T_ARGS...>;
        using return_t              = T_RET;

    public:

        constexpr type_rtn_any(void) = default;

        inline ~type_rtn_any(void) noexcept
        {
            xcore::lock::scope Lock(m_Lock);

            // Remove all the delegates currently hooked on the event
            for (auto* pNext = m_pHead; pNext; )
            {
                auto const pNextNext = pNext->m_pNext;

                pNext->m_pNext = nullptr;
                pNext->m_pPrev = nullptr;
                pNext->unlock(nullptr);
                pNext = pNextNext;

            }

            // Reset the head
            m_pHead = nullptr;
        }

        inline T_RET NotifyAll(T_ARGS ... Args) const noexcept
        {
            xcore::lock::scope Lock(m_Lock);

            if constexpr (std::is_same_v<void, T_RET>)
            {
                for (auto pNext = m_pHead; pNext; pNext = pNext->m_pNext)
                {
                    auto uPtr = pNext->m_Offset + reinterpret_cast<size_t>(pNext);
                    pNext->m_Call(reinterpret_cast<void*>(uPtr), std::forward<T_ARGS>(Args)...);
                }
            }
            else
            {
                T_RET x = 0;
                for (auto pNext = m_pHead; pNext; pNext = pNext->m_pNext)
                {
                    auto uPtr = pNext->m_Offset + reinterpret_cast<size_t>(pNext);
                    x = pNext->m_Call(reinterpret_cast<void*>(uPtr), std::forward<T_ARGS>(Args)...);
                    if (x) break;
                }

                return x;
            }
        }

        constexpr bool hasSubscribers( void ) const noexcept { return m_pHead; } 

    private:

        inline void Connect(delegate & Delegate) noexcept
        {
            xcore::lock::scope Lock(m_Lock);

            Delegate.m_pNext = m_pHead;
            Delegate.m_pPrev = nullptr;
            if (m_pHead) m_pHead->m_pPrev = &Delegate;

            // set new head
            m_pHead = &Delegate;
        }

        inline void Disconnect(delegate & Delegate) noexcept
        {
            xcore::lock::scope Lock(m_Lock);
            if (Delegate.isConnected() == false) return;

            if (m_pHead == &Delegate)
            {
                m_pHead = Delegate.m_pNext;
            }
            else
            {
                Delegate.m_pPrev->m_pNext = Delegate.m_pNext;
            }

            if (Delegate.m_pNext) Delegate.m_pNext->m_pPrev = Delegate.m_pPrev;

            // officially release the lock and set the m_Event to null
            // it must be done before releasing m_Lock
            Delegate.unlock(nullptr);
        }

    private:

        mutable xcore::lock::spin   m_Lock{};
        delegate*                   m_pHead{ nullptr };

        friend delegate;
    };

    //---------------------------------------------------------------------------
    // Events without return types
    //---------------------------------------------------------------------------
    template< typename ...T_ARGS >
    using type = type_rtn_any< void, T_ARGS... >;

    //---------------------------------------------------------------------------
    // Events with a bool return err (bool indicates to stop sending the event)
    //---------------------------------------------------------------------------
    template< typename ...T_ARGS >
    using type_rtn = type_rtn_any< bool, T_ARGS... >;
}

#endif


