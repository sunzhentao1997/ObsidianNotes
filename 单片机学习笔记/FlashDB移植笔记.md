###### ① 下载FlashDB源码
	官网：https://github.com/armink/FlashDB/releases
	本地：../FlashDB/FlashDB-2.1.1
###### ② 创建工程(以STM32F103VGT6为例)
	 1、通过STM32CubeIDE创建一个空格的工程;
	 2、新建两个源文件夹 “FlashDB” 和 “FAL”;
###### ③ 文件移植
	1、将FlashDB文件夹下的"fdb.c","fdb_kvdb.c","fdb_tsdb.c","fdb_utils.c","fdb_cfg.h", "fdb_def.h","fdb_low_lvl.h","flashdb.h"文件导入到“FlashDB”文件夹下。"fdb_cfg.h"文件使用"demos\stm32f103ve\applications"文件夹下的。
	2、将"fal_flash_stm32f1_port.c","fal.c","fal_flash.c","fal_partition.c","fal.h","fal_def.h","fal_def.h",导入“Fal”文件夹下。"fal_def.h"在“demos\stm32f103ve\applications”下。
![|300](pic/Pasted%20image%2020250724153905.png#pic_center)

###### ④ 修改配置参数
	1、根据Flash的大小修改"fal_flash_stm32f1_port.c"文件中的参数。
```c
#include <string.h>
#include <fal.h>
#include <stm32f1xx.h>

#if defined(STM32F103xG)
#define PAGE_SIZE     2048
#else
#define PAGE_SIZE     1024
#endif
。
。
。
。
。
/*
  "stm32_onchip" : Flash 设备的名字。
  0x08000000: 对 Flash 操作的起始地址。
  1024*1024：Flash 的总大小（1MB）。
  128*1024：Flash 块/扇区大小（因为 STM32F2 各块大小不均匀，所以擦除粒度为最大块的大小：128K）。
  {init, read, write, erase} ：Flash 的操作函数。 如果没有 init 初始化过程，第一个操作函数位置可以置空。
  8 : 设置写粒度，单位 bit， 0 表示未生效（默认值为 0 ），该成员是 fal 版本大于 0.4.0 的新增成员。各个 flash 写入粒度不尽相同，可通过该成员进行设置，以下列举几种常见 Flash 写粒度：
  nor flash:  1 bit
  stm32f2/f4: 8 bit
  stm32f1:    32 bit
  stm32l4:    64 bit
 */

//1.定义 flash 设备

const struct fal_flash_dev stm32_onchip_flash =
{
    .name       = "stm32_onchip",
    .addr       = 0x08000000,
    .len        = 1024*1024,
    .blk_size   = 2*1024,
    .ops        = {init, read, write, erase},
    .write_gran = 32
};
```
	2、在“fal_cfg.h”头文件中定义flash设备表和分区表
```c
/* ===================== Flash device Configuration ========================= */`
extern const struct fal_flash_dev stm32f2_onchip_flash;
extern struct fal_flash_dev nor_flash0;

/* flash device table */
#define FAL_FLASH_DEV_TABLE 
{ 
	 &stm32f2_onchip_flash,    //stm32片内flash
	 &nor_flash0,              //片外的Nor flash
}
```

```c
#define NOR_FLASH_DEV_NAME "norflash0"`
/* ====================== Partition Configuration ========================== */`
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
#define FAL_PART_TABLE
{
	{FAL_PART_MAGIC_WORD, "fdb_tsdb1", "stm32_onchip", 512*1024, 8*1024, 0},
	{FAL_PART_MAGIC_WORD, "fdb_kvdb1", "stm32_onchip", 520*1024, 8*1024, 0},
	//        ---           分区名          设备名       偏移地址   分区大小
}
#endif /* FAL_PART_HAS_TABLE_CFG */`
```

|  分区名   |    设备名    | 偏移地址 | 分区大小 |
|:---------:|:------------:|:--------:|:--------:|
| fdb_tsdb1 | stm32_onchip | 512*1024 |  8*1024  |
| fdb_kvdb1 | stm32_onchip | 520*1024 |  8*1024  |
###### ⑤ 测试代码
```c
#include <stdio.h>
#include "flashdb.h"
#include "stm32f1xx_hal.h"

float NVMBlock2DataInfo[36] = {1,2,3,4,5,6,7,8};
float NVMBlock1DataInfo[36] = {1,2,3,4,5,6,7,8};
float NVMBlock1Data[36] = {0};
float NVMBlock2Data[36] = {0};

/*定义键值*/
struct fdb_default_kv_node default_kv_table[] =
{
	//{"NVMBlock1",&test,sizeof(test)},
	{"NVMBlock1",&NVMBlock1DataInfo,sizeof(NVMBlock1DataInfo)},
	{"NVMBlock2",&NVMBlock2DataInfo,sizeof(NVMBlock2DataInfo)},
};
static struct fdb_kvdb nvm_kvdb = {0};

static void lock(fdb_db_t db)
{
	__disable_irq();
}

static void unlock(fdb_db_t db)
{
	__enable_irq();
}

uint8_t flashdb_kvdb_init(void)
{
	struct fdb_blob blob;
	struct fdb_default_kv default_kv;

	default_kv.kvs = default_kv_table;
	default_kv.num = sizeof(default_kv_table) / sizeof(default_kv_table[0]);
	/* set the lock and unlock function if you want */
	fdb_kvdb_control(&nvm_kvdb, FDB_KVDB_CTRL_SET_LOCK, (void *)lock);
	fdb_kvdb_control(&nvm_kvdb, FDB_KVDB_CTRL_SET_UNLOCK, (void *)unlock);
	
	/* Key-Value database initialization
	*
	* &kvdb: database object
	* "env": database name
	* "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
	* Please change to YOUR partition name.
	* &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
	* NULL: The user data if you need, now is empty.
	*/
	if(fdb_kvdb_init(&nvm_kvdb, "env", "fdb_kvdb1", &default_kv, NULL) == FDB_NO_ERR)
	{
		fdb_kv_get_blob(&nvm_kvdb, "NVMBlock1", fdb_blob_make(&blob, (const void*)&NVMBlock1Data, sizeof(NVMBlock1Data)));
		fdb_kv_get_blob(&nvm_kvdb, "NVMBlock2", fdb_blob_make(&blob, (const void*)&NVMBlock2Data, sizeof(NVMBlock2Data)));
	}
	return 0;
}

void flashdb_kvdb_main(void)
{
	struct fdb_blob blob;
	static uint8_t old_read_all_flg = 0u;
	static uint8_t old_write_all_flg = 0u;
	static uint8_t old_read_block1_flg = 0u;
	static uint8_t old_write_block1_flg = 0u;
	static uint8_t old_read_block2_flg = 0u;
	static uint8_t old_write_block2_flg = 0u;
	uint8_t read_all_flg = 1;
	uint8_t write_all_flg = 1;
	uint8_t read_block1_flg;
	uint8_t write_block1_flg;
	uint8_t read_block2_flg;
	uint8_t write_block2_flg;

	if((old_read_all_flg == 0u) && (read_all_flg == 1u))
	{
		old_read_all_flg = read_all_flg;
		if(fdb_kv_get_blob(&nvm_kvdb, "NVMBlock1", fdb_blob_make(&blob, (const void*)&NVMBlock1Data, sizeof(NVMBlock1Data))))
		{
			return ;
		}else
		{
		}

		if(fdb_kv_get_blob(&nvm_kvdb, "NVMBlock2", fdb_blob_make(&blob, (const void*)&NVMBlock2Data, sizeof(NVMBlock2Data))))
		{
			return ;
		}else
		{
		}
		return ;
	}

	if((old_write_all_flg == 0u) && (write_all_flg != 0u))
	{
		old_write_all_flg = write_all_flg;
		fdb_kv_set_blob(&nvm_kvdb, "NVMBlock1", fdb_blob_make(&blob, (const void*)&NVMBlock1DataInfo, sizeof(NVMBlock1DataInfo)));
		fdb_kv_set_blob(&nvm_kvdb, "NVMBlock2", fdb_blob_make(&blob, (const void*)&NVMBlock2DataInfo, sizeof(NVMBlock2DataInfo)));
		read_block1_flg = 0;
		read_block2_flg = 0;
		return ;
	}
	if((old_read_block1_flg == 0u) && (read_block1_flg == 1u)){
		if(fdb_kv_get_blob(&nvm_kvdb, "NVMBlock1", fdb_blob_make(&blob, (const void*)&NVMBlock1Data, sizeof(NVMBlock1Data))))
		{
			return ;
		}else
		{
			return ;
		}
	}
	if((old_read_block2_flg == 0u) && (read_block2_flg == 1u))
	{
		if(fdb_kv_get_blob(&nvm_kvdb, "NVMBlock2", fdb_blob_make(&blob, (const void*)&NVMBlock2Data, sizeof(NVMBlock2Data))))
		{
			return ;
		}else
		{
			return ;
		}
	}
	if((old_write_block1_flg == 0u) && (write_block1_flg == 1u)){
		fdb_kv_set_blob(&nvm_kvdb, "NVMBlock1", fdb_blob_make(&blob, (const void*)&NVMBlock1DataInfo, sizeof(NVMBlock1DataInfo)));
		read_block1_flg = 0;
	}
	if((old_write_block2_flg == 0u) && (write_block2_flg == 1u)){
		fdb_kv_set_blob(&nvm_kvdb, "NVMBlock2", fdb_blob_make(&blob, (const void*)&NVMBlock2DataInfo, sizeof(NVMBlock2DataInfo)));
		read_block2_flg = 0;
	}
	old_read_all_flg = read_all_flg;
	old_write_all_flg = write_all_flg;
	old_read_block1_flg = read_block1_flg;
	old_read_block2_flg = read_block2_flg;
	old_write_block1_flg = write_block1_flg;
	old_write_block2_flg = write_block2_flg;
}	
```

