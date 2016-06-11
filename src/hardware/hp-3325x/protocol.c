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
#include <stdio.h>
#include "protocol.h"

static double freq_unit_to_multiplier(const char *unit)
{
	if (!strcmp(unit, "HZ"))
		return 1.0E0;
	else if (!strcmp(unit, "KH"))
		return 1.0E3;
	else if (!strcmp(unit, "MH"))
		return 1.0E6;
	else
		return 1.0;
}

SR_PRIV int hp_3325x_query_freq(struct sr_scpi_dev_inst *scpi, double *freq)
{
	int ret;
	char *response;
	char func[4], unit[4];
	double val;

	ret = sr_scpi_get_string(scpi, "IFR", &response);
	if ((ret != SR_OK) || !response)
		return SR_ERR_IO;

	ret = sscanf(response, "%2s%lf%2s", func, &val, unit);

	if (ret != 3)
		return SR_ERR_IO;

	if (!strcmp(func, "FR"))
		return SR_ERR_IO;

	sr_spew("Got funx %s, unit %s, with value %lf", func, unit, val);

	*freq = val;

	return SR_OK;
}