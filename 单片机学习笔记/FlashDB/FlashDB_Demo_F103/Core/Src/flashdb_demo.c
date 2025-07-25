/*
 * flashdb_demo.c
 *
 *  Created on: Jul 24, 2025
 *      Author: szt
 */
#include <stdio.h>
#include "flashdb.h"
#include "flashdb_demo.h"
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
	 *       &kvdb: database object
	 *       "env": database name
	 * "fdb_kvdb1": The flash partition name base on FAL. Please make sure it's in FAL partition table.
	 *              Please change to YOUR partition name.
	 * &default_kv: The default KV nodes. It will auto add to KVDB when first initialize successfully.
	 *        NULL: The user data if you need, now is empty.
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
		}
		else
		{
		}
		if(fdb_kv_get_blob(&nvm_kvdb, "NVMBlock2", fdb_blob_make(&blob, (const void*)&NVMBlock2Data, sizeof(NVMBlock2Data))))
		{
			return ;
		}
		else
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




