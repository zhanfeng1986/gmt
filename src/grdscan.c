/*--------------------------------------------------------------------
 *
 *	Copyright (c) 1991-2020 by the GMT Team (https://www.generic-mapping-tools.org/team.html)
 *	See LICENSE.TXT file for copying and redistribution conditions.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU Lesser General Public License as published by
 *	the Free Software Foundation; version 3 or any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU Lesser General Public License for more details.
 *
 *	Contact info: www.generic-mapping-tools.org
 *--------------------------------------------------------------------*/
/*
 * API functions to support the gmtconvert application.
 *
 * Author:	Walter H.F. Smith
 * Date:	1-JAN-2010
 * Version:	6 API
 *
 * Brief synopsis: Read a grid file and determine one point per row (or er col),
 * taht satisfies a test, e.g., max value.
 */

#include "gmt_dev.h"

#define THIS_MODULE_CLASSIC_NAME	"grdscan"
#define THIS_MODULE_MODERN_NAME	"grdscan"
#define THIS_MODULE_LIB		"core"
#define THIS_MODULE_PURPOSE	"Extract unique points per row or col"
#define THIS_MODULE_KEYS	"<G{,DD}"
#define THIS_MODULE_NEEDS	""
#define THIS_MODULE_OPTIONS "-RV"

struct GRDSCAN_CTRL {
	struct In {
		bool active;
		char *file;
	} In;
	struct S {	/* -Sx|y */
		bool active;
		unsigned int mode;

	} S;
};

GMT_LOCAL void *New_Ctrl (struct GMT_CTRL *GMT) {	/* Allocate and initialize a new control structure */
	struct GRDSCAN_CTRL *C;

	C = gmt_M_memory (GMT, NULL, 1, struct GRDSCAN_CTRL);

	/* Initialize values whose defaults are not 0/false/NULL */

	return (C);
}

GMT_LOCAL void Free_Ctrl (struct GMT_CTRL *GMT, struct GRDSCAN_CTRL *C) {	/* Deallocate control structure */
	if (!C) return;
	gmt_M_str_free (C->In.file);
	gmt_M_free (GMT, C);
}

GMT_LOCAL int usage (struct GMTAPI_CTRL *API, int level) {
	const char *name = gmt_show_name_and_purpose (API, THIS_MODULE_LIB, THIS_MODULE_CLASSIC_NAME, THIS_MODULE_PURPOSE);
	if (level == GMT_MODULE_PURPOSE) return (GMT_NOERROR);
	GMT_Message (API, GMT_TIME_NONE, "usage: %s <ingrid> [%s] [-Sx|y] [%s] [%s]\n", name, GMT_Rgeo_OPT, GMT_V_OPT, GMT_PAR_OPT);

	if (level == GMT_SYNOPSIS) return (GMT_MODULE_SYNOPSIS);

	GMT_Message (API, GMT_TIME_NONE, "\n\t<ingrid> is a single grid file.\n");
	GMT_Message (API, GMT_TIME_NONE, "\n\tOPTIONS:\n");
	GMT_Option (API, "R");
	GMT_Message (API, GMT_TIME_NONE, "\t-Sx will scan across rows.\n");
	GMT_Message (API, GMT_TIME_NONE, "\t-Sy will scan across columns.\n");

	GMT_Option (API, "V,.");

	return (GMT_MODULE_USAGE);
}

GMT_LOCAL int parse (struct GMT_CTRL *GMT, struct GRDSCAN_CTRL *Ctrl, struct GMT_OPTION *options) {
	/* This parses the options provided to grdcut and sets parameters in CTRL.
	 * Any GMT common options will override values set previously by other commands.
	 * It also replaces any file names specified as input or output with the data ID
	 * returned when registering these sources/destinations with the API.
	 */

	unsigned int n_errors = 0, n_files = 0;
	struct GMT_OPTION *opt = NULL;
	struct GMTAPI_CTRL *API = GMT->parent;

	for (opt = options; opt; opt = opt->next) {	/* Process all the options given */

		switch (opt->option) {

			case '<':	/* Input file (only one is accepted) */
				if (n_files++ > 0) break;
				if ((Ctrl->In.active = gmt_check_filearg (GMT, '<', opt->arg, GMT_IN, GMT_IS_GRID)))
					Ctrl->In.file = strdup (opt->arg);
				else
					n_errors++;
				break;

			/* Processes program-specific parameters */

			case 'S':	/* Set limits */
				Ctrl->S.active = true;
				switch (opt->arg[0]) {
					case 'x':
						Ctrl->S.mode = GMT_X; break;
					case 'y':
						Ctrl->S.mode = GMT_Y;
						break;
					default:
						GMT_Report (API, GMT_MSG_ERROR, "Option -S: Expected -Sx or -Sy\n");
						n_errors++;
						break;
				}
				break;

			default:	/* Report bad options */
				n_errors += gmt_default_error (GMT, opt->option);
				break;
		}
	}

	n_errors += gmt_M_check_condition (GMT, n_files != 1, "Must specify a single grid file\n");

	return (n_errors ? GMT_PARSE_ERROR : GMT_NOERROR);
}

#define bailout(code) {gmt_M_free_options (mode); return (code);}
#define Return(code) {Free_Ctrl (GMT, Ctrl); gmt_end_module (GMT, GMT_cpy); bailout (code);}

int GMT_grdscan (void *V_API, int mode, void *args) {
	unsigned int row, col, r, c;
	int error = 0;

	uint64_t ij;

	double wesn[4], out[3], *x = NULL, *y = NULL;

	float z;

	struct GRDSCAN_CTRL *Ctrl = NULL;
	struct GMT_GRID *G = NULL;
	struct GMT_RECORD *Out = NULL;
	struct GMT_CTRL *GMT = NULL, *GMT_cpy = NULL;
	struct GMT_OPTION *options = NULL;
	struct GMTAPI_CTRL *API = gmt_get_api_ptr (V_API);	/* Cast from void to GMTAPI_CTRL pointer */

	/*----------------------- Standard module initialization and parsing ----------------------*/

	if (API == NULL) return (GMT_NOT_A_SESSION);
	if (mode == GMT_MODULE_PURPOSE) return (usage (API, GMT_MODULE_PURPOSE));	/* Return the purpose of program */
	options = GMT_Create_Options (API, mode, args);	if (API->error) return (API->error);	/* Set or get option list */

	if ((error = gmt_report_usage (API, options, 0, usage)) != GMT_NOERROR) bailout (error);	/* Give usage if requested */

	/* Parse the command-line arguments */

	if ((GMT = gmt_init_module (API, THIS_MODULE_LIB, THIS_MODULE_CLASSIC_NAME, THIS_MODULE_KEYS, THIS_MODULE_NEEDS, NULL, &options, &GMT_cpy)) == NULL) bailout (API->error); /* Save current state */
	if (GMT_Parse_Common (API, THIS_MODULE_OPTIONS, options)) Return (API->error);
	Ctrl = New_Ctrl (GMT);	/* Allocate and initialize a new control structure */
	if ((error = parse (GMT, Ctrl, options)) != 0) Return (error);

	/*---------------------------- This is the grdscan main code ----------------------------*/

	GMT_Report (API, GMT_MSG_INFORMATION, "Processing input grid\n");
	gmt_M_memcpy (wesn, GMT->common.R.wesn, 4, double);	/* Current -R setting, if any */

	if ((G = GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_CONTAINER_ONLY, NULL, Ctrl->In.file, NULL)) == NULL) {
		Return (API->error);
	}
	if (gmt_M_is_subset (GMT, G->header, wesn)) gmt_M_err_fail (GMT, gmt_adjust_loose_wesn (GMT, wesn, G->header), "");	/* Subset requested; make sure wesn matches header spacing */
	if (GMT_Read_Data (API, GMT_IS_GRID, GMT_IS_FILE, GMT_IS_SURFACE, GMT_DATA_ONLY, wesn, Ctrl->In.file, G) == NULL) {
		Return (API->error);	/* Get subset */
	}

	if ((error = GMT_Set_Columns (API, GMT_OUT, 3, GMT_COL_FIX_NO_TEXT)) != GMT_NOERROR) Return (error);

	if (GMT_Init_IO (API, GMT_IS_DATASET, GMT_IS_POINT, GMT_OUT, GMT_ADD_DEFAULT, 0, options) != GMT_NOERROR) {	/* Registers stdout, unless already set */
		Return (API->error);
	}
	if (GMT_Begin_IO (API, GMT_IS_DATASET, GMT_OUT, GMT_HEADER_ON) != GMT_NOERROR) {	/* Enables data output and sets access mode */
		Return (API->error);
	}
	if (GMT_Set_Geometry (API, GMT_OUT, GMT_IS_POINT) != GMT_NOERROR) {	/* Sets output geometry */
		Return (API->error);
	}

	x = gmt_grd_coord (GMT, G->header, GMT_X);
	y = gmt_grd_coord (GMT, G->header, GMT_Y);

	Out = gmt_new_record (GMT, out, NULL);	/* Since we only need to worry about numerics in this module */

	if (Ctrl->S.mode == GMT_X) {
		gmt_M_row_loop (GMT, G, row) {	/* Along each row */
			out[GMT_Y] = y[row];
			z = -FLT_MAX;
			gmt_M_col_loop (GMT, G, row, col, ij) {	/* Loop across this row */
				if (gmt_M_is_fnan (G->data[ij])) continue;
				if (G->data[ij] > z) {
					z = G->data[ij];
					c = col;
				}
			}
			out[GMT_X] = x[c];
			out[GMT_Z] = z;
			if (z != -FLT_MAX) GMT_Put_Record (API, GMT_WRITE_DATA, Out);		/* Write this to output */
		}
	}
	else {
		gmt_M_col_loop2 (GMT, G, col) {	/* For all possible columns */
			out[GMT_X] = x[col];
			z = -FLT_MAX;
			gmt_M_row_loop (GMT, G, row) {	/* Loop over all rows */
				ij = gmt_M_ijp (G->header, row, col);
				if (gmt_M_is_fnan (G->data[ij])) continue;
				if (G->data[ij] > z) {
					z = G->data[ij];
					r = row;
				}
			}
			out[GMT_Y] = y[r];
			out[GMT_Z] = z;
			if (z != -FLT_MAX) GMT_Put_Record (API, GMT_WRITE_DATA, Out);		/* Write this to output */
		}
	}

	if (GMT_End_IO (API, GMT_OUT, 0) != GMT_NOERROR) {	/* Disables further data output */
		Return (API->error);
	}

	gmt_M_free (GMT, x);
	gmt_M_free (GMT, y);
	gmt_M_free (GMT, Out);

	Return (GMT_NOERROR);
}
