#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
int fac(void)
{
	// Please tweak the iteration counts to make this calculation run long enough
	volatile long long unsigned int sum = 0;
	for (int i = 0; i < 100000000; i++) {
		volatile long long unsigned int fac = 1;
		for (int j = 1; j <= 50; j++) {
			fac *= j;
		}
		sum += fac;
	}
	return 0;
}

int write_to_proc(int pid)
{
	FILE *file = fopen("/proc/mp1/status", "w");
	if (file == NULL) {
		perror("error: unable to open file");
		return EXIT_FAILURE;
	}

	fprintf(file, "%d", pid);
        fflush(file);
	fclose(file);
	return 0;
}

int read_from_proc(void)
{
        char buffer[256];
        int pid;
	FILE *file = fopen("/proc/mp1/status", "r");
	if (file == NULL) {
		perror("error: unable to open file");
		return EXIT_FAILURE;
	}
        
    printf("starting read form /proc/status/mp1\n");

    /* read till eof and add pid to buffer */    
    while (fgets(buffer, sizeof(buffer), file)) {
        if (sscanf(buffer, "PID: %d", &pid) == 1) {
            printf("PID: %d\n", pid);
        }
    }
   
    fclose(file);
    return EXIT_SUCCESS;

}

int main(void) {
    
    int pid = getpid();

    write_to_proc(pid);

    int fac();

    read_from_proc();
}
