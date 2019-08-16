
namespace xcore::math::examples::vector3s
{
    //-------------------------------------------------------------------------
    // VECTOR3D
    //-------------------------------------------------------------------------
    constexpr static auto max_rnd_v = static_cast<float>( RAND_MAX );
    float frand( float min, float max )
    {
        float D = max - min;
        return ((std::rand() / max_rnd_v) * D) - min;
    }

    namespace v3d
    {

        //-------------------------------------------------------------------------

        void Dot( void )
        {
            vector3d A( 100, 0, 0 );
            float dis = math::Sqrt( A.Dot(A) );
            xassert( math::FEqual( 100.0f, dis ) );
        }

        //-------------------------------------------------------------------------

        void GetLength( void )
        {
            vector3d A( 0, 100, 0 );
            float dis = A.getLength();
            xassert( math::FEqual( 100.0f, dis ) );
        }

        //-------------------------------------------------------------------------

        void GetLengthSquared( void )
        {
            vector3d A( 0, 0, 100 );
            float dis = A.getLengthSquared();
            xassert( math::FEqual( 100.0f*100.0f, dis ) );
        }

        //-------------------------------------------------------------------------

        void Normalize( void )
        {
            std::int32_t   i;
            for( i=0; i<1000; i++ )
            {
                vector3d A( frand(-100000.0f, 1000000.0), frand(-100000.0f, 1000000.0), frand(-100000.0f, 1000000.0) );
        
                A.Normalize();
                xassert( math::FEqual( A.getLength(), 1.0f ) );
            }
    
            xassert( i >= 1000 );
        }

        //-------------------------------------------------------------------------

        void NormalizeSafe( void )
        {
            std::int32_t i;
            for(  i=0; i<1000; i++ )
            {
                vector3d A( frand(-100000.0f, 1000000.0), frand(-100000.0f, 1000000.0), frand(-100000.0f, 1000000.0) );
        
                A.NormalizeSafe();
                xassert( math::FEqual( A.getLength(), 1.0f ) );
            }
    
            xassert( i >= 1000 );
        }

        //-------------------------------------------------------------------------

        void Operators( void )
        {
            vector3d A( 100, 0, 0), B(1,1,1), C;
    
            B = (B+B) / vector3d( 2 );
            C = A * B;
            C *= C;
            B -= B;
            C += B;
    
            // math::Math::vector3d.+-*etc OKAY
            xassert( math::FEqual( C.getLength(), A.getLengthSquared() ) );
        }

        //-------------------------------------------------------------------------

        void Test( void )
        {
            Dot();
            GetLength();
            GetLengthSquared();
            Normalize();
            NormalizeSafe();
            Operators();
        }
    }

    //-------------------------------------------------------------------------
    // VECTOR3
    //-------------------------------------------------------------------------
    namespace v3
    {
        //-------------------------------------------------------------------------

        void GetLength( void )
        {
            vector3 A( 0, 100, 0 );
            float dis = A.getLength();
            xassert( math::FEqual( 100.0f, dis ) );
        }

        //-------------------------------------------------------------------------

        void Normalize( void )
        {
            int i;
    
            for( i=0; i<1000; i++ )
            {
               vector3 A( frand(-100000.0f, 1000000.0), frand(-100000.0f, 1000000.0), frand(-100000.0f, 1000000.0) );
        
                A.Normalize();
                xassert( math::FEqual( A.getLength(), 1.0f ) );
            }
        }

        //-------------------------------------------------------------------------

        void NormalizeSafe( void )
        {
            int i;
            for( i=0; i<1000; i++ )
            {
                vector3 A( frand(-100000.0f, 1000000.0), frand(-100000.0f, 1000000.0), frand(-100000.0f, 1000000.0) );
        
                A.NormalizeSafe();
                xassert( math::FEqual( A.getLength(), 1.0f ) );
            }
        }

        //-------------------------------------------------------------------------

        void LengthSquared( void )
        {
            vector3 A( 100, 0, 0), B(1,1,1), C;
    
            B = (B+B) / vector3( 2 );
            C = A * B;
            C *= C;
            B -= B;
            C += B;
    
            // x_Math::xvector3.+-*etc OKAY
            xassert( math::FEqual( C.getLength(), A.getLengthSquared() ) );
        }

        //-------------------------------------------------------------------------

        void Dot( void )
        {
            vector3 A( 100, 0, 0 );
            float dis = math::Sqrt( A.Dot(A) );
            xassert( math::FEqual( 100.0f, dis ) );
        }


        //-------------------------------------------------------------------------

        void Test( void )
        {
            LengthSquared();
            NormalizeSafe();
            Normalize();
            Dot();
            GetLength();
        }
    }

    //-------------------------------------------------------------------------

    void Test( void )
    {
        v3d::Test();
        v3::Test();
    }
}