#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

int main() {
    float probs[] = {0.05, 0.10, 0.15, 0.20, 0.25, 0.30, 0.35, 0.40, 0.45, 0.50};
    int num_probs = sizeof(probs) / sizeof(probs[0]);
    char command[256];

    printf("Starting Automated KTP Testing\n");

    for (int i = 0; i < num_probs; i++) {
        printf("[%d] Testing with DROP_PROB = %.2f\n",i, probs[i]);

        snprintf(command, sizeof(command), "sed -i 's/#define DROP_PROB .*/#define DROP_PROB %.2f/' ksocket.h", probs[i]);
        system(command);

        system("gcc -c ksocket.c -o ksocket.o");
        system("ar rcs libksocket.a ksocket.o");
        system("gcc initksocket.c -o initksocket -L. -lksocket -lpthread");
        system("gcc user1.c -o user1 -L. -lksocket -lpthread");
        system("gcc user2.c -o user2 -L. -lksocket -lpthread");

        pid_t init_pid = fork();
        if (init_pid == 0) {
            freopen("/dev/null", "w", stdout);
            freopen("/dev/null", "w", stderr);
            execl("./initksocket", "./initksocket", NULL);
            exit(1);
        }
        sleep(1); 

        pid_t user2_pid = fork();
        if (user2_pid == 0) {
            freopen("/dev/null", "w", stdout);
            freopen("/dev/null", "w", stderr);
            execl("./user2", "./user2", NULL);
            exit(1);
        }
        sleep(1); 

        system("./user1 | grep -E 'Average Transmissions|Total Unique|Total Transmissions'");

        sleep(2); 
        kill(init_pid, SIGKILL);
        kill(user2_pid, SIGKILL);

        waitpid(init_pid, NULL, 0);
        waitpid(user2_pid, NULL, 0);

        system("ipcrm -M 100 2>/dev/null");

    }

    printf("Testing Complete!\n");
    return 0;
}