/*
 * (C) Copyright 2014
 *
 * Dmitry Yatsushkevih <dmitry.yatsushkevich@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 *
 * EXT4 support in SPL
 */

#include <common.h>
#include <spl.h>
#include <asm/u-boot.h>
#include <ext4fs.h>
#include <image.h>

static int ext4_registered;
static disk_partition_t cur_part_info;


#ifdef CONFIG_SPL_EXT4_SUPPORT
static int spl_register_ext4_device(block_dev_desc_t *block_dev, int partition)
{
	int err = 0;
	if (ext4_registered)
		return err;

	/* Read the partition table, if present */
	err = get_partition_info(block_dev, partition, &cur_part_info);
	if (err) {
		if (partition != 0){
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
		printf("%s: partition %d not valid on device %d\n",
			__func__, partition, block_dev->dev);
#endif
			goto end;
		}

		cur_part_info.start = 0;
		cur_part_info.size = block_dev->lba;
		cur_part_info.blksz = block_dev->blksz;
		cur_part_info.name[0] = 0;
		cur_part_info.type[0] = 0;
		cur_part_info.bootable = 0;
#ifdef CONFIG_PARTITION_UUIDS
		cur_part_info.uuid[0] = 0;
#endif
	}

	err = ext4fs_probe(block_dev, &cur_part_info);

end:
	if (err){
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
		printf("%s: ext4 register err - %d\n",
			__func__, err);
#endif
		hang();
	}

	ext4_registered = 1;

	return err;
}

int spl_load_image_ext4(block_dev_desc_t *block_dev, int partition,
		const char *filename)
{
	int err;
	struct image_header *header;

	err = spl_register_ext4_device(block_dev, partition);
	if (err)
		goto end;

	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE -
						sizeof(struct image_header));

	err = ext4_read_file(filename, header, 0, sizeof(struct image_header));
	if (err <= 0)
		goto end;

	spl_parse_image_header(header);

	err = ext4_read_file(filename, (u8 *)spl_image.load_addr, 0, 0);

end:
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
	if (err <= 0)
		printf("%s: error reading image %s, err - %d\n",
		       __func__, filename, err);
#endif

	return (err <= 0);
}

#ifdef CONFIG_SPL_OS_BOOT
int spl_load_image_ext4_os(block_dev_desc_t *block_dev, int partition)
{
	int err;

	err = spl_register_ext4_device(block_dev, partition);
	if (err)
		return err;

	err = ext4_read_file(CONFIG_SPL_FAT_LOAD_ARGS_NAME,
			    (void *)CONFIG_SYS_SPL_ARGS_ADDR, 0, 0);
	if (err <= 0) {
#ifdef CONFIG_SPL_LIBCOMMON_SUPPORT
		printf("%s: error reading image %s, err - %d\n",
		       __func__, CONFIG_SPL_FAT_LOAD_ARGS_NAME, err);
#endif
		return -1;
	}

	return spl_load_image_ext4(block_dev, partition,
			CONFIG_SPL_FAT_LOAD_KERNEL_NAME);
}
#endif
#endif
