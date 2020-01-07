/* imported the crc32 checksum code from gds/nds2 server */
/* added two defines
 * if CRC32_AUTO_GENERATE_TABLE is set then do not use a precomputed lookup for
 * the 8 way slice.  Generate it at runtime. if CRC32_MAIN then a main function
 * is generated which computes and prints the 8 way slice. These are wanted for
 * reproducability, the generated table has been included and enabled by
 * default.
 */

//======================================  Set endianness on linux, gcc
#include <stdint.h>
#undef USE_LITTLE_ENDIAN
#undef USE_BIG_ENDIAN
#if defined( __linux )
#include <endian.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define USE_LITTLE_ENDIAN 1
#else
#warning "Big-endian checksum"
#define USE_BIG_ENDIAN 1
#endif
#elif defined( __BYTE_ORDER__ )
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define USE_LITTLE_ENDIAN 1
#else
#warning "Big-endian checksum"
#define USE_BIG_ENDIAN 1
#endif
#else
#warning "Byte ordering not available"
#define USE_BIG_ENDIAN 1
#endif

//======================================  Checksum calculation table
static uint32_t const crctab[ 256 ] = {
    0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
    0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
    0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
    0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
    0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
    0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
    0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
    0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
    0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
    0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
    0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
    0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
    0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
    0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
    0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
    0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
    0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
    0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
    0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
    0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
    0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
    0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
    0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
    0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
    0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
    0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
    0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
    0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
    0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
    0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
    0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
    0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
    0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
    0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
    0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
    0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
    0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
    0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
    0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
    0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
    0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
    0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
    0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

//======================================  Reversed table for 8-byte slices
#ifdef USE_LITTLE_ENDIAN
#ifdef CRC32_AUTO_GENERATE_TABLE
static uint32_t crctab_8byte[ 8 ][ 256 ];
static bool     crctab_set = false;
#else // CRC32_AUTO_GENERATE_TABLE

static const uint32_t crctab_8byte[ 8 ][ 256 ] = {
    { 0x00000000, 0xb71dc104, 0x6e3b8209, 0xd926430d, 0xdc760413, 0x6b6bc517,
      0xb24d861a, 0x0550471e, 0xb8ed0826, 0x0ff0c922, 0xd6d68a2f, 0x61cb4b2b,
      0x649b0c35, 0xd386cd31, 0x0aa08e3c, 0xbdbd4f38, 0x70db114c, 0xc7c6d048,
      0x1ee09345, 0xa9fd5241, 0xacad155f, 0x1bb0d45b, 0xc2969756, 0x758b5652,
      0xc836196a, 0x7f2bd86e, 0xa60d9b63, 0x11105a67, 0x14401d79, 0xa35ddc7d,
      0x7a7b9f70, 0xcd665e74, 0xe0b62398, 0x57abe29c, 0x8e8da191, 0x39906095,
      0x3cc0278b, 0x8bdde68f, 0x52fba582, 0xe5e66486, 0x585b2bbe, 0xef46eaba,
      0x3660a9b7, 0x817d68b3, 0x842d2fad, 0x3330eea9, 0xea16ada4, 0x5d0b6ca0,
      0x906d32d4, 0x2770f3d0, 0xfe56b0dd, 0x494b71d9, 0x4c1b36c7, 0xfb06f7c3,
      0x2220b4ce, 0x953d75ca, 0x28803af2, 0x9f9dfbf6, 0x46bbb8fb, 0xf1a679ff,
      0xf4f63ee1, 0x43ebffe5, 0x9acdbce8, 0x2dd07dec, 0x77708634, 0xc06d4730,
      0x194b043d, 0xae56c539, 0xab068227, 0x1c1b4323, 0xc53d002e, 0x7220c12a,
      0xcf9d8e12, 0x78804f16, 0xa1a60c1b, 0x16bbcd1f, 0x13eb8a01, 0xa4f64b05,
      0x7dd00808, 0xcacdc90c, 0x07ab9778, 0xb0b6567c, 0x69901571, 0xde8dd475,
      0xdbdd936b, 0x6cc0526f, 0xb5e61162, 0x02fbd066, 0xbf469f5e, 0x085b5e5a,
      0xd17d1d57, 0x6660dc53, 0x63309b4d, 0xd42d5a49, 0x0d0b1944, 0xba16d840,
      0x97c6a5ac, 0x20db64a8, 0xf9fd27a5, 0x4ee0e6a1, 0x4bb0a1bf, 0xfcad60bb,
      0x258b23b6, 0x9296e2b2, 0x2f2bad8a, 0x98366c8e, 0x41102f83, 0xf60dee87,
      0xf35da999, 0x4440689d, 0x9d662b90, 0x2a7bea94, 0xe71db4e0, 0x500075e4,
      0x892636e9, 0x3e3bf7ed, 0x3b6bb0f3, 0x8c7671f7, 0x555032fa, 0xe24df3fe,
      0x5ff0bcc6, 0xe8ed7dc2, 0x31cb3ecf, 0x86d6ffcb, 0x8386b8d5, 0x349b79d1,
      0xedbd3adc, 0x5aa0fbd8, 0xeee00c69, 0x59fdcd6d, 0x80db8e60, 0x37c64f64,
      0x3296087a, 0x858bc97e, 0x5cad8a73, 0xebb04b77, 0x560d044f, 0xe110c54b,
      0x38368646, 0x8f2b4742, 0x8a7b005c, 0x3d66c158, 0xe4408255, 0x535d4351,
      0x9e3b1d25, 0x2926dc21, 0xf0009f2c, 0x471d5e28, 0x424d1936, 0xf550d832,
      0x2c769b3f, 0x9b6b5a3b, 0x26d61503, 0x91cbd407, 0x48ed970a, 0xfff0560e,
      0xfaa01110, 0x4dbdd014, 0x949b9319, 0x2386521d, 0x0e562ff1, 0xb94beef5,
      0x606dadf8, 0xd7706cfc, 0xd2202be2, 0x653deae6, 0xbc1ba9eb, 0x0b0668ef,
      0xb6bb27d7, 0x01a6e6d3, 0xd880a5de, 0x6f9d64da, 0x6acd23c4, 0xddd0e2c0,
      0x04f6a1cd, 0xb3eb60c9, 0x7e8d3ebd, 0xc990ffb9, 0x10b6bcb4, 0xa7ab7db0,
      0xa2fb3aae, 0x15e6fbaa, 0xccc0b8a7, 0x7bdd79a3, 0xc660369b, 0x717df79f,
      0xa85bb492, 0x1f467596, 0x1a163288, 0xad0bf38c, 0x742db081, 0xc3307185,
      0x99908a5d, 0x2e8d4b59, 0xf7ab0854, 0x40b6c950, 0x45e68e4e, 0xf2fb4f4a,
      0x2bdd0c47, 0x9cc0cd43, 0x217d827b, 0x9660437f, 0x4f460072, 0xf85bc176,
      0xfd0b8668, 0x4a16476c, 0x93300461, 0x242dc565, 0xe94b9b11, 0x5e565a15,
      0x87701918, 0x306dd81c, 0x353d9f02, 0x82205e06, 0x5b061d0b, 0xec1bdc0f,
      0x51a69337, 0xe6bb5233, 0x3f9d113e, 0x8880d03a, 0x8dd09724, 0x3acd5620,
      0xe3eb152d, 0x54f6d429, 0x7926a9c5, 0xce3b68c1, 0x171d2bcc, 0xa000eac8,
      0xa550add6, 0x124d6cd2, 0xcb6b2fdf, 0x7c76eedb, 0xc1cba1e3, 0x76d660e7,
      0xaff023ea, 0x18ede2ee, 0x1dbda5f0, 0xaaa064f4, 0x738627f9, 0xc49be6fd,
      0x09fdb889, 0xbee0798d, 0x67c63a80, 0xd0dbfb84, 0xd58bbc9a, 0x62967d9e,
      0xbbb03e93, 0x0cadff97, 0xb110b0af, 0x060d71ab, 0xdf2b32a6, 0x6836f3a2,
      0x6d66b4bc, 0xda7b75b8, 0x035d36b5, 0xb440f7b1 },
    { 0x00000000, 0xdcc119d2, 0x0f9ef2a0, 0xd35feb72, 0xa9212445, 0x75e03d97,
      0xa6bfd6e5, 0x7a7ecf37, 0x5243488a, 0x8e825158, 0x5dddba2a, 0x811ca3f8,
      0xfb626ccf, 0x27a3751d, 0xf4fc9e6f, 0x283d87bd, 0x139b5110, 0xcf5a48c2,
      0x1c05a3b0, 0xc0c4ba62, 0xbaba7555, 0x667b6c87, 0xb52487f5, 0x69e59e27,
      0x41d8199a, 0x9d190048, 0x4e46eb3a, 0x9287f2e8, 0xe8f93ddf, 0x3438240d,
      0xe767cf7f, 0x3ba6d6ad, 0x2636a320, 0xfaf7baf2, 0x29a85180, 0xf5694852,
      0x8f178765, 0x53d69eb7, 0x808975c5, 0x5c486c17, 0x7475ebaa, 0xa8b4f278,
      0x7beb190a, 0xa72a00d8, 0xdd54cfef, 0x0195d63d, 0xd2ca3d4f, 0x0e0b249d,
      0x35adf230, 0xe96cebe2, 0x3a330090, 0xe6f21942, 0x9c8cd675, 0x404dcfa7,
      0x931224d5, 0x4fd33d07, 0x67eebaba, 0xbb2fa368, 0x6870481a, 0xb4b151c8,
      0xcecf9eff, 0x120e872d, 0xc1516c5f, 0x1d90758d, 0x4c6c4641, 0x90ad5f93,
      0x43f2b4e1, 0x9f33ad33, 0xe54d6204, 0x398c7bd6, 0xead390a4, 0x36128976,
      0x1e2f0ecb, 0xc2ee1719, 0x11b1fc6b, 0xcd70e5b9, 0xb70e2a8e, 0x6bcf335c,
      0xb890d82e, 0x6451c1fc, 0x5ff71751, 0x83360e83, 0x5069e5f1, 0x8ca8fc23,
      0xf6d63314, 0x2a172ac6, 0xf948c1b4, 0x2589d866, 0x0db45fdb, 0xd1754609,
      0x022aad7b, 0xdeebb4a9, 0xa4957b9e, 0x7854624c, 0xab0b893e, 0x77ca90ec,
      0x6a5ae561, 0xb69bfcb3, 0x65c417c1, 0xb9050e13, 0xc37bc124, 0x1fbad8f6,
      0xcce53384, 0x10242a56, 0x3819adeb, 0xe4d8b439, 0x37875f4b, 0xeb464699,
      0x913889ae, 0x4df9907c, 0x9ea67b0e, 0x426762dc, 0x79c1b471, 0xa500ada3,
      0x765f46d1, 0xaa9e5f03, 0xd0e09034, 0x0c2189e6, 0xdf7e6294, 0x03bf7b46,
      0x2b82fcfb, 0xf743e529, 0x241c0e5b, 0xf8dd1789, 0x82a3d8be, 0x5e62c16c,
      0x8d3d2a1e, 0x51fc33cc, 0x98d88c82, 0x44199550, 0x97467e22, 0x4b8767f0,
      0x31f9a8c7, 0xed38b115, 0x3e675a67, 0xe2a643b5, 0xca9bc408, 0x165addda,
      0xc50536a8, 0x19c42f7a, 0x63bae04d, 0xbf7bf99f, 0x6c2412ed, 0xb0e50b3f,
      0x8b43dd92, 0x5782c440, 0x84dd2f32, 0x581c36e0, 0x2262f9d7, 0xfea3e005,
      0x2dfc0b77, 0xf13d12a5, 0xd9009518, 0x05c18cca, 0xd69e67b8, 0x0a5f7e6a,
      0x7021b15d, 0xace0a88f, 0x7fbf43fd, 0xa37e5a2f, 0xbeee2fa2, 0x622f3670,
      0xb170dd02, 0x6db1c4d0, 0x17cf0be7, 0xcb0e1235, 0x1851f947, 0xc490e095,
      0xecad6728, 0x306c7efa, 0xe3339588, 0x3ff28c5a, 0x458c436d, 0x994d5abf,
      0x4a12b1cd, 0x96d3a81f, 0xad757eb2, 0x71b46760, 0xa2eb8c12, 0x7e2a95c0,
      0x04545af7, 0xd8954325, 0x0bcaa857, 0xd70bb185, 0xff363638, 0x23f72fea,
      0xf0a8c498, 0x2c69dd4a, 0x5617127d, 0x8ad60baf, 0x5989e0dd, 0x8548f90f,
      0xd4b4cac3, 0x0875d311, 0xdb2a3863, 0x07eb21b1, 0x7d95ee86, 0xa154f754,
      0x720b1c26, 0xaeca05f4, 0x86f78249, 0x5a369b9b, 0x896970e9, 0x55a8693b,
      0x2fd6a60c, 0xf317bfde, 0x204854ac, 0xfc894d7e, 0xc72f9bd3, 0x1bee8201,
      0xc8b16973, 0x147070a1, 0x6e0ebf96, 0xb2cfa644, 0x61904d36, 0xbd5154e4,
      0x956cd359, 0x49adca8b, 0x9af221f9, 0x4633382b, 0x3c4df71c, 0xe08ceece,
      0x33d305bc, 0xef121c6e, 0xf28269e3, 0x2e437031, 0xfd1c9b43, 0x21dd8291,
      0x5ba34da6, 0x87625474, 0x543dbf06, 0x88fca6d4, 0xa0c12169, 0x7c0038bb,
      0xaf5fd3c9, 0x739eca1b, 0x09e0052c, 0xd5211cfe, 0x067ef78c, 0xdabfee5e,
      0xe11938f3, 0x3dd82121, 0xee87ca53, 0x3246d381, 0x48381cb6, 0x94f90564,
      0x47a6ee16, 0x9b67f7c4, 0xb35a7079, 0x6f9b69ab, 0xbcc482d9, 0x60059b0b,
      0x1a7b543c, 0xc6ba4dee, 0x15e5a69c, 0xc924bf4e },
    { 0x00000000, 0x87acd801, 0x0e59b103, 0x89f56902, 0x1cb26207, 0x9b1eba06,
      0x12ebd304, 0x95470b05, 0x3864c50e, 0xbfc81d0f, 0x363d740d, 0xb191ac0c,
      0x24d6a709, 0xa37a7f08, 0x2a8f160a, 0xad23ce0b, 0x70c88a1d, 0xf764521c,
      0x7e913b1e, 0xf93de31f, 0x6c7ae81a, 0xebd6301b, 0x62235919, 0xe58f8118,
      0x48ac4f13, 0xcf009712, 0x46f5fe10, 0xc1592611, 0x541e2d14, 0xd3b2f515,
      0x5a479c17, 0xddeb4416, 0xe090153b, 0x673ccd3a, 0xeec9a438, 0x69657c39,
      0xfc22773c, 0x7b8eaf3d, 0xf27bc63f, 0x75d71e3e, 0xd8f4d035, 0x5f580834,
      0xd6ad6136, 0x5101b937, 0xc446b232, 0x43ea6a33, 0xca1f0331, 0x4db3db30,
      0x90589f26, 0x17f44727, 0x9e012e25, 0x19adf624, 0x8ceafd21, 0x0b462520,
      0x82b34c22, 0x051f9423, 0xa83c5a28, 0x2f908229, 0xa665eb2b, 0x21c9332a,
      0xb48e382f, 0x3322e02e, 0xbad7892c, 0x3d7b512d, 0xc0212b76, 0x478df377,
      0xce789a75, 0x49d44274, 0xdc934971, 0x5b3f9170, 0xd2caf872, 0x55662073,
      0xf845ee78, 0x7fe93679, 0xf61c5f7b, 0x71b0877a, 0xe4f78c7f, 0x635b547e,
      0xeaae3d7c, 0x6d02e57d, 0xb0e9a16b, 0x3745796a, 0xbeb01068, 0x391cc869,
      0xac5bc36c, 0x2bf71b6d, 0xa202726f, 0x25aeaa6e, 0x888d6465, 0x0f21bc64,
      0x86d4d566, 0x01780d67, 0x943f0662, 0x1393de63, 0x9a66b761, 0x1dca6f60,
      0x20b13e4d, 0xa71de64c, 0x2ee88f4e, 0xa944574f, 0x3c035c4a, 0xbbaf844b,
      0x325aed49, 0xb5f63548, 0x18d5fb43, 0x9f792342, 0x168c4a40, 0x91209241,
      0x04679944, 0x83cb4145, 0x0a3e2847, 0x8d92f046, 0x5079b450, 0xd7d56c51,
      0x5e200553, 0xd98cdd52, 0x4ccbd657, 0xcb670e56, 0x42926754, 0xc53ebf55,
      0x681d715e, 0xefb1a95f, 0x6644c05d, 0xe1e8185c, 0x74af1359, 0xf303cb58,
      0x7af6a25a, 0xfd5a7a5b, 0x804356ec, 0x07ef8eed, 0x8e1ae7ef, 0x09b63fee,
      0x9cf134eb, 0x1b5decea, 0x92a885e8, 0x15045de9, 0xb82793e2, 0x3f8b4be3,
      0xb67e22e1, 0x31d2fae0, 0xa495f1e5, 0x233929e4, 0xaacc40e6, 0x2d6098e7,
      0xf08bdcf1, 0x772704f0, 0xfed26df2, 0x797eb5f3, 0xec39bef6, 0x6b9566f7,
      0xe2600ff5, 0x65ccd7f4, 0xc8ef19ff, 0x4f43c1fe, 0xc6b6a8fc, 0x411a70fd,
      0xd45d7bf8, 0x53f1a3f9, 0xda04cafb, 0x5da812fa, 0x60d343d7, 0xe77f9bd6,
      0x6e8af2d4, 0xe9262ad5, 0x7c6121d0, 0xfbcdf9d1, 0x723890d3, 0xf59448d2,
      0x58b786d9, 0xdf1b5ed8, 0x56ee37da, 0xd142efdb, 0x4405e4de, 0xc3a93cdf,
      0x4a5c55dd, 0xcdf08ddc, 0x101bc9ca, 0x97b711cb, 0x1e4278c9, 0x99eea0c8,
      0x0ca9abcd, 0x8b0573cc, 0x02f01ace, 0x855cc2cf, 0x287f0cc4, 0xafd3d4c5,
      0x2626bdc7, 0xa18a65c6, 0x34cd6ec3, 0xb361b6c2, 0x3a94dfc0, 0xbd3807c1,
      0x40627d9a, 0xc7cea59b, 0x4e3bcc99, 0xc9971498, 0x5cd01f9d, 0xdb7cc79c,
      0x5289ae9e, 0xd525769f, 0x7806b894, 0xffaa6095, 0x765f0997, 0xf1f3d196,
      0x64b4da93, 0xe3180292, 0x6aed6b90, 0xed41b391, 0x30aaf787, 0xb7062f86,
      0x3ef34684, 0xb95f9e85, 0x2c189580, 0xabb44d81, 0x22412483, 0xa5edfc82,
      0x08ce3289, 0x8f62ea88, 0x0697838a, 0x813b5b8b, 0x147c508e, 0x93d0888f,
      0x1a25e18d, 0x9d89398c, 0xa0f268a1, 0x275eb0a0, 0xaeabd9a2, 0x290701a3,
      0xbc400aa6, 0x3becd2a7, 0xb219bba5, 0x35b563a4, 0x9896adaf, 0x1f3a75ae,
      0x96cf1cac, 0x1163c4ad, 0x8424cfa8, 0x038817a9, 0x8a7d7eab, 0x0dd1a6aa,
      0xd03ae2bc, 0x57963abd, 0xde6353bf, 0x59cf8bbe, 0xcc8880bb, 0x4b2458ba,
      0xc2d131b8, 0x457de9b9, 0xe85e27b2, 0x6ff2ffb3, 0xe60796b1, 0x61ab4eb0,
      0xf4ec45b5, 0x73409db4, 0xfab5f4b6, 0x7d192cb7 },
    { 0x00000000, 0xb79a6ddc, 0xd9281abc, 0x6eb27760, 0x054cf57c, 0xb2d698a0,
      0xdc64efc0, 0x6bfe821c, 0x0a98eaf9, 0xbd028725, 0xd3b0f045, 0x642a9d99,
      0x0fd41f85, 0xb84e7259, 0xd6fc0539, 0x616668e5, 0xa32d14f7, 0x14b7792b,
      0x7a050e4b, 0xcd9f6397, 0xa661e18b, 0x11fb8c57, 0x7f49fb37, 0xc8d396eb,
      0xa9b5fe0e, 0x1e2f93d2, 0x709de4b2, 0xc707896e, 0xacf90b72, 0x1b6366ae,
      0x75d111ce, 0xc24b7c12, 0xf146e9ea, 0x46dc8436, 0x286ef356, 0x9ff49e8a,
      0xf40a1c96, 0x4390714a, 0x2d22062a, 0x9ab86bf6, 0xfbde0313, 0x4c446ecf,
      0x22f619af, 0x956c7473, 0xfe92f66f, 0x49089bb3, 0x27baecd3, 0x9020810f,
      0x526bfd1d, 0xe5f190c1, 0x8b43e7a1, 0x3cd98a7d, 0x57270861, 0xe0bd65bd,
      0x8e0f12dd, 0x39957f01, 0x58f317e4, 0xef697a38, 0x81db0d58, 0x36416084,
      0x5dbfe298, 0xea258f44, 0x8497f824, 0x330d95f8, 0x559013d1, 0xe20a7e0d,
      0x8cb8096d, 0x3b2264b1, 0x50dce6ad, 0xe7468b71, 0x89f4fc11, 0x3e6e91cd,
      0x5f08f928, 0xe89294f4, 0x8620e394, 0x31ba8e48, 0x5a440c54, 0xedde6188,
      0x836c16e8, 0x34f67b34, 0xf6bd0726, 0x41276afa, 0x2f951d9a, 0x980f7046,
      0xf3f1f25a, 0x446b9f86, 0x2ad9e8e6, 0x9d43853a, 0xfc25eddf, 0x4bbf8003,
      0x250df763, 0x92979abf, 0xf96918a3, 0x4ef3757f, 0x2041021f, 0x97db6fc3,
      0xa4d6fa3b, 0x134c97e7, 0x7dfee087, 0xca648d5b, 0xa19a0f47, 0x1600629b,
      0x78b215fb, 0xcf287827, 0xae4e10c2, 0x19d47d1e, 0x77660a7e, 0xc0fc67a2,
      0xab02e5be, 0x1c988862, 0x722aff02, 0xc5b092de, 0x07fbeecc, 0xb0618310,
      0xded3f470, 0x694999ac, 0x02b71bb0, 0xb52d766c, 0xdb9f010c, 0x6c056cd0,
      0x0d630435, 0xbaf969e9, 0xd44b1e89, 0x63d17355, 0x082ff149, 0xbfb59c95,
      0xd107ebf5, 0x669d8629, 0x1d3de6a6, 0xaaa78b7a, 0xc415fc1a, 0x738f91c6,
      0x187113da, 0xafeb7e06, 0xc1590966, 0x76c364ba, 0x17a50c5f, 0xa03f6183,
      0xce8d16e3, 0x79177b3f, 0x12e9f923, 0xa57394ff, 0xcbc1e39f, 0x7c5b8e43,
      0xbe10f251, 0x098a9f8d, 0x6738e8ed, 0xd0a28531, 0xbb5c072d, 0x0cc66af1,
      0x62741d91, 0xd5ee704d, 0xb48818a8, 0x03127574, 0x6da00214, 0xda3a6fc8,
      0xb1c4edd4, 0x065e8008, 0x68ecf768, 0xdf769ab4, 0xec7b0f4c, 0x5be16290,
      0x355315f0, 0x82c9782c, 0xe937fa30, 0x5ead97ec, 0x301fe08c, 0x87858d50,
      0xe6e3e5b5, 0x51798869, 0x3fcbff09, 0x885192d5, 0xe3af10c9, 0x54357d15,
      0x3a870a75, 0x8d1d67a9, 0x4f561bbb, 0xf8cc7667, 0x967e0107, 0x21e46cdb,
      0x4a1aeec7, 0xfd80831b, 0x9332f47b, 0x24a899a7, 0x45cef142, 0xf2549c9e,
      0x9ce6ebfe, 0x2b7c8622, 0x4082043e, 0xf71869e2, 0x99aa1e82, 0x2e30735e,
      0x48adf577, 0xff3798ab, 0x9185efcb, 0x261f8217, 0x4de1000b, 0xfa7b6dd7,
      0x94c91ab7, 0x2353776b, 0x42351f8e, 0xf5af7252, 0x9b1d0532, 0x2c8768ee,
      0x4779eaf2, 0xf0e3872e, 0x9e51f04e, 0x29cb9d92, 0xeb80e180, 0x5c1a8c5c,
      0x32a8fb3c, 0x853296e0, 0xeecc14fc, 0x59567920, 0x37e40e40, 0x807e639c,
      0xe1180b79, 0x568266a5, 0x383011c5, 0x8faa7c19, 0xe454fe05, 0x53ce93d9,
      0x3d7ce4b9, 0x8ae68965, 0xb9eb1c9d, 0x0e717141, 0x60c30621, 0xd7596bfd,
      0xbca7e9e1, 0x0b3d843d, 0x658ff35d, 0xd2159e81, 0xb373f664, 0x04e99bb8,
      0x6a5becd8, 0xddc18104, 0xb63f0318, 0x01a56ec4, 0x6f1719a4, 0xd88d7478,
      0x1ac6086a, 0xad5c65b6, 0xc3ee12d6, 0x74747f0a, 0x1f8afd16, 0xa81090ca,
      0xc6a2e7aa, 0x71388a76, 0x105ee293, 0xa7c48f4f, 0xc976f82f, 0x7eec95f3,
      0x151217ef, 0xa2887a33, 0xcc3a0d53, 0x7ba0608f },
    { 0x00000000, 0x8d670d49, 0x1acf1a92, 0x97a817db, 0x8383f420, 0x0ee4f969,
      0x994ceeb2, 0x142be3fb, 0x0607e941, 0x8b60e408, 0x1cc8f3d3, 0x91affe9a,
      0x85841d61, 0x08e31028, 0x9f4b07f3, 0x122c0aba, 0x0c0ed283, 0x8169dfca,
      0x16c1c811, 0x9ba6c558, 0x8f8d26a3, 0x02ea2bea, 0x95423c31, 0x18253178,
      0x0a093bc2, 0x876e368b, 0x10c62150, 0x9da12c19, 0x898acfe2, 0x04edc2ab,
      0x9345d570, 0x1e22d839, 0xaf016503, 0x2266684a, 0xb5ce7f91, 0x38a972d8,
      0x2c829123, 0xa1e59c6a, 0x364d8bb1, 0xbb2a86f8, 0xa9068c42, 0x2461810b,
      0xb3c996d0, 0x3eae9b99, 0x2a857862, 0xa7e2752b, 0x304a62f0, 0xbd2d6fb9,
      0xa30fb780, 0x2e68bac9, 0xb9c0ad12, 0x34a7a05b, 0x208c43a0, 0xadeb4ee9,
      0x3a435932, 0xb724547b, 0xa5085ec1, 0x286f5388, 0xbfc74453, 0x32a0491a,
      0x268baae1, 0xabeca7a8, 0x3c44b073, 0xb123bd3a, 0x5e03ca06, 0xd364c74f,
      0x44ccd094, 0xc9abdddd, 0xdd803e26, 0x50e7336f, 0xc74f24b4, 0x4a2829fd,
      0x58042347, 0xd5632e0e, 0x42cb39d5, 0xcfac349c, 0xdb87d767, 0x56e0da2e,
      0xc148cdf5, 0x4c2fc0bc, 0x520d1885, 0xdf6a15cc, 0x48c20217, 0xc5a50f5e,
      0xd18eeca5, 0x5ce9e1ec, 0xcb41f637, 0x4626fb7e, 0x540af1c4, 0xd96dfc8d,
      0x4ec5eb56, 0xc3a2e61f, 0xd78905e4, 0x5aee08ad, 0xcd461f76, 0x4021123f,
      0xf102af05, 0x7c65a24c, 0xebcdb597, 0x66aab8de, 0x72815b25, 0xffe6566c,
      0x684e41b7, 0xe5294cfe, 0xf7054644, 0x7a624b0d, 0xedca5cd6, 0x60ad519f,
      0x7486b264, 0xf9e1bf2d, 0x6e49a8f6, 0xe32ea5bf, 0xfd0c7d86, 0x706b70cf,
      0xe7c36714, 0x6aa46a5d, 0x7e8f89a6, 0xf3e884ef, 0x64409334, 0xe9279e7d,
      0xfb0b94c7, 0x766c998e, 0xe1c48e55, 0x6ca3831c, 0x788860e7, 0xf5ef6dae,
      0x62477a75, 0xef20773c, 0xbc06940d, 0x31619944, 0xa6c98e9f, 0x2bae83d6,
      0x3f85602d, 0xb2e26d64, 0x254a7abf, 0xa82d77f6, 0xba017d4c, 0x37667005,
      0xa0ce67de, 0x2da96a97, 0x3982896c, 0xb4e58425, 0x234d93fe, 0xae2a9eb7,
      0xb008468e, 0x3d6f4bc7, 0xaac75c1c, 0x27a05155, 0x338bb2ae, 0xbeecbfe7,
      0x2944a83c, 0xa423a575, 0xb60fafcf, 0x3b68a286, 0xacc0b55d, 0x21a7b814,
      0x358c5bef, 0xb8eb56a6, 0x2f43417d, 0xa2244c34, 0x1307f10e, 0x9e60fc47,
      0x09c8eb9c, 0x84afe6d5, 0x9084052e, 0x1de30867, 0x8a4b1fbc, 0x072c12f5,
      0x1500184f, 0x98671506, 0x0fcf02dd, 0x82a80f94, 0x9683ec6f, 0x1be4e126,
      0x8c4cf6fd, 0x012bfbb4, 0x1f09238d, 0x926e2ec4, 0x05c6391f, 0x88a13456,
      0x9c8ad7ad, 0x11eddae4, 0x8645cd3f, 0x0b22c076, 0x190ecacc, 0x9469c785,
      0x03c1d05e, 0x8ea6dd17, 0x9a8d3eec, 0x17ea33a5, 0x8042247e, 0x0d252937,
      0xe2055e0b, 0x6f625342, 0xf8ca4499, 0x75ad49d0, 0x6186aa2b, 0xece1a762,
      0x7b49b0b9, 0xf62ebdf0, 0xe402b74a, 0x6965ba03, 0xfecdadd8, 0x73aaa091,
      0x6781436a, 0xeae64e23, 0x7d4e59f8, 0xf02954b1, 0xee0b8c88, 0x636c81c1,
      0xf4c4961a, 0x79a39b53, 0x6d8878a8, 0xe0ef75e1, 0x7747623a, 0xfa206f73,
      0xe80c65c9, 0x656b6880, 0xf2c37f5b, 0x7fa47212, 0x6b8f91e9, 0xe6e89ca0,
      0x71408b7b, 0xfc278632, 0x4d043b08, 0xc0633641, 0x57cb219a, 0xdaac2cd3,
      0xce87cf28, 0x43e0c261, 0xd448d5ba, 0x592fd8f3, 0x4b03d249, 0xc664df00,
      0x51ccc8db, 0xdcabc592, 0xc8802669, 0x45e72b20, 0xd24f3cfb, 0x5f2831b2,
      0x410ae98b, 0xcc6de4c2, 0x5bc5f319, 0xd6a2fe50, 0xc2891dab, 0x4fee10e2,
      0xd8460739, 0x55210a70, 0x470d00ca, 0xca6a0d83, 0x5dc21a58, 0xd0a51711,
      0xc48ef4ea, 0x49e9f9a3, 0xde41ee78, 0x5326e331 },
    { 0x00000000, 0x780d281b, 0xf01a5036, 0x8817782d, 0xe035a06c, 0x98388877,
      0x102ff05a, 0x6822d841, 0xc06b40d9, 0xb86668c2, 0x307110ef, 0x487c38f4,
      0x205ee0b5, 0x5853c8ae, 0xd044b083, 0xa8499898, 0x37ca41b6, 0x4fc769ad,
      0xc7d01180, 0xbfdd399b, 0xd7ffe1da, 0xaff2c9c1, 0x27e5b1ec, 0x5fe899f7,
      0xf7a1016f, 0x8fac2974, 0x07bb5159, 0x7fb67942, 0x1794a103, 0x6f998918,
      0xe78ef135, 0x9f83d92e, 0xd9894268, 0xa1846a73, 0x2993125e, 0x519e3a45,
      0x39bce204, 0x41b1ca1f, 0xc9a6b232, 0xb1ab9a29, 0x19e202b1, 0x61ef2aaa,
      0xe9f85287, 0x91f57a9c, 0xf9d7a2dd, 0x81da8ac6, 0x09cdf2eb, 0x71c0daf0,
      0xee4303de, 0x964e2bc5, 0x1e5953e8, 0x66547bf3, 0x0e76a3b2, 0x767b8ba9,
      0xfe6cf384, 0x8661db9f, 0x2e284307, 0x56256b1c, 0xde321331, 0xa63f3b2a,
      0xce1de36b, 0xb610cb70, 0x3e07b35d, 0x460a9b46, 0xb21385d0, 0xca1eadcb,
      0x4209d5e6, 0x3a04fdfd, 0x522625bc, 0x2a2b0da7, 0xa23c758a, 0xda315d91,
      0x7278c509, 0x0a75ed12, 0x8262953f, 0xfa6fbd24, 0x924d6565, 0xea404d7e,
      0x62573553, 0x1a5a1d48, 0x85d9c466, 0xfdd4ec7d, 0x75c39450, 0x0dcebc4b,
      0x65ec640a, 0x1de14c11, 0x95f6343c, 0xedfb1c27, 0x45b284bf, 0x3dbfaca4,
      0xb5a8d489, 0xcda5fc92, 0xa58724d3, 0xdd8a0cc8, 0x559d74e5, 0x2d905cfe,
      0x6b9ac7b8, 0x1397efa3, 0x9b80978e, 0xe38dbf95, 0x8baf67d4, 0xf3a24fcf,
      0x7bb537e2, 0x03b81ff9, 0xabf18761, 0xd3fcaf7a, 0x5bebd757, 0x23e6ff4c,
      0x4bc4270d, 0x33c90f16, 0xbbde773b, 0xc3d35f20, 0x5c50860e, 0x245dae15,
      0xac4ad638, 0xd447fe23, 0xbc652662, 0xc4680e79, 0x4c7f7654, 0x34725e4f,
      0x9c3bc6d7, 0xe436eecc, 0x6c2196e1, 0x142cbefa, 0x7c0e66bb, 0x04034ea0,
      0x8c14368d, 0xf4191e96, 0xd33acba5, 0xab37e3be, 0x23209b93, 0x5b2db388,
      0x330f6bc9, 0x4b0243d2, 0xc3153bff, 0xbb1813e4, 0x13518b7c, 0x6b5ca367,
      0xe34bdb4a, 0x9b46f351, 0xf3642b10, 0x8b69030b, 0x037e7b26, 0x7b73533d,
      0xe4f08a13, 0x9cfda208, 0x14eada25, 0x6ce7f23e, 0x04c52a7f, 0x7cc80264,
      0xf4df7a49, 0x8cd25252, 0x249bcaca, 0x5c96e2d1, 0xd4819afc, 0xac8cb2e7,
      0xc4ae6aa6, 0xbca342bd, 0x34b43a90, 0x4cb9128b, 0x0ab389cd, 0x72bea1d6,
      0xfaa9d9fb, 0x82a4f1e0, 0xea8629a1, 0x928b01ba, 0x1a9c7997, 0x6291518c,
      0xcad8c914, 0xb2d5e10f, 0x3ac29922, 0x42cfb139, 0x2aed6978, 0x52e04163,
      0xdaf7394e, 0xa2fa1155, 0x3d79c87b, 0x4574e060, 0xcd63984d, 0xb56eb056,
      0xdd4c6817, 0xa541400c, 0x2d563821, 0x555b103a, 0xfd1288a2, 0x851fa0b9,
      0x0d08d894, 0x7505f08f, 0x1d2728ce, 0x652a00d5, 0xed3d78f8, 0x953050e3,
      0x61294e75, 0x1924666e, 0x91331e43, 0xe93e3658, 0x811cee19, 0xf911c602,
      0x7106be2f, 0x090b9634, 0xa1420eac, 0xd94f26b7, 0x51585e9a, 0x29557681,
      0x4177aec0, 0x397a86db, 0xb16dfef6, 0xc960d6ed, 0x56e30fc3, 0x2eee27d8,
      0xa6f95ff5, 0xdef477ee, 0xb6d6afaf, 0xcedb87b4, 0x46ccff99, 0x3ec1d782,
      0x96884f1a, 0xee856701, 0x66921f2c, 0x1e9f3737, 0x76bdef76, 0x0eb0c76d,
      0x86a7bf40, 0xfeaa975b, 0xb8a00c1d, 0xc0ad2406, 0x48ba5c2b, 0x30b77430,
      0x5895ac71, 0x2098846a, 0xa88ffc47, 0xd082d45c, 0x78cb4cc4, 0x00c664df,
      0x88d11cf2, 0xf0dc34e9, 0x98feeca8, 0xe0f3c4b3, 0x68e4bc9e, 0x10e99485,
      0x8f6a4dab, 0xf76765b0, 0x7f701d9d, 0x077d3586, 0x6f5fedc7, 0x1752c5dc,
      0x9f45bdf1, 0xe74895ea, 0x4f010d72, 0x370c2569, 0xbf1b5d44, 0xc716755f,
      0xaf34ad1e, 0xd7398505, 0x5f2efd28, 0x2723d533 },
    { 0x00000000, 0x1168574f, 0x22d0ae9e, 0x33b8f9d1, 0xf3bd9c39, 0xe2d5cb76,
      0xd16d32a7, 0xc00565e8, 0xe67b3973, 0xf7136e3c, 0xc4ab97ed, 0xd5c3c0a2,
      0x15c6a54a, 0x04aef205, 0x37160bd4, 0x267e5c9b, 0xccf772e6, 0xdd9f25a9,
      0xee27dc78, 0xff4f8b37, 0x3f4aeedf, 0x2e22b990, 0x1d9a4041, 0x0cf2170e,
      0x2a8c4b95, 0x3be41cda, 0x085ce50b, 0x1934b244, 0xd931d7ac, 0xc85980e3,
      0xfbe17932, 0xea892e7d, 0x2ff224c8, 0x3e9a7387, 0x0d228a56, 0x1c4add19,
      0xdc4fb8f1, 0xcd27efbe, 0xfe9f166f, 0xeff74120, 0xc9891dbb, 0xd8e14af4,
      0xeb59b325, 0xfa31e46a, 0x3a348182, 0x2b5cd6cd, 0x18e42f1c, 0x098c7853,
      0xe305562e, 0xf26d0161, 0xc1d5f8b0, 0xd0bdafff, 0x10b8ca17, 0x01d09d58,
      0x32686489, 0x230033c6, 0x057e6f5d, 0x14163812, 0x27aec1c3, 0x36c6968c,
      0xf6c3f364, 0xe7aba42b, 0xd4135dfa, 0xc57b0ab5, 0xe9f98894, 0xf891dfdb,
      0xcb29260a, 0xda417145, 0x1a4414ad, 0x0b2c43e2, 0x3894ba33, 0x29fced7c,
      0x0f82b1e7, 0x1eeae6a8, 0x2d521f79, 0x3c3a4836, 0xfc3f2dde, 0xed577a91,
      0xdeef8340, 0xcf87d40f, 0x250efa72, 0x3466ad3d, 0x07de54ec, 0x16b603a3,
      0xd6b3664b, 0xc7db3104, 0xf463c8d5, 0xe50b9f9a, 0xc375c301, 0xd21d944e,
      0xe1a56d9f, 0xf0cd3ad0, 0x30c85f38, 0x21a00877, 0x1218f1a6, 0x0370a6e9,
      0xc60bac5c, 0xd763fb13, 0xe4db02c2, 0xf5b3558d, 0x35b63065, 0x24de672a,
      0x17669efb, 0x060ec9b4, 0x2070952f, 0x3118c260, 0x02a03bb1, 0x13c86cfe,
      0xd3cd0916, 0xc2a55e59, 0xf11da788, 0xe075f0c7, 0x0afcdeba, 0x1b9489f5,
      0x282c7024, 0x3944276b, 0xf9414283, 0xe82915cc, 0xdb91ec1d, 0xcaf9bb52,
      0xec87e7c9, 0xfdefb086, 0xce574957, 0xdf3f1e18, 0x1f3a7bf0, 0x0e522cbf,
      0x3dead56e, 0x2c828221, 0x65eed02d, 0x74868762, 0x473e7eb3, 0x565629fc,
      0x96534c14, 0x873b1b5b, 0xb483e28a, 0xa5ebb5c5, 0x8395e95e, 0x92fdbe11,
      0xa14547c0, 0xb02d108f, 0x70287567, 0x61402228, 0x52f8dbf9, 0x43908cb6,
      0xa919a2cb, 0xb871f584, 0x8bc90c55, 0x9aa15b1a, 0x5aa43ef2, 0x4bcc69bd,
      0x7874906c, 0x691cc723, 0x4f629bb8, 0x5e0accf7, 0x6db23526, 0x7cda6269,
      0xbcdf0781, 0xadb750ce, 0x9e0fa91f, 0x8f67fe50, 0x4a1cf4e5, 0x5b74a3aa,
      0x68cc5a7b, 0x79a40d34, 0xb9a168dc, 0xa8c93f93, 0x9b71c642, 0x8a19910d,
      0xac67cd96, 0xbd0f9ad9, 0x8eb76308, 0x9fdf3447, 0x5fda51af, 0x4eb206e0,
      0x7d0aff31, 0x6c62a87e, 0x86eb8603, 0x9783d14c, 0xa43b289d, 0xb5537fd2,
      0x75561a3a, 0x643e4d75, 0x5786b4a4, 0x46eee3eb, 0x6090bf70, 0x71f8e83f,
      0x424011ee, 0x532846a1, 0x932d2349, 0x82457406, 0xb1fd8dd7, 0xa095da98,
      0x8c1758b9, 0x9d7f0ff6, 0xaec7f627, 0xbfafa168, 0x7faac480, 0x6ec293cf,
      0x5d7a6a1e, 0x4c123d51, 0x6a6c61ca, 0x7b043685, 0x48bccf54, 0x59d4981b,
      0x99d1fdf3, 0x88b9aabc, 0xbb01536d, 0xaa690422, 0x40e02a5f, 0x51887d10,
      0x623084c1, 0x7358d38e, 0xb35db666, 0xa235e129, 0x918d18f8, 0x80e54fb7,
      0xa69b132c, 0xb7f34463, 0x844bbdb2, 0x9523eafd, 0x55268f15, 0x444ed85a,
      0x77f6218b, 0x669e76c4, 0xa3e57c71, 0xb28d2b3e, 0x8135d2ef, 0x905d85a0,
      0x5058e048, 0x4130b707, 0x72884ed6, 0x63e01999, 0x459e4502, 0x54f6124d,
      0x674eeb9c, 0x7626bcd3, 0xb623d93b, 0xa74b8e74, 0x94f377a5, 0x859b20ea,
      0x6f120e97, 0x7e7a59d8, 0x4dc2a009, 0x5caaf746, 0x9caf92ae, 0x8dc7c5e1,
      0xbe7f3c30, 0xaf176b7f, 0x896937e4, 0x980160ab, 0xabb9997a, 0xbad1ce35,
      0x7ad4abdd, 0x6bbcfc92, 0x58040543, 0x496c520c },
    { 0x00000000, 0xcadca15b, 0x94b943b7, 0x5e65e2ec, 0x9f6e466a, 0x55b2e731,
      0x0bd705dd, 0xc10ba486, 0x3edd8cd4, 0xf4012d8f, 0xaa64cf63, 0x60b86e38,
      0xa1b3cabe, 0x6b6f6be5, 0x350a8909, 0xffd62852, 0xcba7d8ad, 0x017b79f6,
      0x5f1e9b1a, 0x95c23a41, 0x54c99ec7, 0x9e153f9c, 0xc070dd70, 0x0aac7c2b,
      0xf57a5479, 0x3fa6f522, 0x61c317ce, 0xab1fb695, 0x6a141213, 0xa0c8b348,
      0xfead51a4, 0x3471f0ff, 0x2152705f, 0xeb8ed104, 0xb5eb33e8, 0x7f3792b3,
      0xbe3c3635, 0x74e0976e, 0x2a857582, 0xe059d4d9, 0x1f8ffc8b, 0xd5535dd0,
      0x8b36bf3c, 0x41ea1e67, 0x80e1bae1, 0x4a3d1bba, 0x1458f956, 0xde84580d,
      0xeaf5a8f2, 0x202909a9, 0x7e4ceb45, 0xb4904a1e, 0x759bee98, 0xbf474fc3,
      0xe122ad2f, 0x2bfe0c74, 0xd4282426, 0x1ef4857d, 0x40916791, 0x8a4dc6ca,
      0x4b46624c, 0x819ac317, 0xdfff21fb, 0x152380a0, 0x42a4e0be, 0x887841e5,
      0xd61da309, 0x1cc10252, 0xddcaa6d4, 0x1716078f, 0x4973e563, 0x83af4438,
      0x7c796c6a, 0xb6a5cd31, 0xe8c02fdd, 0x221c8e86, 0xe3172a00, 0x29cb8b5b,
      0x77ae69b7, 0xbd72c8ec, 0x89033813, 0x43df9948, 0x1dba7ba4, 0xd766daff,
      0x166d7e79, 0xdcb1df22, 0x82d43dce, 0x48089c95, 0xb7deb4c7, 0x7d02159c,
      0x2367f770, 0xe9bb562b, 0x28b0f2ad, 0xe26c53f6, 0xbc09b11a, 0x76d51041,
      0x63f690e1, 0xa92a31ba, 0xf74fd356, 0x3d93720d, 0xfc98d68b, 0x364477d0,
      0x6821953c, 0xa2fd3467, 0x5d2b1c35, 0x97f7bd6e, 0xc9925f82, 0x034efed9,
      0xc2455a5f, 0x0899fb04, 0x56fc19e8, 0x9c20b8b3, 0xa851484c, 0x628de917,
      0x3ce80bfb, 0xf634aaa0, 0x373f0e26, 0xfde3af7d, 0xa3864d91, 0x695aecca,
      0x968cc498, 0x5c5065c3, 0x0235872f, 0xc8e92674, 0x09e282f2, 0xc33e23a9,
      0x9d5bc145, 0x5787601e, 0x33550079, 0xf989a122, 0xa7ec43ce, 0x6d30e295,
      0xac3b4613, 0x66e7e748, 0x388205a4, 0xf25ea4ff, 0x0d888cad, 0xc7542df6,
      0x9931cf1a, 0x53ed6e41, 0x92e6cac7, 0x583a6b9c, 0x065f8970, 0xcc83282b,
      0xf8f2d8d4, 0x322e798f, 0x6c4b9b63, 0xa6973a38, 0x679c9ebe, 0xad403fe5,
      0xf325dd09, 0x39f97c52, 0xc62f5400, 0x0cf3f55b, 0x529617b7, 0x984ab6ec,
      0x5941126a, 0x939db331, 0xcdf851dd, 0x0724f086, 0x12077026, 0xd8dbd17d,
      0x86be3391, 0x4c6292ca, 0x8d69364c, 0x47b59717, 0x19d075fb, 0xd30cd4a0,
      0x2cdafcf2, 0xe6065da9, 0xb863bf45, 0x72bf1e1e, 0xb3b4ba98, 0x79681bc3,
      0x270df92f, 0xedd15874, 0xd9a0a88b, 0x137c09d0, 0x4d19eb3c, 0x87c54a67,
      0x46ceeee1, 0x8c124fba, 0xd277ad56, 0x18ab0c0d, 0xe77d245f, 0x2da18504,
      0x73c467e8, 0xb918c6b3, 0x78136235, 0xb2cfc36e, 0xecaa2182, 0x267680d9,
      0x71f1e0c7, 0xbb2d419c, 0xe548a370, 0x2f94022b, 0xee9fa6ad, 0x244307f6,
      0x7a26e51a, 0xb0fa4441, 0x4f2c6c13, 0x85f0cd48, 0xdb952fa4, 0x11498eff,
      0xd0422a79, 0x1a9e8b22, 0x44fb69ce, 0x8e27c895, 0xba56386a, 0x708a9931,
      0x2eef7bdd, 0xe433da86, 0x25387e00, 0xefe4df5b, 0xb1813db7, 0x7b5d9cec,
      0x848bb4be, 0x4e5715e5, 0x1032f709, 0xdaee5652, 0x1be5f2d4, 0xd139538f,
      0x8f5cb163, 0x45801038, 0x50a39098, 0x9a7f31c3, 0xc41ad32f, 0x0ec67274,
      0xcfcdd6f2, 0x051177a9, 0x5b749545, 0x91a8341e, 0x6e7e1c4c, 0xa4a2bd17,
      0xfac75ffb, 0x301bfea0, 0xf1105a26, 0x3bccfb7d, 0x65a91991, 0xaf75b8ca,
      0x9b044835, 0x51d8e96e, 0x0fbd0b82, 0xc561aad9, 0x046a0e5f, 0xceb6af04,
      0x90d34de8, 0x5a0fecb3, 0xa5d9c4e1, 0x6f0565ba, 0x31608756, 0xfbbc260d,
      0x3ab7828b, 0xf06b23d0, 0xae0ec13c, 0x64d26067 }
};

#endif // CRC32_AUTO_GENREATE_TABLE

//======================================  Reverse a 4-byte integer
inline uint32_t
rev_int4u( uint32_t in )
{
    uint32_t out = 0;
    for ( uint32_t j = 0; j < sizeof( in ); j++ )
    {
        out <<= 8;
        out |= ( in & 0xff );
        in >>= 8;
    }
    return out;
}
#endif

//======================================  Reversed table for 8-byte slices
static void
crc32_init( void )
{
#ifdef USE_LITTLE_ENDIAN
#ifdef CRC32_AUTO_GENERATE_TABLE
    if ( !crctab_set )
    {
        for ( int i = 0; i < 256; i++ )
        {
            crctab_8byte[ 0 ][ i ] = rev_int4u( crctab[ i ] );
        }
        for ( int i = 0; i < 256; i++ )
        {
            crctab_8byte[ 1 ][ i ] = ( crctab_8byte[ 0 ][ i ] >> 8 ) ^
                crctab_8byte[ 0 ][ crctab_8byte[ 0 ][ i ] & 0xFF ];
            crctab_8byte[ 2 ][ i ] = ( crctab_8byte[ 1 ][ i ] >> 8 ) ^
                crctab_8byte[ 0 ][ crctab_8byte[ 1 ][ i ] & 0xFF ];
            crctab_8byte[ 3 ][ i ] = ( crctab_8byte[ 2 ][ i ] >> 8 ) ^
                crctab_8byte[ 0 ][ crctab_8byte[ 2 ][ i ] & 0xFF ];
            crctab_8byte[ 4 ][ i ] = ( crctab_8byte[ 3 ][ i ] >> 8 ) ^
                crctab_8byte[ 0 ][ crctab_8byte[ 3 ][ i ] & 0xFF ];
            crctab_8byte[ 5 ][ i ] = ( crctab_8byte[ 4 ][ i ] >> 8 ) ^
                crctab_8byte[ 0 ][ crctab_8byte[ 4 ][ i ] & 0xFF ];
            crctab_8byte[ 6 ][ i ] = ( crctab_8byte[ 5 ][ i ] >> 8 ) ^
                crctab_8byte[ 0 ][ crctab_8byte[ 5 ][ i ] & 0xFF ];
            crctab_8byte[ 7 ][ i ] = ( crctab_8byte[ 6 ][ i ] >> 8 ) ^
                crctab_8byte[ 0 ][ crctab_8byte[ 6 ][ i ] & 0xFF ];
        }
        crctab_set = true;
    }
#endif // CRC32_AUTO_GENERATE_TABLE
#endif // USE_LITTLE_ENDIAN
}

//======================================  Add checksum for 1 byte
inline uint32_t
crc32_1byte( uint32_t data, uint32_t crc )
{
#ifdef USE_LITTLE_ENDIAN
    return ( crc >> 8 ) ^ crctab_8byte[ 0 ][ ( crc ^ data ) & 0xFF ];
#else
    return ( crc << 8 ) ^ crctab[ ( ( crc >> 24 ) ^ data ) & 0xFF ];
#endif
}

//======================================  Add data to checksum
static uint32_t
crc32_calc( const void* buf, uint32_t len, uint32_t crc )
{
    const unsigned char* cp = reinterpret_cast< const unsigned char* >( buf );
#ifdef USE_LITTLE_ENDIAN
    //----------------------------------  Force int32 alignment
    while ( len && ( long( cp ) & 3 ) != 0 )
    {
        crc = crc32_1byte( *( cp++ ), crc );
        len--;
    }

    //----------------------------------  Iterate over 8-byte slieces
    const uint32_t* current = reinterpret_cast< const uint32_t* >( cp );
    while ( len >= 8 )
    {
        uint32_t one = *( current++ ) ^ crc;
        uint32_t two = *( current++ );
        crc = crctab_8byte[ 7 ][ one & 0xFF ] ^
            crctab_8byte[ 6 ][ ( one >> 8 ) & 0xFF ] ^
            crctab_8byte[ 5 ][ ( one >> 16 ) & 0xFF ] ^
            crctab_8byte[ 4 ][ one >> 24 ] ^ crctab_8byte[ 3 ][ two & 0xFF ] ^
            crctab_8byte[ 2 ][ ( two >> 8 ) & 0xFF ] ^
            crctab_8byte[ 1 ][ ( two >> 16 ) & 0xFF ] ^
            crctab_8byte[ 0 ][ two >> 24 ];
        len -= 8;
    }

    cp = reinterpret_cast< const unsigned char* >( current );
#endif
    while ( len-- )
    {
        crc = crc32_1byte( *( cp++ ), crc );
    }
    return crc;
}

//======================================  Finish checksum
static uint32_t
crc32_finish( uint32_t length, uint32_t crc )
{
    for ( ; length; length >>= 8 )
    {
        crc = crc32_1byte( length, crc );
    }
#ifdef USE_LITTLE_ENDIAN
    return ~rev_int4u( crc );
#else
    return ~crc & 0xFFFFFFFF;
#endif
}

typedef struct checksum_crc32
{
    uint32_t value;
    uint32_t length;
} checksum_crc32;

static void
checksum_crc32_init( checksum_crc32* csum )
{
    crc32_init( );
    checksum_crc32_reset( csum );
}

static void
checksum_crc32_add( checksum_crc32* csum, const void* buf, uint32_t length )
{
    csum->value = crc32_calc( buf, length, csum->value );
    csum->length += length;
}

static void
checksum_crc32_reset( checksum_crc32* csum )
{
    csum->value = 0;
    csum->length = 0;
}

static uint32_t
checksum_crc32_result( checksum_crc32* csum )
{
    return crc32_finish( csum->length, csum->value );
}

#ifdef CRC32_MAIN
int
main( int argc, char* argv[] )
{
    crc32_init( );
    std::cout << "static const uint32_t crctab_8byte[8][256] = {\n";
    for ( int i = 0; i < 8; ++i )
    {
        char* tab = "    ";
        std::cout << tab << "{\n";
        for ( int j = 0; j < 256; ++j )
        {
            const int ENTRIES_PER_LINE = 8;
            if ( j % ENTRIES_PER_LINE == 0 )
            {
                std::cout << tab << tab;
            }
            else
            {
                std::cout << " ";
            }
            std::cout << "0x" << std::hex << std::setw( 8 )
                      << std::setfill( '0' ) << crctab_8byte[ i ][ j ];
            if ( j != 255 )
            {
                std::cout << ",";
            }
            if ( j % ENTRIES_PER_LINE == ENTRIES_PER_LINE - 1 )
            {
                std::cout << "\n";
            }
        }
        std::cout << tab << "}" << ( i < 7 ? "," : "" ) << "\n";
    }
    std::cout << "};" << std::endl;
}
#endif // CRC32_MAIN
