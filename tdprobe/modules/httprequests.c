#include <stdio.h>
#include <string.h>

#include "list.h"
#include "td.h"
#include "conf.h"
#include "probemodule.h"

module_t INFO = { "HTTP Requests", false };

module_t* get_module() {
	return &INFO;
}

static bool isHttpRequestThread(const char* name) {
	return (strstr(name, "http-") == name || 
		strstr(name, "AJPRequest") == name);
}

int probe(td_dump* dump, conf* config, char* comment_buffer) {
	int maxCount = conf_geti_def(config, "requests.max", 0);
	int count = 0;
	
	for (list* threads = dump->threads; threads; threads = threads->next) {
		char* threadName = ((td_thread*) threads->data)->name;
		if (isHttpRequestThread(threadName)) {
			count++;
		}
	}

	if (count > maxCount) {
		sprintf(comment_buffer, "Too many http-requests: %d (max %d).", count, maxCount);
		return R_BAD;
	} else {
		return R_SUSPICIOUS;
	}
}
