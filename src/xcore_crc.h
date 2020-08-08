#ifndef _XCORE_CRC_H
#define _XCORE_CRC_H
#pragma once

// Not too sure which is faster to compile with
#define XCORE_CRC_STATIC_TABLE true

namespace xcore
{
    namespace crc_details
    {
        //-------------------------------------------------------------------------------------------------------
        template< typename T >
        struct table;

        //-------------------------------------------------------------------------------------------------------
        template< std::size_t T_BITS_V >
        using to_t = types::byte_size_uint_t<T_BITS_V/8>;

        //-------------------------------------------------------------------------------------------------------
        // Generates CRC-X table, algorithm based from this link:
        //-------------------------------------------------------------------------------------------------------
        template< typename T, T T_POLY >
        constexpr auto MakeTable( void )
        {
            std::array<T, 256>  Table {};
            for( auto& crc : Table )  
            {
                crc = static_cast<T>(&crc - Table.data());
                for( auto i = 8; i--; crc = (T_POLY & -static_cast<xcore::types::to_int_t<T>>(crc&1)) ^ (crc >> 1) );
            }
            return Table;
        }

        //-------------------------------------------------------------------------------------------------------
        // Generates CRC-64 table, algorithm based from this link:
        // https://searchcode.com/codesearch/view/6414554/
        //-------------------------------------------------------------------------------------------------------
        template<>
        struct table< std::uint64_t >
        {
        #if XCORE_CRC_STATIC_TABLE
            static constexpr std::array<std::uint64_t,256> ms_Table =
            {
                0x0000000000000000,0x7AD870C830358979,0xF5B0E190606B12F2,0x8F689158505E9B8B,0xC038E5739841B68F,0xBAE095BBA8743FF6,
                0x358804E3F82AA47D,0x4F50742BC81F2D04,0xAB28ECB46814FE75,0xD1F09C7C5821770C,0x5E980D24087FEC87,0x24407DEC384A65FE,
                0x6B1009C7F05548FA,0x11C8790FC060C183,0x9EA0E857903E5A08,0xE478989FA00BD371,0x7D08FF3B88BE6F81,0x07D08FF3B88BE6F8,
                0x88B81EABE8D57D73,0xF2606E63D8E0F40A,0xBD301A4810FFD90E,0xC7E86A8020CA5077,0x4880FBD87094CBFC,0x32588B1040A14285,
                0xD620138FE0AA91F4,0xACF86347D09F188D,0x2390F21F80C18306,0x594882D7B0F40A7F,0x1618F6FC78EB277B,0x6CC0863448DEAE02,
                0xE3A8176C18803589,0x997067A428B5BCF0,0xFA11FE77117CDF02,0x80C98EBF2149567B,0x0FA11FE77117CDF0,0x75796F2F41224489,
                0x3A291B04893D698D,0x40F16BCCB908E0F4,0xCF99FA94E9567B7F,0xB5418A5CD963F206,0x513912C379682177,0x2BE1620B495DA80E,
                0xA489F35319033385,0xDE51839B2936BAFC,0x9101F7B0E12997F8,0xEBD98778D11C1E81,0x64B116208142850A,0x1E6966E8B1770C73,
                0x8719014C99C2B083,0xFDC17184A9F739FA,0x72A9E0DCF9A9A271,0x08719014C99C2B08,0x4721E43F0183060C,0x3DF994F731B68F75,
                0xB29105AF61E814FE,0xC849756751DD9D87,0x2C31EDF8F1D64EF6,0x56E99D30C1E3C78F,0xD9810C6891BD5C04,0xA3597CA0A188D57D,
                0xEC09088B6997F879,0x96D1784359A27100,0x19B9E91B09FCEA8B,0x636199D339C963F2,0xDF7ADABD7A6E2D6F,0xA5A2AA754A5BA416,
                0x2ACA3B2D1A053F9D,0x50124BE52A30B6E4,0x1F423FCEE22F9BE0,0x659A4F06D21A1299,0xEAF2DE5E82448912,0x902AAE96B271006B,
                0x74523609127AD31A,0x0E8A46C1224F5A63,0x81E2D7997211C1E8,0xFB3AA75142244891,0xB46AD37A8A3B6595,0xCEB2A3B2BA0EECEC,
                0x41DA32EAEA507767,0x3B024222DA65FE1E,0xA2722586F2D042EE,0xD8AA554EC2E5CB97,0x57C2C41692BB501C,0x2D1AB4DEA28ED965,
                0x624AC0F56A91F461,0x1892B03D5AA47D18,0x97FA21650AFAE693,0xED2251AD3ACF6FEA,0x095AC9329AC4BC9B,0x7382B9FAAAF135E2,
                0xFCEA28A2FAAFAE69,0x8632586ACA9A2710,0xC9622C4102850A14,0xB3BA5C8932B0836D,0x3CD2CDD162EE18E6,0x460ABD1952DB919F,
                0x256B24CA6B12F26D,0x5FB354025B277B14,0xD0DBC55A0B79E09F,0xAA03B5923B4C69E6,0xE553C1B9F35344E2,0x9F8BB171C366CD9B,
                0x10E3202993385610,0x6A3B50E1A30DDF69,0x8E43C87E03060C18,0xF49BB8B633338561,0x7BF329EE636D1EEA,0x012B592653589793,
                0x4E7B2D0D9B47BA97,0x34A35DC5AB7233EE,0xBBCBCC9DFB2CA865,0xC113BC55CB19211C,0x5863DBF1E3AC9DEC,0x22BBAB39D3991495,
                0xADD33A6183C78F1E,0xD70B4AA9B3F20667,0x985B3E827BED2B63,0xE2834E4A4BD8A21A,0x6DEBDF121B863991,0x1733AFDA2BB3B0E8,
                0xF34B37458BB86399,0x8993478DBB8DEAE0,0x06FBD6D5EBD3716B,0x7C23A61DDBE6F812,0x3373D23613F9D516,0x49ABA2FE23CC5C6F,
                0xC6C333A67392C7E4,0xBC1B436E43A74E9D,0x95AC9329AC4BC9B5,0xEF74E3E19C7E40CC,0x601C72B9CC20DB47,0x1AC40271FC15523E,
                0x5594765A340A7F3A,0x2F4C0692043FF643,0xA02497CA54616DC8,0xDAFCE7026454E4B1,0x3E847F9DC45F37C0,0x445C0F55F46ABEB9,
                0xCB349E0DA4342532,0xB1ECEEC59401AC4B,0xFEBC9AEE5C1E814F,0x8464EA266C2B0836,0x0B0C7B7E3C7593BD,0x71D40BB60C401AC4,
                0xE8A46C1224F5A634,0x927C1CDA14C02F4D,0x1D148D82449EB4C6,0x67CCFD4A74AB3DBF,0x289C8961BCB410BB,0x5244F9A98C8199C2,
                0xDD2C68F1DCDF0249,0xA7F41839ECEA8B30,0x438C80A64CE15841,0x3954F06E7CD4D138,0xB63C61362C8A4AB3,0xCCE411FE1CBFC3CA,
                0x83B465D5D4A0EECE,0xF96C151DE49567B7,0x76048445B4CBFC3C,0x0CDCF48D84FE7545,0x6FBD6D5EBD3716B7,0x15651D968D029FCE,
                0x9A0D8CCEDD5C0445,0xE0D5FC06ED698D3C,0xAF85882D2576A038,0xD55DF8E515432941,0x5A3569BD451DB2CA,0x20ED197575283BB3,
                0xC49581EAD523E8C2,0xBE4DF122E51661BB,0x3125607AB548FA30,0x4BFD10B2857D7349,0x04AD64994D625E4D,0x7E7514517D57D734,
                0xF11D85092D094CBF,0x8BC5F5C11D3CC5C6,0x12B5926535897936,0x686DE2AD05BCF04F,0xE70573F555E26BC4,0x9DDD033D65D7E2BD,
                0xD28D7716ADC8CFB9,0xA85507DE9DFD46C0,0x273D9686CDA3DD4B,0x5DE5E64EFD965432,0xB99D7ED15D9D8743,0xC3450E196DA80E3A,
                0x4C2D9F413DF695B1,0x36F5EF890DC31CC8,0x79A59BA2C5DC31CC,0x037DEB6AF5E9B8B5,0x8C157A32A5B7233E,0xF6CD0AFA9582AA47,
                0x4AD64994D625E4DA,0x300E395CE6106DA3,0xBF66A804B64EF628,0xC5BED8CC867B7F51,0x8AEEACE74E645255,0xF036DC2F7E51DB2C,
                0x7F5E4D772E0F40A7,0x05863DBF1E3AC9DE,0xE1FEA520BE311AAF,0x9B26D5E88E0493D6,0x144E44B0DE5A085D,0x6E963478EE6F8124,
                0x21C640532670AC20,0x5B1E309B16452559,0xD476A1C3461BBED2,0xAEAED10B762E37AB,0x37DEB6AF5E9B8B5B,0x4D06C6676EAE0222,
                0xC26E573F3EF099A9,0xB8B627F70EC510D0,0xF7E653DCC6DA3DD4,0x8D3E2314F6EFB4AD,0x0256B24CA6B12F26,0x788EC2849684A65F,
                0x9CF65A1B368F752E,0xE62E2AD306BAFC57,0x6946BB8B56E467DC,0x139ECB4366D1EEA5,0x5CCEBF68AECEC3A1,0x2616CFA09EFB4AD8,
                0xA97E5EF8CEA5D153,0xD3A62E30FE90582A,0xB0C7B7E3C7593BD8,0xCA1FC72BF76CB2A1,0x45775673A732292A,0x3FAF26BB9707A053,
                0x70FF52905F188D57,0x0A2722586F2D042E,0x854FB3003F739FA5,0xFF97C3C80F4616DC,0x1BEF5B57AF4DC5AD,0x61372B9F9F784CD4,
                0xEE5FBAC7CF26D75F,0x9487CA0FFF135E26,0xDBD7BE24370C7322,0xA10FCEEC0739FA5B,0x2E675FB4576761D0,0x54BF2F7C6752E8A9,
                0xCDCF48D84FE75459,0xB71738107FD2DD20,0x387FA9482F8C46AB,0x42A7D9801FB9CFD2,0x0DF7ADABD7A6E2D6,0x772FDD63E7936BAF,
                0xF8474C3BB7CDF024,0x829F3CF387F8795D,0x66E7A46C27F3AA2C,0x1C3FD4A417C62355,0x935745FC4798B8DE,0xE98F353477AD31A7,
                0xA6DF411FBFB21CA3,0xDC0731D78F8795DA,0x536FA08FDFD90E51,0x29B7D047EFEC8728
            };
        #else
            static constexpr auto ms_Table = MakeCRCTable<std::uint64_t,0x95AC9329AC4BC9B5ull>();
            static_assert(     ms_Table.size() == 256 
                            && ms_Table[1]     == 0x7AD870C830358979ULL
                            && ms_Table[255]   == 0x29B7D047EFEC8728ULL );
        #endif
        };

        //-------------------------------------------------------------------------------------------------------
        // Generates CRC-32 table, algorithm based from this link:
        // http://www.hackersdelight.org/hdcodetxt/crc.c.txt
        //-------------------------------------------------------------------------------------------------------
        template<>
        struct table< std::uint32_t >
        {
        #if XCORE_CRC_STATIC_TABLE
            static constexpr std::array<std::uint32_t,256> ms_Table =
            {
                0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419,
                0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4,
                0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07,
                0x90BF1D91, 0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
                0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7, 0x136C9856,
                0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
                0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4,
                0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
                0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3,
                0x45DF5C75, 0xDCD60DCF, 0xABD13D59, 0x26D930AC, 0x51DE003A,
                0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599,
                0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
                0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190,
                0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F,
                0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E,
                0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
                0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED,
                0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
                0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3,
                0xFBD44C65, 0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
                0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A,
                0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5,
                0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA, 0xBE0B1010,
                0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
                0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17,
                0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6,
                0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615,
                0x73DC1683, 0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
                0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1, 0xF00F9344,
                0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
                0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A,
                0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
                0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1,
                0xA6BC5767, 0x3FB506DD, 0x48B2364B, 0xD80D2BDA, 0xAF0A1B4C,
                0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF,
                0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
                0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE,
                0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31,
                0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226, 0x756AA39C,
                0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
                0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B,
                0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
                0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1,
                0x18B74777, 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
                0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 0xA00AE278,
                0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7,
                0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC, 0x40DF0B66,
                0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
                0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605,
                0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8,
                0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B,
                0x2D02EF8D
            };
        #else
            static constexpr auto ms_Table = MakeCRCTable<std::uint32_t,0xEDB88320>();
            static_assert(     ms_Table.size() == 256 
                            && ms_Table[1]     == 0x77073096 
                            && ms_Table[255]   == 0x2D02EF8D );
        #endif
        };

        //-------------------------------------------------------------------------------------------------------
        // Generates CRC-16 table
        // https://cs.fit.edu/code/svn/cse2410f13team7/wireshark/wsutil/crc16.c
        //-------------------------------------------------------------------------------------------------------
        template<>
        struct table< std::uint16_t >
        {
        #if XCORE_CRC_STATIC_TABLE
            static constexpr std::array<std::uint32_t,256> ms_Table =
            {
                0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
                0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
                0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
                0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
                0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
                0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
                0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
                0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
                0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
                0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
                0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
                0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
                0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
                0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
                0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
                0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
                0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
                0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
                0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
                0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
                0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
                0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
                0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
                0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
                0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
                0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
                0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
                0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
                0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
                0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
                0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
                0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
            };
        #else
            static constexpr auto ms_Table = []() constexpr noexcept 
            {
                std::array<std::uint16_t, 256>  Table {};
                for( auto& crc : Table )  
                {
                    crc = static_cast<std::uint16_t>(&crc - Table.data()) << 8;
                    for( auto i = 8; i--; crc = (crc & 0x8000) ? ( crc << 1 ) ^ 0x1021 : ( crc << 1) );
                }
                return Table;
            }();
            static_assert(     ms_Table.size() == 256 
                            && ms_Table[1]     == 0x1021 
                            && ms_Table[255]   == 0x1EF0 );
        #endif
        };

        //-------------------------------------------------------------------------------------------------------
        // Generates CRC-8 table
        // https://lentz.com.au/blog/tag/crc-table-generator 
        //-------------------------------------------------------------------------------------------------------
        template<>
        struct table< std::uint8_t >
        {
        #if XCORE_CRC_STATIC_TABLE
            static constexpr std::array<std::uint8_t,32> ms_Table =
            {
                0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83,
                0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
                0x00, 0x9D, 0x23, 0xBE, 0x46, 0xDB, 0x65, 0xF8,
                0x8C, 0x11, 0xAF, 0x32, 0xCA, 0x57, 0xE9, 0x74
            };
        #else
            static constexpr auto ms_Table = []() constexpr noexcept 
            {
                std::array<std::uint8_t, 32>  Table {};
                for( auto& crc : Table )  
                {
                    crc = static_cast<std::uint8_t>(&crc - Table.data());
                    if( crc>15 ) crc = (crc-16)<<4;
                    for( auto i = 8; i--; crc = (0x8C & -static_cast<std::int8_t>(crc&1)) ^ (crc >> 1) );
                }
                return Table;
            }();
            static_assert(     ms_Table.size() == 32 
                            && ms_Table[1]     == 0x5E 
                            && ms_Table[31]    == 0x74 );
        #endif
        };
    }

    //-------------------------------------------------------------------------------------------------------
    // Basic CRC type 
    //-------------------------------------------------------------------------------------------------------
    template< std::size_t T_BITS_V >
    struct crc : xcore::units::type< crc<T_BITS_V>, crc_details::to_t<T_BITS_V> >
    {
        using parent  = xcore::units::type< crc<T_BITS_V>, crc_details::to_t<T_BITS_V> >;
        using basic_t = typename parent::basic_t;
        using self    = crc<T_BITS_V>;

        using parent::parent;

        static_assert(T_BITS_V == 8 || T_BITS_V == 16 || T_BITS_V == 32 || T_BITS_V == 64, "");

        constexpr static self FromString(const char* pIn, basic_t Value = basic_t{ 0 }) noexcept
        {
            xassert(pIn);
            for (auto i = 0u; auto c = static_cast<basic_t>(pIn[i]); ++i)
            {
                if constexpr (T_BITS_V == 8)
                {
                    c = c ^ Value;
                    Value = crc_details::table<basic_t>::ms_Table[c & 0x0f] ^ crc_details::table<basic_t>::ms_Table[16l + ((c >> 4) & 0x0f)];
                }
                else
                {
                    Value = crc_details::table<basic_t>::ms_Table[(Value ^ c) & 0xFF] ^ (Value >> 8);
                }
            }

            return self{ Value };
        }

        xforceinline self& setupFromString( const char* pIn ) noexcept
        {
            return *this = FromString(pIn);
        }

        constexpr self FromBytes(std::span<const std::byte> View, basic_t Value = basic_t{ 0 }) noexcept
        {
            assert(View.size());
            assert(View.data());

            for(auto c : View )
            {
                if constexpr (T_BITS_V == 8)
                {
                    c = c ^ Value;
                    Value = crc_details::table<basic_t>::ms_Table[c & 0x0f] ^ crc_details::table<basic_t>::ms_Table[16l + ((c >> 4) & 0x0f)];
                }
                else
                {
                    Value = crc_details::table<basic_t>::ms_Table[(Value ^ static_cast<basic_t>(c)) & 0xFF] ^ (Value >> 8);
                }
            }

            return self{ Value };
        }

        xforceinline self& setupFromBytes( std::span<const std::byte> View ) noexcept
        {
            return *this = FromBytes(View);
        }
    };
}

//------------------------------------------------------------------------------
// Add as a potential hash 
//------------------------------------------------------------------------------
template< std::size_t S > 
struct std::hash<xcore::crc<S>>
{
    using argument_type = xcore::crc<S>;
    using result_type   = std::size_t;
    result_type operator()(argument_type const& s) const noexcept
    {
        return s.m_Value;
    }
};

#endif
