/*
 * TEE service to handle the calls to trusted applications.
 *
 * Copyright (C) ST-Ericsson SA 2010
 * Author: Joakim Bech <joakim.xx.bech@stericsson.com>
 * License terms: GNU General Public License (GPL) version 2
 */
#include <linux/kernel.h>
#include <linux/tee.h>
#include <linux/io.h>

#include <mach/hardware.h>

#define BOOT_CONVERSION_FUNC (U8500_BOOT_ROM_BASE + 0x18588)
#define BOOT_BRIDGE_FUNC (U8500_BOOT_ROM_BASE + 0x18300)

#define ISSWAPI_EXECUTE_TA 0x11000001
#define SEC_ROM_FORCE_CLEAN_MASK 0x20

static u32 call_sec_rom_bridge(u32 service_id, u32 cfg, ...)
{
	typedef u32 (*bridge_func)(u32, u32, va_list);
	static bridge_func hw_sec_rom_pub_bridge;
	va_list ap;
	u32 ret;

	hw_sec_rom_pub_bridge =
		(bridge_func)((u32)IO_ADDRESS(BOOT_BRIDGE_FUNC));

	va_start(ap, cfg);
	ret = hw_sec_rom_pub_bridge(service_id, cfg, ap);
	va_end(ap);

	return ret;
}

int call_sec_world(struct tee_session *ts)
{
	/*
	 * ts->ta and ts->uuid is set to NULL when opening the device,
	 * hence it should be safe to just do the call here.
	 */

	if (ts->uuid == NULL) {
		call_sec_rom_bridge(ISSWAPI_EXECUTE_TA,
				    SEC_ROM_FORCE_CLEAN_MASK,
				    virt_to_phys(&ts->id),
				    NULL,
				    virt_to_phys(ts->ta),
				    ts->cmd,
				    virt_to_phys((void *)(ts->idata)),
				    virt_to_phys((void *)(&ts->origin)));
	} else {
		call_sec_rom_bridge(ISSWAPI_EXECUTE_TA,
				    SEC_ROM_FORCE_CLEAN_MASK,
				    virt_to_phys(&ts->id),
				    virt_to_phys(ts->uuid),
				    virt_to_phys(ts->ta),
				    ts->cmd,
				    virt_to_phys((void *)(ts->idata)),
				    virt_to_phys((void *)(&ts->origin)));
	}


	return 0;
}
