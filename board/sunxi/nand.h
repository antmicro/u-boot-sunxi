#ifndef NAND_H

#define NAND_H

#define W32(a, b) (*(volatile unsigned int *)(a)) = b
#define R32(a) (*(volatile unsigned int *)(a))

#define PORTC_BASE                 0x01c20800
#define CCMU_BASE                  0x01c20000
#define NANDFLASHC_BASE            0x01c03000
#define DMAC_BASE                  0x01c02000

#define NANDFLASHC_CTL             0x00000000

#define NFC_CTL_EN                 (1 << 0)
#define NFC_CTL_RESET              (1 << 1)
#define NFC_CTL_RAM_METHOD         (1 << 14)

#define NANDFLASHC_ST              0x00000004
#define NANDFLASHC_INT             0x00000008
#define NANDFLASHC_TIMING_CTL      0x0000000C
#define NANDFLASHC_TIMING_CFG      0x00000010
#define NANDFLASHC_ADDR_LOW        0x00000014
#define NANDFLASHC_ADDR_HIGH       0x00000018
#define NANDFLASHC_SECTOR_NUM      0x0000001C
#define NANDFLASHC_CNT             0x00000020
#define NANDFLASHC_CMD             0x00000024
#define NANDFLASHC_RCMD_SET        0x00000028
#define NANDFLASHC_WCMD_SET        0x0000002C
#define NANDFLASHC_IO_DATA         0x00000030
#define NANDFLASHC_ECC_CTL         0x00000034

#define NFC_ECC_EN                 (1 << 0)
#define NFC_ECC_PIPELINE           (1 << 3)
#define NFC_ECC_EXCEPTION          (1 << 4)
#define NFC_ECC_BLOCK_SIZE         (1 << 5)
#define NFC_ECC_RANDOM_EN          (1 << 9)
#define NFC_ECC_RANDOM_DIRECTION   (1 << 10)

#define NANDFLASHC_ECC_ST          0x00000038
#define NANDFLASHC_DEBUG           0x0000003C
#define NANDFLASHC_ECC_CNT0        0x00000040
#define NANDFLASHC_ECC_CNT1        0x00000044
#define NANDFLASHC_ECC_CNT2        0x00000048
#define NANDFLASHC_ECC_CNT3        0x0000004C
#define NANDFLASHC_USER_DATA_BASE  0x00000050
#define NANDFLASHC_EFNAND_STATUS   0x00000090
#define NANDFLASHC_SPARE_AREA      0x000000A0
#define NANDFLASHC_PATTERN_ID      0x000000A4
#define NANDFLASHC_RAM0_BASE       0x00000400
#define NANDFLASHC_RAM1_BASE       0x00000800

#define NFC_SEND_ADR               (1 << 19)
#define NFC_ACCESS_DIR             (1 << 20)
#define NFC_DATA_TRANS             (1 << 21)
#define NFC_SEND_CMD1              (1 << 22)
#define NFC_WAIT_FLAG              (1 << 23)
#define NFC_SEND_CMD2              (1 << 24)
#define NFC_SEQ                    (1 << 25)
#define NFC_DATA_SWAP_METHOD       (1 << 26)
#define NFC_ROW_AUTO_INC           (1 << 27)
#define NFC_SEND_CMD3              (1 << 28)
#define NFC_SEND_CMD4              (1 << 29)

#define NFC_PAGE_CMD               (2 << 30)

#define SPL_WRITE_SIZE             CONFIG_CMD_SPL_WRITE_SIZE

/* random seed used by linux */
const uint16_t random_seed[128] = {
	0x2b75, 0x0bd0, 0x5ca3, 0x62d1, 0x1c93, 0x07e9, 0x2162, 0x3a72,
	0x0d67, 0x67f9, 0x1be7, 0x077d, 0x032f, 0x0dac, 0x2716, 0x2436,
	0x7922, 0x1510, 0x3860, 0x5287, 0x480f, 0x4252, 0x1789, 0x5a2d,
	0x2a49, 0x5e10, 0x437f, 0x4b4e, 0x2f45, 0x216e, 0x5cb7, 0x7130,
	0x2a3f, 0x60e4, 0x4dc9, 0x0ef0, 0x0f52, 0x1bb9, 0x6211, 0x7a56,
	0x226d, 0x4ea7, 0x6f36, 0x3692, 0x38bf, 0x0c62, 0x05eb, 0x4c55,
	0x60f4, 0x728c, 0x3b6f, 0x2037, 0x7f69, 0x0936, 0x651a, 0x4ceb,
	0x6218, 0x79f3, 0x383f, 0x18d9, 0x4f05, 0x5c82, 0x2912, 0x6f17,
	0x6856, 0x5938, 0x1007, 0x61ab, 0x3e7f, 0x57c2, 0x542f, 0x4f62,
	0x7454, 0x2eac, 0x7739, 0x42d4, 0x2f90, 0x435a, 0x2e52, 0x2064,
	0x637c, 0x66ad, 0x2c90, 0x0bad, 0x759c, 0x0029, 0x0986, 0x7126,
	0x1ca7, 0x1605, 0x386a, 0x27f5, 0x1380, 0x6d75, 0x24c3, 0x0f8e,
	0x2b7a, 0x1418, 0x1fd1, 0x7dc1, 0x2d8e, 0x43af, 0x2267, 0x7da3,
	0x4e3d, 0x1338, 0x50db, 0x454d, 0x764d, 0x40a3, 0x42e6, 0x262b,
	0x2d2e, 0x1aea, 0x2e17, 0x173d, 0x3a6e, 0x71bf, 0x25f9, 0x0a5d,
	0x7c57, 0x0fbe, 0x46ce, 0x4939, 0x6b17, 0x37bb, 0x3e91, 0x76db,
};

/* random seed used for syndrome calls */
const uint16_t random_seed_syndrome = 0x4a80;

#endif /* end of include guard: NAND_H */
