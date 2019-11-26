#ifndef _XCORE_RANDOM_H
#define _XCORE_RANDOM_H
#pragma once

namespace xcore::random
{
    //------------------------------------------------------------------------------
    // Description:
    //     small_generator is a simple random number generator based on
    //     George Marsaglia's MWC (multiply with carry) generator.
    //     Although it is very simple, it passes Marsaglia's DIEHARD
    //     series of random number generator tests.
    //------------------------------------------------------------------------------
    class small_generator
    {
    public:

        constexpr                           small_generator     ( void )                                                noexcept = default;
    
        inline          void                setSeed64           ( std::uint64_t u )                                     noexcept;
        inline          void                setSeed32           ( std::uint32_t u )                                     noexcept;
        inline          std::uint64_t       getSeed             ( void )                                        const   noexcept;
          
                  
        inline          std::uint32_t       RandU32             ( void )                                                noexcept;
        inline          std::int32_t        RandS32             ( std::int32_t Start, std::int32_t End )                noexcept;

        inline          double              RandF64             ( void )                                                noexcept;
        inline          float               RandF32             ( void )                                                noexcept;
        inline          float               RandF32             ( float Start, float End )                              noexcept;
        inline          double              RandF64             ( double Start, double End )                            noexcept;
          
        inline          double              Normal              ( void )                                                noexcept;
        inline          double              Normal              ( double mean, double standardDeviation )               noexcept;
        inline          double              Exponential         ( void )                                                noexcept;
        inline          double              Exponential         ( double mean )                                         noexcept;
        inline          double              Gamma               ( double shape, double scale )                          noexcept;
        inline          double              ChiSquare           ( double degreesOfFreedom )                             noexcept;
        inline          double              InverseGamma        ( double shape, double scale )                          noexcept;
        inline          double              Weibull             ( double shape, double scale )                          noexcept;
        inline          double              Cauchy              ( double median, double scale )                         noexcept;
        inline          double              StudentT            ( double degreesOfFreedom )                             noexcept;
        inline          double              Laplace             ( double mean, double scale )                           noexcept;
        inline          double              LogNormal           ( double mu, double sigma )                             noexcept;
        inline          double              Beta                ( double a, double b )                                  noexcept;
    
    protected:
    
        // These values are not magical, just the default values Marsaglia used.
        // Any pair of unsigned integers should be fine.
        std::uint32_t         m_W { 521288629 };
        std::uint32_t         m_Z { 362436069 };
    };
}

#endif