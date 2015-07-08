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

void nand_set_clocks(void)
{
	uint32_t val;

	writei(PORTC_BASE + 0x48, 0x22222222);
	writei(PORTC_BASE + 0x4C, 0x22222222);
	writei(PORTC_BASE + 0x50, 0x2222222);
	writei(PORTC_BASE + 0x54, 0x2);
	writei(PORTC_BASE + 0x5C, 0x55555555);
	writei(PORTC_BASE + 0x60, 0x15555);
	writei(PORTC_BASE + 0x64, 0x5140);
	writei(PORTC_BASE + 0x68, 0x4016);

	val = readi(CCMU_BASE + 0x60);
	writei(CCMU_BASE + 0x60, 0x2000 | val);

	val = readi(CCMU_BASE + 0x80);
	writei(CCMU_BASE + 0x80, val | 0x80000000 | 0x1);
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
	writei(NANDFLASHC_BASE + NANDFLASHC_CMD,
	    NFC_SEND_CMD1 | NFC_WAIT_FLAG | 0xff);

	do {
		val = readi(NANDFLASHC_BASE + NANDFLASHC_ST);
		if (val & (1 << 1))
			break;
		udelay(1000);
	} while (1);

	page = (real_addr / 0x2000);
	column = real_addr % 0x2000;

	if (syndrome) {
		/* shift every 1kB in syndrome */
		column += (column / SPL_WRITE_SIZE) * ECC_OFF;
	}

	/* clear ecc status */
	writei(NANDFLASHC_BASE + NANDFLASHC_ECC_ST, 0);

	/* Choose correct seed */
	if (syndrome)
		rand_seed = random_seed_syndrome;
	else
		rand_seed = random_seed[page % 128];

	writei(NANDFLASHC_BASE + NANDFLASHC_ECC_CTL,
	    (rand_seed << 16) | NFC_ECC_RANDOM_EN | NFC_ECC_EN |
	    NFC_ECC_PIPELINE | (ecc_mode << 12));

	val = readi(NANDFLASHC_BASE + NANDFLASHC_CTL);
	writei(NANDFLASHC_BASE + NANDFLASHC_CTL, val | NFC_CTL_RAM_METHOD);

	if (syndrome) {
		writei(NANDFLASHC_BASE + NANDFLASHC_SPARE_AREA, SPL_WRITE_SIZE);
	} else {
		oob_offset = 0x2000  + (column / SPL_WRITE_SIZE) * ECC_OFF;
		writei(NANDFLASHC_BASE + NANDFLASHC_SPARE_AREA, oob_offset);
	}

	/* DMAC */
	writei(DMAC_BASE + 0x300, 0x0); /* clr dma cmd */
	/* read from REG_IO_DATA */
	writei(DMAC_BASE + 0x304, NANDFLASHC_BASE + NANDFLASHC_IO_DATA);
	writei(DMAC_BASE + 0x308, (uint32_t)temp_buf); /* read to RAM */
	writei(DMAC_BASE + 0x318, 0x7F0F);
	writei(DMAC_BASE + 0x30C, SPL_WRITE_SIZE); /* 1kB */
	writei(DMAC_BASE + 0x300, 0x84000423);

	writei(NANDFLASHC_BASE + NANDFLASHC_RCMD_SET, 0x00E00530); /* READ */
	writei(NANDFLASHC_BASE + NANDFLASHC_SECTOR_NUM, 1);
	writei(NANDFLASHC_BASE + NANDFLASHC_ADDR_LOW,
	    ((page & 0xFFFF) << 16) | column);
	writei(NANDFLASHC_BASE + NANDFLASHC_ADDR_HIGH, (page >> 16) & 0xFF);
	writei(NANDFLASHC_BASE + NANDFLASHC_CMD,
	    NFC_SEND_CMD1 | NFC_SEND_CMD2 | NFC_DATA_TRANS |
	    NFC_PAGE_CMD | NFC_WAIT_FLAG | /* ADDR_CYCLE */ (4 << 16) |
	    NFC_SEND_ADR | NFC_DATA_SWAP_METHOD | (syndrome ? NFC_SEQ : 0));

	do { /* wait for dma irq */
		val = readi(NANDFLASHC_BASE + NANDFLASHC_ST);
		if (val & (1 << 2))
			break;
		udelay(1000);
	} while (1);

	do { /* make sure cmd is finished */
		val = readi(DMAC_BASE + 300);
		if (!(val & 0x80000000))
			break;
		udelay(1000);
	} while (1);

	if (readi(NANDFLASHC_BASE + NANDFLASHC_ECC_ST))
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
	val = readi(NANDFLASHC_BASE + NANDFLASHC_CTL);
	/* enable and reset CTL */
	writei(NANDFLASHC_BASE + NANDFLASHC_CTL, val | NFC_CTL_EN | NFC_CTL_RESET);
	do {
		val = readi(NANDFLASHC_BASE + NANDFLASHC_CTL);
		if (val & NFC_CTL_RESET)
			break;
	} while (1);

	return 0;
}

int nand_spl_load_image(uint32_t offs, unsigned int size, void *dest)
{
	helper_load(offs, size, dest);
	return 0;
}

void nand_deselect(void) {}
