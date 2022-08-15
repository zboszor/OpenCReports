/*
 * OpenCReports header
 * Copyright (C) 2019-2022 Zoltán Böszörményi <zboszor@gmail.com>
 * See COPYING.LGPLv3 in the toplevel directory.
 */
#ifndef _FUNCTION_H_
#define _FUNCTION_H_

struct ocrpt_function {
	const char *fname;
	ocrpt_function_call func;
	int32_t n_ops;
	bool commutative:1;
	bool associative:1;
	bool left_associative:1;
	bool dont_optimize:1;
};

#endif
