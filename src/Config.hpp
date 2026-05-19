#ifndef MINICDI_CONFIG
#define MINICDI_CONFIG

typedef struct {
	bool pal;
	bool lcd;
	int lines = 384;
	FILE* log;
} MiniCDIConfig;

#endif