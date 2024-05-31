#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#ifndef CREATESHM_H_
#define CREATESHM_H_
	int create_shm_file(void);
	int allocate_shm_file(size_t size);
#endif
