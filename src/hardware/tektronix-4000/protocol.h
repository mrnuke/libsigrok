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

#ifndef LIBSIGROK_HARDWARE_TEKTRONIX_4000_PROTOCOL_H
#define LIBSIGROK_HARDWARE_TEKTRONIX_4000_PROTOCOL_H

#include <stdint.h>
#include <glib.h>
#include "libsigrok.h"
#include "libsigrok-internal.h"

#define LOG_PREFIX "tektronix-4000"

enum acq_state {
	ACQ_IDLE,
	ACQ_WAIT_HEADER,
	ACQ_RECEIVING_DATA,
};

/** Private, per-device-instance driver context. */
struct dev_context {
	/* Model-specific information */

	/* Acquisition settings */
	size_t samples_per_frame;
	uint64_t num_frames;

	/* Operational state */
	size_t num_bytes_received;
	float *analog_buf;
	uint8_t *tekbuf_rx;
	size_t tekbuf_size_elems;
	size_t tekbuf_num_in_rx;
	size_t num_expected_bytes;
	size_t habemus_packetum;
	uint64_t num_frames_received;
	enum acq_state acq_state;

	/* Temporary state across callbacks */

};

SR_PRIV int tektronix_4000_receive_data(int fd, int revents, void *cb_data);

#endif
