#include <pwd.h>
#include <signal.h>
#include <sys/types.h>

void sync(void) {}
uid_t getuid(void) { return 0; }

struct passwd *getpwuid(uid_t uid) {
    (void)uid;
    static struct passwd result = { .pw_dir = "/sd/MiaOS/Library" };
    return &result;
}

int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
    (void)signum;
    (void)act;
    (void)oldact;
    return 0;
}
