/*
 * Copyright (c) 2014-2015, Antmicro Ltd <www.antmicro.com>
 * Copyright (c) 2015, AW-SOM Technologies <www.aw-som.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <config.h>
#include <asm/io.h>
#include <nand.h>
#include "nand.h" /* sunxi specific header */

/* minimal "boot0" style NAND support for Allwinner A20 */

/* temporary buffer in internal ram */
#ifdef CONFIG_SPL_BUILD
/* in SPL temporary buffer cannot be @ 0x0 */
unsigned char temp_buf[SPL_WRITE_SIZE] __aligned(0x10) __section(".text#");
#else
/* put temporary buffer @ 0x0 */
unsigned char *temp_buf = (unsigned char *)0x0;
#endif

#define MAX_RETRIES 5

static int check_value_inner(int offset, int expected_bits, int max_number_of_retries, int negation)
{
    int retries = 0;
	do {
		int val = readl(offset) & expected_bits;
		if (negation ? !val : val)
			return 1;
		mdelay(1);
		retries++;
	} while (retries < max_number_of_retries);

	return 0;
}

static inline int check_value(int offset, int expected_bits, int max_number_of_retries)
{
    return check_value_inner(offset, expected_bits, max_number_of_retries, 0);
}

static inline int check_value_negated(int offset, int unexpected_bits, int max_number_of_retries)
{
    return check_value_inner(offset, unexpected_bits, max_number_of_retries, 1);
}

void nand_set_clocks(void)
{
	uint32_t val;

	writel(0x22222222, PORTC_BASE + 0x48);
	writel(0x22222222, PORTC_BASE + 0x4C);
	writel(0x2222222, PORTC_BASE + 0x50);
	writel(0x2, PORTC_BASE + 0x54);
	writel(0x55555555, PORTC_BASE + 0x5C);
	writel(0x15555, PORTC_BASE + 0x60);
	writel(0x5140, PORTC_BASE + 0x64);
	writel(0x4016, PORTC_BASE + 0x68);

	val = readl(CCMU_BASE + 0x60);
	writel(0x2000 | val, CCMU_BASE + 0x60);

	val = readl(CCMU_BASE + 0x80);
	writel(val | 0x80000000 | 0x1, CCMU_BASE + 0x80);
}

volatile int nand_initialized;

void nand_init(void)
{
	board_nand_init(NULL);
}

volatile uint32_t ecc_errors;

/* read 0x400 bytes from real_addr to temp_buf */
void nand_read_block(unsigned int real_addr, int syndrome)
{
	uint32_t val;
	int ECC_OFF = 0;
	uint16_t ecc_mode = 0;
	uint16_t rand_seed;
	uint32_t page;
	uint16_t column;
	uint32_t oob_offset;

	if (!nand_initialized)
		board_nand_init(NULL);

	switch (CONFIG_ECC_STRENGTH) {
	case 16:
		ecc_mode = 0;
		ECC_OFF = 0x20;
		break;
	case 24:
		ecc_mode = 1;
		ECC_OFF = 0x2e;
		break;
	case 28:
		ecc_mode = 2;
		ECC_OFF = 0x32;
		break;
	case 32:
		ecc_mode = 3;
		ECC_OFF = 0x3c;
		break;
	case 40:
		ecc_mode = 4;
		ECC_OFF = 0x4a;
		break;
	case 48:
		ecc_mode = 4;
		ECC_OFF = 0x52;
		break;
	case 56:
		ecc_mode = 4;
		ECC_OFF = 0x60;
		break;
	case 60:
		ecc_mode = 4;
		ECC_OFF = 0x0;
		break;
	case 64:
		ecc_mode = 4;
		ECC_OFF = 0x0;
		break;
	default:
		ecc_mode = 0;
		ECC_OFF = 0;
	}

	if (ECC_OFF == 0) {
		printf("Unsupported ECC strength (%d)!", CONFIG_ECC_STRENGTH);
		return;
	}

	/* clear temp_buf */
	memset((void *)temp_buf, 0, SPL_WRITE_SIZE);

	/* set CMD  */
	writel(NFC_SEND_CMD1 | NFC_WAIT_FLAG | 0xff,
	        NANDFLASHC_BASE + NANDFLASHC_CMD);

    if (!check_value(NANDFLASHC_BASE + NANDFLASHC_ST, (1 << 1), MAX_RETRIES)) {
        printf("Error while initilizing command interrupt");
        return;
    }
    
	page = (real_addr / 0x2000);
	column = real_addr % 0x2000;

	if (syndrome) {
		/* shift every 1kB in syndrome */
		column += (column / SPL_WRITE_SIZE) * ECC_OFF;
	}

	/* clear ecc status */
	writel(0, NANDFLASHC_BASE + NANDFLASHC_ECC_ST);

	/* Choose correct seed */
	if (syndrome)
		rand_seed = random_seed_syndrome;
	else
		rand_seed = random_seed[page % 128];

	writel((rand_seed << 16) | NFC_ECC_RANDOM_EN | NFC_ECC_EN
	        | NFC_ECC_PIPELINE | (ecc_mode << 12),
	        NANDFLASHC_BASE + NANDFLASHC_ECC_CTL);

	val = readl(NANDFLASHC_BASE + NANDFLASHC_CTL);
	writel(val | NFC_CTL_RAM_METHOD, NANDFLASHC_BASE + NANDFLASHC_CTL);

	if (syndrome) {
		writel(SPL_WRITE_SIZE, NANDFLASHC_BASE + NANDFLASHC_SPARE_AREA);
	} else {
		oob_offset = 0x2000  + (column / SPL_WRITE_SIZE) * ECC_OFF;
		writel(oob_offset, NANDFLASHC_BASE + NANDFLASHC_SPARE_AREA);
	}

	/* DMAC */
	writel(0x0, DMAC_BASE + 0x300); /* clr dma cmd */
	/* read from REG_IO_DATA */
	writel(NANDFLASHC_BASE + NANDFLASHC_IO_DATA, DMAC_BASE + 0x304);
	writel((uint32_t)temp_buf, DMAC_BASE + 0x308); /* read to RAM */
	writel(0x7F0F, DMAC_BASE + 0x318);
	writel(SPL_WRITE_SIZE, DMAC_BASE + 0x30C); /* 1kB */
	writel(0x84000423, DMAC_BASE + 0x300);

	writel(0x00E00530, NANDFLASHC_BASE + NANDFLASHC_RCMD_SET); /* READ */
	writel(1, NANDFLASHC_BASE + NANDFLASHC_SECTOR_NUM);
	writel(((page & 0xFFFF) << 16) | column, NANDFLASHC_BASE + NANDFLASHC_ADDR_LOW);
	writel((page >> 16) & 0xFF, NANDFLASHC_BASE + NANDFLASHC_ADDR_HIGH);
	writel(NFC_SEND_CMD1 | NFC_SEND_CMD2 | NFC_DATA_TRANS |
	    NFC_PAGE_CMD | NFC_WAIT_FLAG | /* ADDR_CYCLE */ (4 << 16) |
	    NFC_SEND_ADR | NFC_DATA_SWAP_METHOD | (syndrome ? NFC_SEQ : 0),
        NANDFLASHC_BASE + NANDFLASHC_CMD);

    if (!check_value(NANDFLASHC_BASE + NANDFLASHC_ST, (1 << 2), MAX_RETRIES)) {
        printf("Error while initializing dma interrupt");
        return;
    }

    if (!check_value_negated(DMAC_BASE + 300, 0x80000000, MAX_RETRIES)) {
        printf("Error while waiting for cmd to be finished.");
        return;
    }

	if (readl(NANDFLASHC_BASE + NANDFLASHC_ECC_ST))
		ecc_errors++;
}

int helper_load(uint32_t offs, unsigned int size, void *dest)
{
	uint32_t dst;
	uint32_t count;
	uint32_t count_c;
	uint32_t adr = offs;

	memset((void *)dest, 0x0, size); /* clean destination memory */
	ecc_errors = 0;
	for (dst = (uint32_t)dest;
	     dst < ((uint32_t)dest + size);
	     dst += SPL_WRITE_SIZE) {
		/* if < 0x400000 then syndrome read */
		nand_read_block(adr, adr < 0x400000);
		count = dst - (uint32_t)dest;

		if (size - count > SPL_WRITE_SIZE)
			count_c = SPL_WRITE_SIZE;
		else
			count_c = size - count;

		memcpy((void *)dst,
		       (void *)temp_buf,
		       count_c);
		adr += SPL_WRITE_SIZE;

		if (count &&
		    ((!((count / 0x8000) % 80) &&
			((count % 0x8000) == 00)) ||
			((count + SPL_WRITE_SIZE) >= size)))
			printf("\n");

		if ((count % 0x8000) == 0)
			printf(".");
	}
	return ecc_errors;
}

int board_nand_init(struct nand_chip *nand)
{
	uint32_t val;

	if (nand_initialized) {
		printf("NAND already initialized, should not be here...\n");
		return 0;
	}
	nand_initialized = 1;
	nand_set_clocks();
	val = readl(NANDFLASHC_BASE + NANDFLASHC_CTL);
	/* enable and reset CTL */
	writel(val | NFC_CTL_EN | NFC_CTL_RESET, NANDFLASHC_BASE + NANDFLASHC_CTL);

	if(!check_value(NANDFLASHC_BASE + NANDFLASHC_CTL, NFC_CTL_RESET, MAX_RETRIES)) {
        printf("Couldn't initialize nand");
        return 1;
    }

	return 0;
}

int nand_spl_load_image(uint32_t offs, unsigned int size, void *dest)
{
	helper_load(offs, size, dest);
	return 0;
}

void nand_deselect(void) {}
