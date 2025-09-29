/*!
 * Copyright (c) 2018-2020 TUXEDO Computers GmbH <tux@tuxedocomputers.com>
 *
 * This file is part of tuxedo-keyboard.
 *
 * tuxedo-keyboard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.  If not, see <https://www.gnu.org/licenses/>.
 */
#define pr_fmt(fmt) "tuxedo_keyboard" ": " fmt

#include "tuxedo_keyboard_common.h"
#include "clevo_keyboard.h"
#include "uniwill_keyboard.h"
#include <linux/mutex.h>
#include <asm/cpu_device_id.h>
#include <asm/intel-family.h>
#include <linux/mod_devicetable.h>
#include <linux/version.h>

MODULE_AUTHOR("TUXEDO Computers GmbH <tux@tuxedocomputers.com>");
MODULE_DESCRIPTION("TUXEDO Computers keyboard & keyboard backlight Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("3.2.10");

static DEFINE_MUTEX(tuxedo_keyboard_init_driver_lock);

// static struct tuxedo_keyboard_driver *driver_list[] = { };

static int tuxedo_input_init(const struct key_entry key_map[])
{
	int err;

	tuxedo_input_device = input_allocate_device();
	if (unlikely(!tuxedo_input_device)) {
		TUXEDO_ERROR("Error allocating input device\n");
		return -ENOMEM;
	}

	tuxedo_input_device->name = "TUXEDO Keyboard";
	tuxedo_input_device->phys = DRIVER_NAME "/input0";
	tuxedo_input_device->id.bustype = BUS_HOST;
	tuxedo_input_device->dev.parent = &tuxedo_platform_device->dev;

	if (key_map != NULL) {
		err = sparse_keymap_setup(tuxedo_input_device, key_map, NULL);
		if (err) {
			TUXEDO_ERROR("Failed to setup sparse keymap\n");
			goto err_free_input_device;
		}
	}

	err = input_register_device(tuxedo_input_device);
	if (unlikely(err)) {
		TUXEDO_ERROR("Error registering input device\n");
		goto err_free_input_device;
	}

	return 0;

err_free_input_device:
	input_free_device(tuxedo_input_device);

	return err;
}

struct platform_device *tuxedo_keyboard_init_driver(struct tuxedo_keyboard_driver *tk_driver)
{
	int err;
	struct platform_device *new_platform_device = NULL;

	TUXEDO_DEBUG("init driver start\n");

	mutex_lock(&tuxedo_keyboard_init_driver_lock);

	if (!IS_ERR_OR_NULL(tuxedo_platform_device)) {
		// If already initialized, don't proceed
		TUXEDO_DEBUG("platform device already initialized\n");
		goto init_driver_exit;
	} else {
		// Otherwise, attempt to initialize structures
		TUXEDO_DEBUG("create platform bundle\n");
		new_platform_device = platform_create_bundle(
			tk_driver->platform_driver, tk_driver->probe, NULL, 0, NULL, 0);

		tuxedo_platform_device = new_platform_device;

		if (IS_ERR_OR_NULL(tuxedo_platform_device)) {
			// Normal case probe failed, no init
			goto init_driver_exit;
		}

		TUXEDO_DEBUG("initialize input device\n");
		if (tk_driver->key_map != NULL) {
			err = tuxedo_input_init(tk_driver->key_map);
			if (unlikely(err)) {
				TUXEDO_ERROR("Could not register input device\n");
				tk_driver->input_device = NULL;
			} else {
				TUXEDO_DEBUG("input device registered\n");
				tk_driver->input_device = tuxedo_input_device;
			}
		}

		current_driver = tk_driver;
	}

init_driver_exit:
	mutex_unlock(&tuxedo_keyboard_init_driver_lock);
	return new_platform_device;
}
EXPORT_SYMBOL(tuxedo_keyboard_init_driver);

static void tuxedo_input_exit(void)
{
	if (unlikely(!tuxedo_input_device)) {
		return;
	}

	input_unregister_device(tuxedo_input_device);
	{
		tuxedo_input_device = NULL;
	}
}

void tuxedo_keyboard_remove_driver(struct tuxedo_keyboard_driver *tk_driver)
{
	bool specified_driver_differ_from_used =
		tk_driver != NULL && 
		(
			strcmp(
				tk_driver->platform_driver->driver.name,
				current_driver->platform_driver->driver.name
			) != 0
		);

	if (specified_driver_differ_from_used)
		return;

	TUXEDO_DEBUG("tuxedo_input_exit()\n");
	tuxedo_input_exit();
	TUXEDO_DEBUG("platform_device_unregister()\n");
	if (!IS_ERR_OR_NULL(tuxedo_platform_device)) {
		platform_device_unregister(tuxedo_platform_device);
		tuxedo_platform_device = NULL;
	}
	TUXEDO_DEBUG("platform_driver_unregister()\n");
	if (!IS_ERR_OR_NULL(current_driver)) {
		platform_driver_unregister(current_driver->platform_driver);
		current_driver = NULL;
	}

}
EXPORT_SYMBOL(tuxedo_keyboard_remove_driver);

// Defines that might be missing in older kernel headers
#define INTEL_FAM6_SAPPHIRERAPIDS_X	0x8F
#define INTEL_FAM6_EMERALDRAPIDS_X	0xCF
#define INTEL_FAM6_ALDERLAKE		0x97
#define INTEL_FAM6_ALDERLAKE_L		0x9A
#define INTEL_FAM6_ALDERLAKE_N		0xBE
#define INTEL_FAM6_RAPTORLAKE		0xB7
#define INTEL_FAM6_RAPTORLAKE_P		0xBA
#define INTEL_FAM6_RAPTORLAKE_S		0xBF

// ALDERLAKE_N doesn't seem to be present in the current kernel header at all
#define INTEL_ALDERLAKE_N 			INTEL_FAM6_ALDERLAKE_N

// Compatibility defines: kernel 6.12 renamed the Intel CPU model defines, see e.g.
// https://github.com/torvalds/linux/commit/6568fc18c2f62 and
// https://github.com/torvalds/linux/commit/13ad4848dde0f83a27d433f7e11722924de1d506
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0)
static const struct x86_cpu_id skip_tuxedo_dmi_string_check_match[] __initconst = {
	X86_MATCH_INTEL_FAM6_MODEL(CORE_YONAH, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(CORE2_MEROM, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(CORE2_MEROM_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(CORE2_PENRYN, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(CORE2_DUNNINGTON, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(NEHALEM, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(NEHALEM_G, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(NEHALEM_EP, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(NEHALEM_EX, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(WESTMERE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(WESTMERE_EP, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(WESTMERE_EX, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(SANDYBRIDGE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(SANDYBRIDGE_X, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(IVYBRIDGE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(IVYBRIDGE_X, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(HASWELL, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(HASWELL_X, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(HASWELL_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(HASWELL_G, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(BROADWELL, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(BROADWELL_G, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(BROADWELL_X, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(BROADWELL_D, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(SKYLAKE_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(SKYLAKE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(SKYLAKE_X, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(KABYLAKE_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(KABYLAKE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(COMETLAKE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(COMETLAKE_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(CANNONLAKE_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ICELAKE_X, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ICELAKE_D, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ICELAKE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ICELAKE_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ICELAKE_NNPI, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(LAKEFIELD, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ROCKETLAKE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(TIGERLAKE_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(TIGERLAKE, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(SAPPHIRERAPIDS_X, NULL), // 12th Gen Xeon
	//X86_MATCH_INTEL_FAM6_MODEL(EMERALDRAPIDS_X, NULL), // 13th Gen Xeon
	X86_MATCH_INTEL_FAM6_MODEL(ALDERLAKE, NULL), // 12th Gen
	X86_MATCH_INTEL_FAM6_MODEL(ALDERLAKE_L, NULL), // 12th Gen
	X86_MATCH_INTEL_FAM6_MODEL(ALDERLAKE_N, NULL), // 12th Gen Atom
	X86_MATCH_INTEL_FAM6_MODEL(RAPTORLAKE, NULL), // 13th Gen
	X86_MATCH_INTEL_FAM6_MODEL(RAPTORLAKE_P, NULL), // 13th Gen
	X86_MATCH_INTEL_FAM6_MODEL(RAPTORLAKE_S, NULL), // 13th Gen
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_BONNELL, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_BONNELL_MID, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_SALTWELL, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_SALTWELL_MID, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_SALTWELL_TABLET, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_SILVERMONT, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_SILVERMONT_D, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_SILVERMONT_MID, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_AIRMONT, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_AIRMONT_MID, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_AIRMONT_NP, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_GOLDMONT, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_GOLDMONT_D, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_GOLDMONT_PLUS, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_TREMONT_D, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_TREMONT, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(ATOM_TREMONT_L, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(XEON_PHI_KNL, NULL),
	X86_MATCH_INTEL_FAM6_MODEL(XEON_PHI_KNM, NULL),
	X86_MATCH_VENDOR_FAM_MODEL(INTEL, 5, INTEL_FAM5_QUARK_X1000, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 5, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 6, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 15, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 16, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 17, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 18, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 19, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 20, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 21, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 22, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 23, NULL), // Zen, Zen+, Zen 2
	X86_MATCH_VENDOR_FAM(AMD, 24, NULL), // Zen
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x01, NULL), // Zen 3 Epyc
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x08, NULL), // Zen 3 Threadripper
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x21, NULL), // Zen 3 Vermeer
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x40, NULL), // Zen 3+ Rembrandt
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x44, NULL), // Zen 3+ Rembrandt
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x50, NULL), // Zen 3 Cezanne
	{ }
};
#else
static const struct x86_cpu_id skip_tuxedo_dmi_string_check_match[] __initconst = {
	X86_MATCH_VFM(INTEL_CORE_YONAH, NULL),
	X86_MATCH_VFM(INTEL_CORE2_MEROM, NULL),
	X86_MATCH_VFM(INTEL_CORE2_MEROM_L, NULL),
	X86_MATCH_VFM(INTEL_CORE2_PENRYN, NULL),
	X86_MATCH_VFM(INTEL_CORE2_DUNNINGTON, NULL),
	X86_MATCH_VFM(INTEL_NEHALEM, NULL),
	X86_MATCH_VFM(INTEL_NEHALEM_G, NULL),
	X86_MATCH_VFM(INTEL_NEHALEM_EP, NULL),
	X86_MATCH_VFM(INTEL_NEHALEM_EX, NULL),
	X86_MATCH_VFM(INTEL_WESTMERE, NULL),
	X86_MATCH_VFM(INTEL_WESTMERE_EP, NULL),
	X86_MATCH_VFM(INTEL_WESTMERE_EX, NULL),
	X86_MATCH_VFM(INTEL_SANDYBRIDGE, NULL),
	X86_MATCH_VFM(INTEL_SANDYBRIDGE_X, NULL),
	X86_MATCH_VFM(INTEL_IVYBRIDGE, NULL),
	X86_MATCH_VFM(INTEL_IVYBRIDGE_X, NULL),
	X86_MATCH_VFM(INTEL_HASWELL, NULL),
	X86_MATCH_VFM(INTEL_HASWELL_X, NULL),
	X86_MATCH_VFM(INTEL_HASWELL_L, NULL),
	X86_MATCH_VFM(INTEL_HASWELL_G, NULL),
	X86_MATCH_VFM(INTEL_BROADWELL, NULL),
	X86_MATCH_VFM(INTEL_BROADWELL_G, NULL),
	X86_MATCH_VFM(INTEL_BROADWELL_X, NULL),
	X86_MATCH_VFM(INTEL_BROADWELL_D, NULL),
	X86_MATCH_VFM(INTEL_SKYLAKE_L, NULL),
	X86_MATCH_VFM(INTEL_SKYLAKE, NULL),
	X86_MATCH_VFM(INTEL_SKYLAKE_X, NULL),
	X86_MATCH_VFM(INTEL_KABYLAKE_L, NULL),
	X86_MATCH_VFM(INTEL_KABYLAKE, NULL),
	X86_MATCH_VFM(INTEL_COMETLAKE, NULL),
	X86_MATCH_VFM(INTEL_COMETLAKE_L, NULL),
	X86_MATCH_VFM(INTEL_CANNONLAKE_L, NULL),
	X86_MATCH_VFM(INTEL_ICELAKE_X, NULL),
	X86_MATCH_VFM(INTEL_ICELAKE_D, NULL),
	X86_MATCH_VFM(INTEL_ICELAKE, NULL),
	X86_MATCH_VFM(INTEL_ICELAKE_L, NULL),
	X86_MATCH_VFM(INTEL_ICELAKE_NNPI, NULL),
	X86_MATCH_VFM(INTEL_LAKEFIELD, NULL),
	X86_MATCH_VFM(INTEL_ROCKETLAKE, NULL),
	X86_MATCH_VFM(INTEL_TIGERLAKE_L, NULL),
	X86_MATCH_VFM(INTEL_TIGERLAKE, NULL),
	X86_MATCH_VFM(INTEL_SAPPHIRERAPIDS_X, NULL), // 12th Gen Xeon
	//X86_MATCH_VFM(INTEL_EMERALDRAPIDS_X, NULL), // 13th Gen Xeon
	X86_MATCH_VFM(INTEL_ALDERLAKE, NULL), // 12th Gen
	X86_MATCH_VFM(INTEL_ALDERLAKE_L, NULL), // 12th Gen
	X86_MATCH_VFM(INTEL_ALDERLAKE_N, NULL), // 12th Gen Atom
	X86_MATCH_VFM(INTEL_RAPTORLAKE, NULL), // 13th Gen
	X86_MATCH_VFM(INTEL_RAPTORLAKE_P, NULL), // 13th Gen
	X86_MATCH_VFM(INTEL_RAPTORLAKE_S, NULL), // 13th Gen
	X86_MATCH_VFM(INTEL_ATOM_BONNELL, NULL),
	X86_MATCH_VFM(INTEL_ATOM_BONNELL_MID, NULL),
	X86_MATCH_VFM(INTEL_ATOM_SALTWELL, NULL),
	X86_MATCH_VFM(INTEL_ATOM_SALTWELL_MID, NULL),
	X86_MATCH_VFM(INTEL_ATOM_SALTWELL_TABLET, NULL),
	X86_MATCH_VFM(INTEL_ATOM_SILVERMONT, NULL),
	X86_MATCH_VFM(INTEL_ATOM_SILVERMONT_D, NULL),
	X86_MATCH_VFM(INTEL_ATOM_SILVERMONT_MID, NULL),
	X86_MATCH_VFM(INTEL_ATOM_AIRMONT, NULL),
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 15, 0)
	X86_MATCH_VFM(INTEL_ATOM_AIRMONT_MID, NULL),
#else
  X86_MATCH_VFM(INTEL_ATOM_SILVERMONT_MID2, NULL),
#endif
	X86_MATCH_VFM(INTEL_ATOM_AIRMONT_NP, NULL),
	X86_MATCH_VFM(INTEL_ATOM_GOLDMONT, NULL),
	X86_MATCH_VFM(INTEL_ATOM_GOLDMONT_D, NULL),
	X86_MATCH_VFM(INTEL_ATOM_GOLDMONT_PLUS, NULL),
	X86_MATCH_VFM(INTEL_ATOM_TREMONT_D, NULL),
	X86_MATCH_VFM(INTEL_ATOM_TREMONT, NULL),
	X86_MATCH_VFM(INTEL_ATOM_TREMONT_L, NULL),
	X86_MATCH_VFM(INTEL_XEON_PHI_KNL, NULL),
	X86_MATCH_VFM(INTEL_XEON_PHI_KNM, NULL),
// Compatibility defines: INTEL_FAM5_QUARK_X1000 remove in 6.13
#ifdef INTEL_FAM5_QUARK_X1000
	X86_MATCH_VENDOR_FAM_MODEL(INTEL, 5, INTEL_FAM5_QUARK_X1000, NULL),
#else
	X86_MATCH_VFM(INTEL_QUARK_X1000, NULL),
#endif
	X86_MATCH_VENDOR_FAM(AMD, 5, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 6, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 15, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 16, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 17, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 18, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 19, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 20, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 21, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 22, NULL),
	X86_MATCH_VENDOR_FAM(AMD, 23, NULL), // Zen, Zen+, Zen 2
	X86_MATCH_VENDOR_FAM(AMD, 24, NULL), // Zen
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x01, NULL), // Zen 3 Epyc
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x08, NULL), // Zen 3 Threadripper
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x21, NULL), // Zen 3 Vermeer
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x40, NULL), // Zen 3+ Rembrandt
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x44, NULL), // Zen 3+ Rembrandt
	X86_MATCH_VENDOR_FAM_MODEL(AMD, 25, 0x50, NULL), // Zen 3 Cezanne
	{ }
};
#endif

static const struct x86_cpu_id force_tuxedo_dmi_string_check_match[] __initconst = {
	{ }
};

static const struct dmi_system_id tuxedo_dmi_string_match[] __initconst = {
	{
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "Notebook"),
		},
	},
	{
		.matches = {
			DMI_MATCH(DMI_BOARD_VENDOR, "Notebook"),
		},
	},
	{
		.matches = {
			DMI_MATCH(DMI_CHASSIS_VENDOR, "No Enclosure"),
		},
	},
	{ }
};

static int __init tuxedo_keyboard_init(void)
{
	TUXEDO_INFO("module init\n");

	if (!(dmi_check_system(tuxedo_dmi_string_match)
	    || (x86_match_cpu(skip_tuxedo_dmi_string_check_match)
	    && !x86_match_cpu(force_tuxedo_dmi_string_check_match)))) {
		return -ENODEV;
	}

	return 0;
}

/*
 * 6.16+ strictly enforces that __exit functions cannot call non-__exit functions.
 */
static void tuxedo_keyboard_exit(void)
{
	TUXEDO_INFO("module exit\n");

	if (tuxedo_platform_device != NULL)
		tuxedo_keyboard_remove_driver(NULL);
}

module_init(tuxedo_keyboard_init);
module_exit(tuxedo_keyboard_exit);
