
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

void busy_work(double seconds)
{
	clock_t start = clock();
	while ((double)(clock() - start) / CLOCKS_PER_SEC < seconds) {
		volatile double x = 1.0;
		for (int i = 0; i < 1000000; i++) {
			x *= 1.0000001; 		}
	}
}


int write_to_proc(int pid)
{
	FILE *file = fopen("/proc/mp1/status", "w");
	if (file == NULL) {
		perror("error: unable to open file");
		return EXIT_FAILURE;
	}

	fprintf(file, "%d\n", pid);
	fflush(file);
	fclose(file);
	return 0;
}

int read_from_proc(void)
{
	char buffer[256];
	int pid;
	unsigned long cpu_time;
	FILE *file = fopen("/proc/mp1/status", "r");
	if (file == NULL) {
		perror("error: unable to open file");
		return EXIT_FAILURE;
	}

	// printf("starting read form /proc/status/mp1\n");

	/* read till eof and add pid to buffer */
	while (fgets(buffer, sizeof(buffer), file)) {
		if (sscanf(buffer, "%d: %lu", &pid, &cpu_time) == 2)
			printf("PID: %d, CPU Time: %lu\n", pid, cpu_time);
	}

	fclose(file);
	return EXIT_SUCCESS;
}

int main(void)
{
	int pid = getpid();

	write_to_proc(pid);

	busy_work(15);
	read_from_proc();
}
