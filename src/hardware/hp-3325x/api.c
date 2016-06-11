/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2016 Alexandru Gagniuc <mr.nuke.me@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <scpi.h>
#include "protocol.h"

static struct sr_dev_driver hp_3325x_driver_info;

static const uint32_t scanopts[] = {
	SR_CONF_CONN,
	SR_CONF_SERIALCOMM,
};

static const uint32_t drvopts[] = {
	SR_CONF_FUNCTION_GEN,
};

static const uint32_t devopts[] = {
	SR_CONF_OUTPUT_FREQUENCY_TARGET | SR_CONF_SET | SR_CONF_GET,
};

static struct sr_dev_inst *probe_device(struct sr_scpi_dev_inst *scpi)
{
	int ret;
	char *response;
	struct sr_dev_inst *sdi;
	struct dev_context *devc;

	/* FIXME: Use better way to probe this. */
	ret = sr_scpi_get_string(scpi, "IFU", &response);
	if ((ret != SR_OK) || !response)
		return NULL;

	/* FIXME: Actually check the response. */

	/* FIXME: differentiate between 3325A and 3325B. */
	devc = g_malloc0(sizeof(struct dev_context));
	sdi = g_malloc0(sizeof(struct sr_dev_inst));
	sdi->vendor = g_strdup("Hewlett-Packard");
	/* FIXME: differentiate between 3325A and 3325B. */
	sdi->model = g_strdup("3325A");
	////sdi->version = get_revision(scpi);
	sdi->conn = scpi;
	sdi->driver = &hp_3325x_driver_info;
	sdi->inst_type = SR_INST_SCPI;
	sdi->priv = devc;

	return sdi;

}

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
	return sr_scpi_scan(di->context, options, probe_device);
}

static int dev_clear(const struct sr_dev_driver *di)
{
	return std_dev_clear(di, NULL);
}

static int dev_open(struct sr_dev_inst *sdi)
{
	struct sr_scpi_dev_inst *scpi = sdi->conn;

	if (sr_scpi_open(scpi) != SR_OK)
		return SR_ERR;

	sdi->status = SR_ST_ACTIVE;

	return SR_OK;
}

static int dev_close(struct sr_dev_inst *sdi)
{
	struct sr_scpi_dev_inst *scpi = sdi->conn;

	if (sdi->status != SR_ST_ACTIVE)
		return SR_ERR_DEV_CLOSED;

	sr_scpi_close(scpi);

	sdi->status = SR_ST_INACTIVE;

	return SR_OK;
}

static int config_get(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	int ret;
	double resp_data;

	sr_spew("ZOPA: %s", __func__);

	(void)cg;

	ret = SR_OK;
	switch (key) {
	/* TODO */
	case SR_CONF_OUTPUT_FREQUENCY_TARGET:
		hp_3325x_query_freq(sdi->conn, &resp_data);
		*data = g_variant_new_double(resp_data);
		break;
	default:
		return SR_ERR_NA;
	}

	return ret;
}

static int config_set(uint32_t key, GVariant *data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	int ret;

	(void)data;
	(void)cg;

	if (sdi->status != SR_ST_ACTIVE)
		return SR_ERR_DEV_CLOSED;

	ret = SR_OK;
	switch (key) {
	/* TODO */
	default:
		ret = SR_ERR_NA;
	}

	return ret;
}

static int config_list(uint32_t key, GVariant **data,
	const struct sr_dev_inst *sdi, const struct sr_channel_group *cg)
{
	int ret;
	(void)cg;

	if (key == SR_CONF_SCAN_OPTIONS) {
		*data = g_variant_new_fixed_array(G_VARIANT_TYPE_UINT32,
						  scanopts, ARRAY_SIZE(scanopts), sizeof(uint32_t));
		return SR_OK;
	} else if ((key == SR_CONF_DEVICE_OPTIONS) && !sdi) {
		*data = g_variant_new_fixed_array(G_VARIANT_TYPE_UINT32,
						  drvopts, ARRAY_SIZE(drvopts), sizeof(uint32_t));
		return SR_OK;
	} else if ((key == SR_CONF_DEVICE_OPTIONS) && !cg) {
		sr_spew("ZOPAFUCKA");
		*data = g_variant_new_fixed_array(G_VARIANT_TYPE_UINT32,
						  devopts, ARRAY_SIZE(devopts), sizeof(uint32_t));
		return SR_OK;
	}

	ret = SR_OK;
	switch (key) {
	/* TODO */
	default:
		return SR_ERR_NA;
	}

	return ret;
}

static int dev_acquisition_start(const struct sr_dev_inst *sdi)
{
	(void)sdi;
	/* We can only set output parameters, but not measure them. */
	return SR_ERR_NA;
}

static int dev_acquisition_stop(struct sr_dev_inst *sdi)
{
	if (sdi->status != SR_ST_ACTIVE)
		return SR_ERR_DEV_CLOSED;

	/* TODO: stop acquisition. */

	return SR_OK;
}

static struct sr_dev_driver hp_3325x_driver_info = {
	.name = "hp-3325x",
	.longname = "HP 3325x",
	.api_version = 1,
	.init = std_init,
	.cleanup = std_cleanup,
	.scan = scan,
	.dev_list = std_dev_list,
	.dev_clear = dev_clear,
	.config_get = config_get,
	.config_set = config_set,
	.config_list = config_list,
	.dev_open = dev_open,
	.dev_close = dev_close,
	.dev_acquisition_start = dev_acquisition_start,
	.dev_acquisition_stop = dev_acquisition_stop,
	.context = NULL,
};

SR_REGISTER_DEV_DRIVER(hp_3325x_driver_info);
