/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/printk.h>
#include <errno.h>
#include <stddef.h>

#define STORAGE_PARTITION storage_partition

/* "MPUC". Bump CAL_VERSION whenever struct cal_record changes shape. */
#define CAL_MAGIC   0x4350554dU
#define CAL_VERSION 1U

/*
 * Stored at offset 0 of the storage partition. The magic separates a real
 * record from erased flash (all 0xFF), the version rejects one written by an
 * older layout, and the CRC catches a write cut short by a reset.
 */
struct cal_record {
	uint32_t magic;
	uint32_t version;
	struct accel_calibration cal;
	uint32_t crc;
};

/* flash_area_write needs a size that is a multiple of the write block. */
BUILD_ASSERT(sizeof(struct cal_record) % 4 == 0,
	     "cal_record must be a multiple of the flash write block size");

static const struct flash_area *storage_area;

static uint32_t record_crc(const struct cal_record *rec)
{
	return crc32_ieee((const uint8_t *)rec, offsetof(struct cal_record, crc));
}

/*
 * Opened on first use so load/store do not depend on storage_init() having run
 * first. Thread start order is not something this module should have to rely on.
 */
static int ensure_open(void)
{
	int rc;

	if (storage_area != NULL) {
		return 0;
	}

	rc = flash_area_open(PARTITION_ID(STORAGE_PARTITION), &storage_area);
	if (rc != 0) {
		printk("storage: cannot open partition: %d\n", rc);
		storage_area = NULL;
		return rc;
	}

	if (!flash_area_device_is_ready(storage_area)) {
		printk("storage: flash device not ready\n");
		flash_area_close(storage_area);
		storage_area = NULL;
		return -ENODEV;
	}

	return 0;
}

int storage_init(void)
{
	return ensure_open();
}

int storage_load_calibration(struct accel_calibration *cal)
{
	struct cal_record rec;
	int rc = ensure_open();

	if (rc != 0) {
		return rc;
	}

	rc = flash_area_read(storage_area, 0, &rec, sizeof(rec));
	if (rc != 0) {
		printk("storage: read failed: %d\n", rc);
		return rc;
	}

	if (rec.magic != CAL_MAGIC) {
		/* Erased or never written -- not an error worth logging. */
		return -ENOENT;
	}

	if (rec.version != CAL_VERSION) {
		printk("storage: record version %u, expected %u\n",
		       rec.version, CAL_VERSION);
		return -ENOTSUP;
	}

	if (rec.crc != record_crc(&rec)) {
		printk("storage: record checksum mismatch\n");
		return -EILSEQ;
	}

	*cal = rec.cal;

	return 0;
}

int storage_store_calibration(const struct accel_calibration *cal)
{
	struct cal_record rec = {
		.magic = CAL_MAGIC,
		.version = CAL_VERSION,
		.cal = *cal,
	};
	struct flash_pages_info page;
	int rc = ensure_open();

	if (rc != 0) {
		return rc;
	}

	rec.crc = record_crc(&rec);

	/*
	 * NOR flash can only clear bits on write, so the block has to be erased
	 * first. Erase is page-granular; ask the driver how big a page is here
	 * rather than assuming this SoC's 4 KB.
	 */
	rc = flash_get_page_info_by_offs(flash_area_get_device(storage_area),
					 storage_area->fa_off, &page);
	if (rc != 0) {
		printk("storage: page info failed: %d\n", rc);
		return rc;
	}

	rc = flash_area_erase(storage_area, 0, page.size);
	if (rc != 0) {
		printk("storage: erase failed: %d\n", rc);
		return rc;
	}

	rc = flash_area_write(storage_area, 0, &rec, sizeof(rec));
	if (rc != 0) {
		printk("storage: write failed: %d\n", rc);
		return rc;
	}

	return 0;
}
