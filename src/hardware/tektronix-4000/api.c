/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2015 Google, Inc.
 * (Written by Alexandru Gagniuc <mrnuke@google.com> for Google, Inc.)
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

#include "protocol.h"

#include <string.h>

SR_PRIV struct sr_dev_driver tektronix_4000_driver_info;

static const uint32_t scanopts[] = {
	SR_CONF_CONN,
	SR_CONF_SERIALCOMM,
};

static const uint32_t drvopts[] = {
	SR_CONF_OSCILLOSCOPE,
	SR_CONF_LIMIT_FRAMES | SR_CONF_SET,
};

static const uint32_t devopts[] = {
	SR_CONF_LIMIT_FRAMES | SR_CONF_SET,
	SR_CONF_SAMPLERATE | SR_CONF_GET | SR_CONF_SET,
};

static struct sr_dev_inst *probe_device(struct sr_scpi_dev_inst *scpi)
{
	struct dev_context *devc = NULL;
	struct sr_dev_inst *sdi = NULL;
	struct sr_scpi_hw_info *hw_info = NULL;

	if (sr_scpi_get_hw_id(scpi, &hw_info) != SR_OK) {
		sr_info("Couldn't get IDN response.");
		goto not_found;
	}

	//sr_dbg(" %s, %s, %s, %s", hw_info->manufacturer, hw_info->model,
	//	hw_info->serial_number, hw_info->firmware_version);
	if (strcmp(hw_info->manufacturer, "Tektronix"))
		goto not_found;

	sdi = g_malloc0(sizeof(struct sr_dev_inst));
	sdi->status = SR_ST_INACTIVE;
	sdi->vendor = g_strdup(hw_info->manufacturer);
	sdi->model = g_strdup(hw_info->model);
	sdi->version = g_strdup(hw_info->firmware_version);
	sdi->conn = scpi;
	sdi->driver = &tektronix_4000_driver_info;
	sdi->inst_type = SR_INST_SCPI;
	sdi->serial_num = g_strdup(hw_info->serial_number);

	struct sr_channel *ch;
	ch = sr_channel_new(sdi, 0, SR_CHANNEL_ANALOG, TRUE, "CHxx");
	sdi->channels = g_slist_append(NULL, ch);

	devc = g_malloc0(sizeof(struct dev_context));
	devc->tekbuf_rx = g_malloc(1075);
	devc->analog_buf = g_malloc(1075 * sizeof(*devc->analog_buf));
	devc->tekbuf_size_elems = 1075;
	sdi->priv = devc;

	sr_scpi_hw_info_free(hw_info);
	return sdi;

not_found:
	sr_scpi_hw_info_free(hw_info);
	if (sdi)
		sr_dev_inst_free(sdi);
	if (devc)
		g_free(devc);
	return NULL;
}

static int init(struct sr_dev_driver *di, struct sr_context *sr_ctx)
{
	return std_init(sr_ctx, di, LOG_PREFIX);
}

static GSList *scan(struct sr_dev_driver *di, GSList *options)
{
	return sr_scpi_scan(di->priv, options, probe_device);
}

static GSList *dev_list(const struct sr_dev_driver *di)
{
	sr_spew("%s", __func__);
	return ((struct drv_context *)(di->priv))->instances;
}

static int dev_clear(const struct sr_dev_driver *di)
{
	sr_spew("%s", __func__);
	(void)di;
	//return std_dev_clear(di, NULL);
	return SR_OK;
}

static int dev_open(struct sr_dev_inst *sdi)
{
	struct sr_scpi_dev_inst *scpi;

	sr_spew("%s", __func__);
	if (sdi->status != SR_ST_INACTIVE)
		return SR_ERR;

	scpi = sdi->conn;
	if (sr_scpi_open(scpi) < 0) {
		sr_spew("%scpi open bad mojo");
		return SR_ERR;
	}

	sdi->status = SR_ST_ACTIVE;
	//scpi_cmd(sdi, SCPI_CMD_REMOTE);

	return SR_OK;
}

static int dev_close(struct sr_dev_inst *sdi)
{
	struct sr_scpi_dev_inst *scpi;

	sr_spew("%s", __func__);
	if (sdi->status != SR_ST_ACTIVE)
		return SR_ERR_DEV_CLOSED;

	scpi = sdi->conn;
	if (scpi) {
		sr_scpi_close(scpi);
		sdi->status = SR_ST_INACTIVE;
	}

	return SR_OK;
}

static int cleanup(const struct sr_dev_driver *di)
{
	dev_clear(di);

	sr_spew("%s", __func__);
	/* TODO: free other driver resources, if any. */

	return SR_OK;
}

static int config_get(uint32_t key, GVariant **data, const struct sr_dev_inst *sdi,
		const struct sr_channel_group *cg)
{
	int ret;

	sr_spew("%s", __func__);
	(void)sdi;
	(void)data;
	(void)cg;

	ret = SR_OK;
	switch (key) {
	case SR_CONF_SAMPLERATE:
		*data = g_variant_new_uint64(20000000);
		break;
	/* TODO */
	default:
		return SR_ERR_NA;
	}

	return ret;
}

static int config_set(uint32_t key, GVariant *data, const struct sr_dev_inst *sdi,
		const struct sr_channel_group *cg)
{
	int ret;
	struct dev_context *devc;

	sr_spew("%s", __func__);
	(void)cg;

	if (sdi->status != SR_ST_ACTIVE)
		return SR_ERR_DEV_CLOSED;

	if (!(devc = sdi->priv))
		return SR_ERR_ARG;

	ret = SR_OK;
	switch (key) {
	case SR_CONF_LIMIT_FRAMES:
		devc->num_frames = g_variant_get_uint64(data);
		break;
	case SR_CONF_SAMPLERATE:
		break;
	/* TODO */
	default:
		ret = SR_ERR_NA;
	}

	return ret;
}

static int config_list(uint32_t key, GVariant **data, const struct sr_dev_inst *sdi,
		const struct sr_channel_group *cg)
{
	(void)sdi;
	(void)cg;

	sr_spew("%s", __func__);
	/* Always available, even without sdi. */
	if (key == SR_CONF_SCAN_OPTIONS) {
		*data = g_variant_new_fixed_array(G_VARIANT_TYPE_UINT32,
				scanopts, ARRAY_SIZE(scanopts), sizeof(uint32_t));
		return SR_OK;
	} else if (key == SR_CONF_DEVICE_OPTIONS && !sdi) {
		*data = g_variant_new_fixed_array(G_VARIANT_TYPE_UINT32,
				drvopts, ARRAY_SIZE(drvopts), sizeof(uint32_t));
		return SR_OK;
	}
	switch (key) {
	case SR_CONF_DEVICE_OPTIONS:
		*data = g_variant_new_fixed_array(G_VARIANT_TYPE_UINT32,
				devopts, ARRAY_SIZE(devopts), sizeof(uint32_t));
		return SR_OK;
	/* TODO */
	default:
		return SR_ERR_NA;
	}

	return SR_ERR_NA;
}

static int dev_acquisition_start(const struct sr_dev_inst *sdi, void *cb_data)
{
	struct dev_context *devc;

	(void)cb_data;

	sr_spew("%s", __func__);
	if (sdi->status != SR_ST_ACTIVE)
		return SR_ERR_DEV_CLOSED;

	devc = sdi->priv;

	/* TODO: configure hardware, reset acquisition state, set up
	 * callbacks and send header packet. */

	struct sr_scpi_dev_inst *scpi;
	int ret;

	if (sdi->status != SR_ST_ACTIVE)
		return SR_ERR_DEV_CLOSED;

	scpi = sdi->conn;
	//devc->cb_data = cb_data;
	devc->acq_state = ACQ_IDLE;

	if ((ret = sr_scpi_source_add(sdi->session, scpi, G_IO_IN, 10,
			tektronix_4000_receive_data, (void *)sdi)) != SR_OK)
		return ret;

	std_session_send_df_header(sdi, LOG_PREFIX);
	devc->num_bytes_received = 0;
	devc->num_frames_received = 0;
	devc->tekbuf_num_in_rx = 0;

	/* 16-bit samples; binary encoding; as positive integers; MSB order */
	sr_scpi_send(scpi, ":WFMO:BYT_NR 2;ENCDG BIN;BN_FMT RP;BYT_OR MSB");

	/* Prime the pipe with the first channel's fetch. */
	/*ch = next_enabled_channel(sdi, NULL);
	pch = ch->priv;
	if ((ret = select_channel(sdi, ch)) != SR_OK)
		return ret;
	if (pch->mq == SR_MQ_VOLTAGE)
		cmd = SCPI_CMD_GET_MEAS_VOLTAGE;
	else if (pch->mq == SR_MQ_FREQUENCY)
		cmd = SCPI_CMD_GET_MEAS_FREQUENCY;
	else if (pch->mq == SR_MQ_CURRENT)
		cmd = SCPI_CMD_GET_MEAS_CURRENT;
	else if (pch->mq == SR_MQ_POWER)
		cmd = SCPI_CMD_GET_MEAS_POWER;
	else
		return SR_ERR;
	scpi_cmd(sdi, cmd, pch->hwname);
	*/
	return SR_OK;
}

static int dev_acquisition_stop(struct sr_dev_inst *sdi, void *cb_data)
{
	//struct dev_context *devc;
	struct sr_scpi_dev_inst *scpi;
	struct sr_datafeed_packet packet;

	(void)cb_data;

	//devc = sdi->priv;

	if (sdi->status != SR_ST_ACTIVE) {
		sr_err("Device inactive, can't stop acquisition.");
		return SR_ERR;
	}

	/* End of last frame. */
	packet.type = SR_DF_END;
	sr_session_send(sdi, &packet);

	//g_slist_free(devc->enabled_channels);
	//devc->enabled_channels = NULL;
	scpi = sdi->conn;
	sr_scpi_source_remove(sdi->session, scpi);

	return SR_OK;
}

SR_PRIV struct sr_dev_driver tektronix_4000_driver_info = {
	.name = "tektronix-4000",
	.longname = "Tektronix 4000",
	.api_version = 1,
	.init = init,
	.cleanup = cleanup,
	.scan = scan,
	.dev_list = dev_list,
	.dev_clear = dev_clear,
	.config_get = config_get,
	.config_set = config_set,
	.config_list = config_list,
	.dev_open = dev_open,
	.dev_close = dev_close,
	.dev_acquisition_start = dev_acquisition_start,
	.dev_acquisition_stop = dev_acquisition_stop,
	.priv = NULL,
};
