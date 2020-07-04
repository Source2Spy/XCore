
namespace xcore::random
{
    //--------------------------------------------------------------------------
    // The random generator seed
    inline
    void small_generator::setSeed64( std::uint64_t u ) noexcept
    {
        m_W = std::uint32_t(u);
        m_Z = std::uint32_t(u>>32);
    
        // The seed can not be zero in the lower bits
        xassert( m_W );
    
        // This one can not be zero neither but we will fix it for the user
        if( m_Z == 0 ) m_Z = 362436069;
    }
    
    //--------------------------------------------------------------------------
    inline
    void small_generator::setSeed32( std::uint32_t u ) noexcept
    {
        m_W = std::uint32_t(u);
        m_Z = 362436069 ^ std::uint32_t(u);
    
        xassert( m_W );
    }
    
    //--------------------------------------------------------------------------
    // get the full seed.
    inline
    std::uint64_t small_generator::getSeed( void ) const noexcept
    {
        std::uint64_t Seed = (std::uint64_t(m_W)<<0) |
        (std::uint64_t(m_Z)<<32);
    
        return Seed;
    }
    
    //--------------------------------------------------------------------------
    // Produce a uniform random sample from the open interval (0, 1).
    // The method will not return either end point.
    inline
    double small_generator::RandF64( void ) noexcept
    {
        // 0 <= u < 2^32
        std::uint32_t u = RandU32();
        // The magic number below is 1/(2^32 + 2).
        // The result is strictly between 0 and 1.
        return (u + 1.0) * 2.328306435454494e-10;
    }

    //--------------------------------------------------------------------------
    // Produce a uniform random sample from the open interval (0, 1).
    // The method will not return either end point.
    inline
    float small_generator::RandF32( void ) noexcept
    {
        return float(RandF64());
    }

    //------------------------------------------------------------------------------
    inline
    float small_generator::RandF32( float Start, float End ) noexcept
    {
        xassert(Start<End);
        return Start+RandF32()*(End-Start);
    }

    //------------------------------------------------------------------------------
    inline
    std::int32_t small_generator::RandS32( std::int32_t Start, std::int32_t End ) noexcept
    {
        xassert(Start<End);
        const std::int32_t L = End-Start;
        return Start+(RandU32()%L);
    }

    //------------------------------------------------------------------------------
    inline
    double small_generator::RandF64( double Start, double End ) noexcept
    {
        xassert(Start<End);
        return Start+RandF64()*(End-Start);
    }

    //--------------------------------------------------------------------------
    // This is the heart of the generator.
    // It uses George Marsaglia's MWC algorithm to produce an unsigned integer.
    // See http://www.bobwheeler.com/statistics/Password/MarsagliaPost.txt
    inline
    std::uint32_t small_generator::RandU32( void ) noexcept
    {
        m_Z = 36969 * (m_Z & 65535) + (m_Z >> 16);
        m_W = 18000 * (m_W & 65535) + (m_W >> 16);
        return (m_Z << 16) + m_W;
    }

    //--------------------------------------------------------------------------
    // Get normal (Gaussian) random sample with mean 0 and standard deviation 1
    inline
    double small_generator::Normal( void ) noexcept
    {
        // Use Box-Muller algorithm
        double u1 = RandF64();
        double u2 = RandF64();
        double r = xcore::math::SqrtDouble( -2.0f * xcore::math::LogDouble(u1) );
        double theta = 2.0 * PI_64.m_Value * u2;
        return r*xcore::math::SinDouble( xcore::math::radian64{ theta } );
    }

    //--------------------------------------------------------------------------
    // Get normal (Gaussian) random sample with specified mean and standard deviation
    inline
    double small_generator::Normal( double mean, double standardDeviation ) noexcept
    {
        // Shape must be positive. Received {0}.
        xassert(standardDeviation > 0.0 );
        return mean + standardDeviation * Normal();
    }

    //--------------------------------------------------------------------------
    // Get exponential random sample with mean 1
    inline
    double small_generator::Exponential( void ) noexcept
    {
        return xcore::math::LogDouble( RandF64() );
    }

    //--------------------------------------------------------------------------
    // Get exponential random sample with specified mean
    inline
    double small_generator::Exponential( double mean ) noexcept
    {
        // Mean must be positive. Received {0}.
        xassert(mean > 0.0);
        return mean * Exponential();
    }

    //--------------------------------------------------------------------------
    inline
    double small_generator::Gamma( double shape, double scale ) noexcept
    {
        // Implementation based on "A Simple Method for Generating Gamma Variables"
        // by George Marsaglia and Wai Wan Tsang.  ACM Transactions on Mathematical Software
        // Vol 26, No 3, September 2000, pages 363-372.
        double d, c, x, xsquared, v, u;
    
        if (shape >= 1.0)
        {
            d = shape - 1.0/3.0;
            c = 1.0/xcore::math::SqrtDouble(9.0*d);
            for (;;)
            {
                do
                {
                    x = Normal();
                    v = 1.0 + c*x;
                }
                while (v <= 0.0);
                v = v*v*v;
                u = RandF64();
                xsquared = x*x;
                if (u < 1.0 -.0331*xsquared*xsquared || xcore::math::LogDouble(u) < 0.5*xsquared + d*(1.0 - v + xcore::math::LogDouble(v)))
                    return scale*d*v;
            }
        }
        else
        {
            // Shape must be positive. Received {0}
            xassert( shape > 0.0 );
        
            double g = Gamma(shape+1.0, 1.0);
            double w = RandF64();
            return scale*g*xcore::math::PowDouble(w, 1.0/shape);
        }
    }

    //--------------------------------------------------------------------------
    inline
    double small_generator::ChiSquare( double degreesOfFreedom ) noexcept
    {
        // A chi squared distribution with n degrees of freedom
        // is a gamma distribution with shape n/2 and scale 2.
        return Gamma(0.5 * degreesOfFreedom, 2.0);
    }

    //--------------------------------------------------------------------------
    inline
    double small_generator::InverseGamma( double shape, double scale ) noexcept
    {
        // If X is gamma(shape, scale) then
        // 1/Y is inverse gamma(shape, 1/scale)
        return 1.0 / Gamma(shape, 1.0 / scale);
    }

    //--------------------------------------------------------------------------
    inline
    double small_generator::Weibull( double shape, double scale ) noexcept
    {
        // Shape and scale parameters must be positive. Recieved shape {0} and scale{1}.
        assert( shape > 0 && scale > 0 );
    
        return scale * xcore::math::PowDouble(-xcore::math::LogDouble(RandF64()), 1.0 / shape);
    }

    //--------------------------------------------------------------------------
    inline
    double small_generator::Cauchy( double median, double scale ) noexcept
    {
        // Scale must be positive. Received {0}
        xassert( scale > 0 );
    
        double p = RandF64();
    
        // Apply inverse of the Cauchy distribution function to a uniform
        return median + scale*xcore::math::TanDouble( xcore::math::radian64{ PI_64.m_Value*(p - 0.5) } );
    }

    //--------------------------------------------------------------------------
    inline
    double small_generator::StudentT( double degreesOfFreedom ) noexcept
    {
        // Degrees of freedom must be positive. Received {0}.
        xassert( degreesOfFreedom > 0 );
    
        // See Seminumerical Algorithms by Knuth
        double y1 = Normal();
        double y2 = ChiSquare(degreesOfFreedom);
        return y1 / xcore::math::SqrtDouble(y2 / degreesOfFreedom);
    }

    //--------------------------------------------------------------------------
    // The Laplace distribution is also known as the double exponential distribution.
    inline
    double small_generator::Laplace( double mean, double scale ) noexcept
    {
        double u = RandF64();
        return (u < 0.5) ?
        mean + scale*xcore::math::LogDouble(2.0*u) :
        mean - scale*xcore::math::LogDouble(2*(1-u));
    }

    //--------------------------------------------------------------------------
    inline
    double small_generator::LogNormal( double mu, double sigma ) noexcept
    {
        return xcore::math::ExpDouble(Normal(mu, sigma));
    }

    //--------------------------------------------------------------------------
    inline
    double small_generator::Beta( double a, double b ) noexcept
    {
        // Beta parameters must be positive. Received {0} and {1}.
        xassert( a > 0 && b > 0 );
    
        // There are more efficient methods for generating beta samples.
        // However such methods are a little more efficient and much more complicated.
        // For an explanation of why the following method works, see
        // http://www.johndcook.com/distribution_chart.html#gamma_beta
    
        double u = Gamma(a, 1.0);
        double v = Gamma(b, 1.0);
        return u / (u + v);
    }
}
