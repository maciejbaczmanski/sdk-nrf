/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <openthread/thread.h>
#include <zephyr/net/openthread.h>

#if defined(CONFIG_CLI_SAMPLE_MULTIPROTOCOL)
#include "ble.h"
#endif

#if defined(CONFIG_CLI_SAMPLE_LOW_POWER)
#include "low_power.h"
#endif

#include <zephyr/drivers/uart.h>
#include <zephyr/usb/usb_device.h>

LOG_MODULE_REGISTER(cli_sample, CONFIG_OT_COMMAND_LINE_INTERFACE_LOG_LEVEL);

#define WELLCOME_TEXT \
	"\n\r"\
	"\n\r"\
	"OpenThread Command Line Interface is now running.\n\r" \
	"Use the 'ot' keyword to invoke OpenThread commands e.g. " \
	"'ot thread start.'\n\r" \
	"For the full commands list refer to the OpenThread CLI " \
	"documentation at:\n\r" \
	"https://github.com/openthread/openthread/blob/master/src/cli/README.md\n\r"



static void on_commissioner_state_changed(otCommissionerState aState, void *aContext)
{
	LOG_INF("Commissioner started");
	if(aState == OT_COMMISSIONER_STATE_ACTIVE)
	{
		otCommissionerAddJoiner(openthread_get_default_instance(), NULL, "FEDCBA9876543210",2000);
	}
}

static void on_thread_state_changed(otChangedFlags flags, struct openthread_context *ot_context,
				    void *user_data)
{
	if (flags & OT_CHANGED_THREAD_ROLE) {
		otDeviceRole ot_role = otThreadGetDeviceRole(ot_context->instance);
		if (ot_role != OT_DEVICE_ROLE_DETACHED && ot_role != OT_DEVICE_ROLE_DISABLED) {
			otCommissionerStart(ot_context->instance, &on_commissioner_state_changed, NULL, NULL);
		}
	}
}

static struct openthread_state_changed_cb ot_state_chaged_cb = {
	.state_changed_cb = on_thread_state_changed
};

int ot_initialization(void)
{
	struct openthread_context *context = openthread_get_default_context();

	otInstance *instance = openthread_get_default_instance();

	/* LOG_INF("Updating thread parameters"); */
	// ot_setNetworkConfiguration(instance);
	/* LOG_INF("Enabling thread"); */
	otError err = openthread_start(context); /* 'ifconfig up && thread start' */

	if (err != OT_ERROR_NONE) {
		LOG_ERR("Starting openthread: %d (%s)", err, otThreadErrorToString(err));
	}
	otDeviceRole current_role = otThreadGetDeviceRole(instance);

	LOG_INF("Current role of Thread device: %s", otThreadDeviceRoleToString(current_role));
	return 0;
}


int main(void)
{
#if DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_shell_uart), zephyr_cdc_acm_uart)
	int ret;
	const struct device *dev;
	uint32_t dtr = 0U;

	ret = usb_enable(NULL);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return 0;
	}

	dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_shell_uart));
	if (dev == NULL) {
		LOG_ERR("Failed to find specific UART device");
		return 0;
	}

	LOG_INF("Waiting for host to be ready to communicate");

	/* Data Terminal Ready - check if host is ready to communicate */
	while (!dtr) {
		ret = uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
		if (ret) {
			LOG_ERR("Failed to get Data Terminal Ready line state: %d",
				ret);
			continue;
		}
		k_msleep(100);
	}

	/* Data Carrier Detect Modem - mark connection as established */
	(void)uart_line_ctrl_set(dev, UART_LINE_CTRL_DCD, 1);
	/* Data Set Ready - the NCP SoC is ready to communicate */
	(void)uart_line_ctrl_set(dev, UART_LINE_CTRL_DSR, 1);
#endif

	LOG_INF(WELLCOME_TEXT);

#if defined(CONFIG_CLI_SAMPLE_MULTIPROTOCOL)
	ble_enable();
#endif

#if defined(CONFIG_CLI_SAMPLE_LOW_POWER)
	low_power_enable();
#endif

	ot_initialization();
	openthread_state_changed_cb_register(openthread_get_default_context(), &ot_state_chaged_cb);

	return 0;
}
