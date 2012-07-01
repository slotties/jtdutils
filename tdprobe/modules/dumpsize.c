#include <stdio.h>

#include "list.h"
#include "td.h"
#include "conf.h"
#include "probemodule.h"

module_t INFO = { "Dumpsize", false };

module_t* get_module() {
	return &INFO;
}

presult probe(td_dump* dump, conf* config, char* comment_buffer) {
	int avg = conf_geti_def(config, "dumpsize.avg", 500);
	float tolerance = conf_getf_def(config, "dumpsize.avgTolerance", 0.1);
	float errTolerance = conf_getf_def(config, "dumpsize.avgErrorTolerance", 0.2);
	
	int avgMin = avg * (1.0 - tolerance);
	int avgErrorMin = avg * (1.0 - errTolerance);

	int avgMax = avg * (1.0 + tolerance);
	int avgErrorMax = avg * (1.0 + errTolerance);

	int l = list_len(dump->threads);
	if (l < avgErrorMin) {
		sprintf(comment_buffer, "There are way too few threads (%d).", l);
		return R_BAD;
	} else if (l < avgMin) {
		sprintf(comment_buffer, "There are fewer threads than expected (%d).", l);
		return R_SUSPICIOUS;
	} else if (l > avgErrorMax) {
		sprintf(comment_buffer, "There are way too many threads (%d).", l);
		return R_BAD;
	} else if (l > avgMax) {
		sprintf(comment_buffer, "There are more threads than expected (%d).", l);
		return R_SUSPICIOUS;
	} else {
		return R_OK;
	}
}
