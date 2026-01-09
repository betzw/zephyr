/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_npu

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/init.h>
#include <soc.h>

#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#ifdef CONFIG_STM32N6_ENABLE_NPU_CACHE
static void _npu_cache_axi_enable(void)
{
	/* Disable cache */
	*((__IO uint32_t *)(CACHEAXI_BASE_S)) = 0x0;
 
	k_busy_wait(5 * 1000); // 5ms delay

	/* Enable cache */
	*((__IO uint32_t *)(CACHEAXI_BASE_S)) = 0x1;

	/* Enable cache counters */
	*((__IO uint32_t *)(CACHEAXI_BASE_S)) |= 0x33330000;

	/* Reset cache counters */
	*((__IO uint32_t *)(CACHEAXI_BASE_S)) |= 0xcccc0000;

	/* Enable cache error interrupt */
	*((__IO uint32_t *)(CACHEAXI_BASE_S + 8)) = (1 << 2);
}
#endif /* CONFIG_STM32N6_ENABLE_NPU_CACHE */

/* Read-only driver configuration */
struct npu_stm32_cfg {
	/* Clock configuration. */
	struct stm32_pclken pclken_npu;
	struct stm32_pclken pclken_cacheaxi;
	/* Reset configuration */
	const struct reset_dt_spec reset_npu;
	const struct reset_dt_spec reset_cacheaxi;
};

static int npu_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct npu_stm32_cfg *cfg = dev->config;

	if (!device_is_ready(clk)) {
		return -ENODEV;
	}

	if (clock_control_on(clk, (clock_control_subsys_t) &cfg->pclken_npu) != 0) {
		return -EIO;
	}

	if (clock_control_on(clk, (clock_control_subsys_t) &cfg->pclken_cacheaxi) != 0) {
		return -EIO;
	}

	if (!device_is_ready(cfg->reset_npu.dev)) {
		return -ENODEV;
	}

	/* Reset timer to default state using RCC */
	(void)reset_line_toggle_dt(&cfg->reset_npu);
	(void)reset_line_toggle_dt(&cfg->reset_cacheaxi);

#ifdef CONFIG_STM32N6_ENABLE_NPU_CACHE
	_npu_cache_axi_enable();
#endif /* CONFIG_STM32N6_ENABLE_NPU_CACHE */

	return 0;
}


static const struct npu_stm32_cfg npu_stm32_cfg = {
	.pclken_npu = STM32_CLOCK_INFO_BY_NAME(DT_NODELABEL(npu), npu),
	.pclken_cacheaxi = STM32_CLOCK_INFO_BY_NAME(DT_NODELABEL(npu), cacheaxi),
	.reset_npu = RESET_DT_SPEC_GET_BY_IDX(DT_NODELABEL(npu), 0),
	.reset_cacheaxi = RESET_DT_SPEC_GET_BY_IDX(DT_NODELABEL(npu), 1),
};

DEVICE_DT_DEFINE(DT_NODELABEL(npu), npu_stm32_init, NULL,
		 NULL, &npu_stm32_cfg, POST_KERNEL,
		 CONFIG_APPLICATION_INIT_PRIORITY, NULL);
