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

#include <string.h>

#include "protocol.h"

/* Get a pointer to the first free byte in the buffer */
static uint8_t *tekbuf_tail(struct dev_context *devc)
{
	return &devc->tekbuf_rx[devc->tekbuf_num_in_rx];
}

static size_t tekbuf_num_unused(struct dev_context *devc)
{
	return devc->tekbuf_size_elems - devc->tekbuf_num_in_rx;
}

/* Purge num_bytes from the beginning of the RX buffer */
static void tekbuf_purge(struct dev_context *devc, size_t num_bytes)
{
	size_t num_purged = MIN(devc->tekbuf_num_in_rx, num_bytes);
	size_t num_remaining = devc->tekbuf_num_in_rx - num_purged;
	memmove(devc->tekbuf_rx, devc->tekbuf_rx + num_purged, num_remaining);
	devc->tekbuf_num_in_rx = num_remaining;
}

static size_t tekbuf_parse_header(struct dev_context *devc)
{
	size_t i, header_strlen;
	int num_expected;
	char lenstr[10];

	/* The header starts with a '#'. Anything else is garbage data */
	for (i = 0; i < devc->tekbuf_num_in_rx; i++)
		if (devc->tekbuf_rx[i] == '#')
			break;

	tekbuf_purge(devc, i);

	if (devc->tekbuf_num_in_rx < 2)
		return 0;

	header_strlen = devc->tekbuf_rx[1] - '0';

	if (header_strlen > 9) {
		sr_err("Received invalid length specifier");
		tekbuf_purge(devc, 2);
		return 0;
	}

	if (devc->tekbuf_num_in_rx < header_strlen + 2)
		return 0;

	strncpy(lenstr, (char *)devc->tekbuf_rx + 2, header_strlen);
	lenstr[header_strlen] = '\0';
	if (sr_atoi(lenstr, &num_expected) != SR_OK) {
		sr_err("Received invalid length string %s", lenstr);
		num_expected = 0;
	}

	tekbuf_purge(devc, 2 + header_strlen);
	return num_expected;
}

static size_t tek_send_ana(const struct sr_dev_inst *sdi, uint8_t *dat, size_t len)
{
	uint16_t sam, *dbuf;
	size_t i, wench = len >> 1;
	struct dev_context *devc;

	devc = sdi->priv;
	float *fart = devc->analog_buf;
	struct sr_datafeed_analog analog;
	struct sr_datafeed_packet packet;

	float yoff = 32768.0;
	float ymul = 1.5625E-3 * 50.0;

	for (i = 0; i < wench; i++) {
		dbuf = &dat[i << 1];
		sam = (dbuf[0] << 8) | dbuf[1];
		fart[i] = ((float)sam - yoff) * ymul;
	}
//    tcp-raw/192.168.2.21/4000
	analog.channels = g_slist_append(NULL, sdi->channels->data);
	analog.num_samples = wench;
	analog.data = fart;
	analog.mq = SR_MQ_VOLTAGE;
	analog.mqflags = 0;
	analog.unit = SR_UNIT_AMPERE;
	packet.type = SR_DF_ANALOG;
	packet.payload = &analog;

	sr_session_send(sdi, &packet);

	g_slist_free(analog.channels);

	return wench << 1;
}

static void request_frame(const struct sr_dev_inst *sdi)
{
	struct dev_context *devc = sdi->priv;
	struct sr_scpi_dev_inst *scpi = sdi->conn;

	sr_scpi_send(scpi, ":CURV?");
	devc->tekbuf_num_in_rx = devc->num_bytes_received = 0;
}

SR_PRIV int tektronix_4000_receive_data(int fd, int revents, void *cb_data)
{
	struct sr_dev_inst *sdi;
	struct dev_context *devc;
	struct sr_scpi_dev_inst *scpi;

	(void)fd;

	if (!(sdi = cb_data)) {
		sr_spew("boo");
		return TRUE;
	}

	if (!(devc = sdi->priv)) {
		sr_spew("BA");
		return TRUE;
	}

	scpi = sdi->conn;

	if (revents == G_IO_IN) {
		/* TODO */
	}

	if (devc->acq_state == ACQ_IDLE) {
		request_frame(sdi);
		devc->acq_state = ACQ_WAIT_HEADER;
		return TRUE;
	}

	switch (devc->acq_state) {
	case ACQ_WAIT_HEADER:
		devc->num_expected_bytes = tekbuf_parse_header(devc);
		if (!devc->num_expected_bytes)
			break;

		devc->habemus_packetum = devc->tekbuf_num_in_rx;
		sr_spew("Expecting a %zu bytes frame",
			devc->num_expected_bytes);
		devc->acq_state = ACQ_RECEIVING_DATA;
		struct sr_datafeed_packet packet;
		packet.type = SR_DF_FRAME_BEGIN;
		sr_session_send(sdi, &packet);
		break;
	case ACQ_RECEIVING_DATA:
		break;
	default:
		sr_spew("OOOPS Bad state machine!");
		return FALSE;
	}

	int yay;
	yay = sr_scpi_read_data(scpi, (char *)tekbuf_tail(devc),
				tekbuf_num_unused(devc));
	if (yay < 0)
		sr_err("MESSED UP %s (%i)", sr_strerror(yay), yay);

	if(yay) {
		//sr_spew("did %d", yay);
		devc->tekbuf_num_in_rx += yay;
		devc->habemus_packetum += yay;
	} else {
		sr_spew("nay");
	}


	if(devc->acq_state == ACQ_RECEIVING_DATA) {
		size_t handled;
		if (yay > 0) {
			handled = tek_send_ana(sdi, devc->tekbuf_rx, devc->tekbuf_num_in_rx);
			tekbuf_purge(devc, handled);
			devc->num_bytes_received += handled;
		}
	}

	if ((devc->acq_state == ACQ_RECEIVING_DATA) &&
	    (devc->num_bytes_received >= devc->num_expected_bytes)) {
		struct sr_datafeed_packet packet;
		packet.type = SR_DF_FRAME_END;
		sr_session_send(sdi, &packet);
		devc->num_frames_received++;
		devc->num_bytes_received = 0;
		devc->acq_state = ACQ_IDLE;
		sr_dbg("This like has had %zu baitz\n", devc->habemus_packetum);
	}

	if (devc->num_frames_received >= devc->num_frames) {
		sdi->driver->dev_acquisition_stop(sdi, cb_data);
		sr_dbg("EL ACQ es stopados %zu baitz\n", devc->habemus_packetum);
	}

	return TRUE;
}
