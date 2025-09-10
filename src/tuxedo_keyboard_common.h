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
#ifndef TUXEDO_KEYBOARD_COMMON_H
#define TUXEDO_KEYBOARD_COMMON_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/acpi.h>
#include <linux/dmi.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/input/sparse-keymap.h>

/* ::::  Module specific Constants and simple Macros   :::: */
#define __TUXEDO_PR(lvl, fmt, ...) do { pr_##lvl(fmt, ##__VA_ARGS__); } while (0)
#define TUXEDO_INFO(fmt, ...) __TUXEDO_PR(info, fmt, ##__VA_ARGS__)
#define TUXEDO_ERROR(fmt, ...) __TUXEDO_PR(err, fmt, ##__VA_ARGS__)
#define TUXEDO_DEBUG(fmt, ...) __TUXEDO_PR(debug, "[%s:%u] " fmt, __func__, __LINE__, ##__VA_ARGS__)

#ifndef DRIVER_NAME
#define DRIVER_NAME "tuxedo_keyboard"
#endif

struct tuxedo_keyboard_driver {
	// Platform driver provided by driver
	struct platform_driver *platform_driver;
	// Probe method provided by driver
	int (*probe)(struct platform_device *);
	// Keymap provided by driver
	struct key_entry *key_map;
	// Input device reference filled in on module init after probe success
	struct input_dev *input_device;
};

// Global module devices
static struct platform_device *tuxedo_platform_device = NULL;
static struct input_dev *tuxedo_input_device = NULL;

// Currently chosen driver
static struct tuxedo_keyboard_driver *current_driver = NULL;

struct platform_device *tuxedo_keyboard_init_driver(struct tuxedo_keyboard_driver *tk_driver);
void tuxedo_keyboard_remove_driver(struct tuxedo_keyboard_driver *tk_driver);

/**
 * Basically a copy of the existing report event but doesn't report unknown events
 */
inline bool sparse_keymap_report_known_event(struct input_dev *dev, unsigned int code,
					unsigned int value, bool autorelease)
{
	const struct key_entry *ke =
		sparse_keymap_entry_from_scancode(dev, code);

	if (ke) {
		sparse_keymap_report_entry(dev, ke, value, autorelease);
		return true;
	}

	return false;
}

struct color_t {
	u32 code;
	char* name;
};

struct color_list_t {
	uint size;
	struct color_t colors[];
};

/**
 * Commonly used standard colors
 */
static struct color_list_t color_list = {
	.size = 8,
	.colors = {
		{ .name = "BLACK",    .code = 0x000000 },  // 0
		{ .name = "RED",      .code = 0xFF0000 },  // 1
		{ .name = "GREEN",    .code = 0x00FF00 },  // 2
		{ .name = "BLUE",     .code = 0x0000FF },  // 3
		{ .name = "YELLOW",   .code = 0xFFFF00 },  // 4
		{ .name = "MAGENTA",  .code = 0xFF00FF },  // 5
		{ .name = "CYAN",     .code = 0x00FFFF },  // 6
		{ .name = "WHITE",    .code = 0xFFFFFF },  // 7
	}
};

#endif
