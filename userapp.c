#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
// int fac(void)
// {
// 	// Please tweak the iteration counts to make this calculation run long enough
// 	volatile long long unsigned int sum = 0;
// 	for (int i = 0; i < 100000000; i++) {
// 		volatile long long unsigned int fac = 1;
// 		for (int j = 1; j <= 50; j++) {
// 			fac *= j;
// 		}
// 		sum += fac;
// 	}
// 	return 0;
// }

long long fib (int n) {
	if (n <= 1) return n;
	return fib (n - 1) + fib(n - 2);
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

	printf("starting read form /proc/status/mp1\n");

	/* read till eof and add pid to buffer */
	while (fgets(buffer, sizeof(buffer), file)) {
		if (sscanf(buffer, "%d: %lu", &pid, &cpu_time) == 2)
			printf("%d: %lu\n", pid, cpu_time);
	}

	fclose(file);
	return EXIT_SUCCESS;
}

int main(void)
{
	int pid = getpid();

	write_to_proc(pid);

	// fac();
	fib(45);
	read_from_proc();

	printf("Done.\n");
	return 0;
}
