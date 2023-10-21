#ifndef LOGKA_H
#define LOGKA_H

// assume user included stdio.h

static const char * DEBUG_LABEL = "[\033[36m DEBUG\033[0m]: "; // Cyan
static const char * WARN_LABEL  = "[\033[33m  WARN\033[0m]: "; // Yellow
static const char * ERROR_LABEL = "[\033[31m ERROR\033[0m]: "; // Red
static const char * INFO_LABEL  = "[\033[34m  INFO\033[0m]: "; // Blue
static const char * OK_LABEL    = "[\033[32m    OK\033[0m]: "; // Green

static const char * GL_LABEL    = "[\033[35mOpenGL\033[0m]: "; // 

#ifdef SILENT
	#define debug(format, ...) ((void)0)
	#define  warn(format, ...) ((void)0)
	#define error(format, ...) ((void)0)
	#define  info(format, ...) ((void)0)
	#define    ok(format, ...) ((void)0)

	#define gldebug(format, ...) ((void)0)
#else
	#define debug(format, ...) fprintf(stdout, "%s" format "\n", DEBUG_LABEL, ##__VA_ARGS__)
	#define  warn(format, ...) fprintf(stdout, "%s" format "\n", WARN_LABEL , ##__VA_ARGS__)
	#define error(format, ...) fprintf(stdout, "%s" format "\n", ERROR_LABEL, ##__VA_ARGS__)
	#define  info(format, ...) fprintf(stdout, "%s" format "\n", INFO_LABEL , ##__VA_ARGS__)
	#define    ok(format, ...) fprintf(stdout, "%s" format "\n", OK_LABEL   , ##__VA_ARGS__)

	#define gldebug(format, ...) fprintf(stdout, "%s" format "\n", GL_LABEL, ##__VA_ARGS__)
#endif

#ifdef RELEASE
	#undef  debug
	#define debug(format, ...) ((void)0)
#endif

#endif // LOGKA_H
