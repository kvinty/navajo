#include <climits>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <linux/limits.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

static bool find_user_uid(const char *name, uid_t *result) {
    struct passwd *pwd = getpwnam(name);
    if (pwd) {
        *result = pwd->pw_uid;
        return true;
    } else {
        return false;
    }
}

static bool is_allowed() {
    static const char allowed_parent[] = "/usr/bin/" PROJECT_NAME;
    pid_t parent_pid = getppid();
    char proc_file_name[80];
    sprintf(proc_file_name, "/proc/%lld/exe", (long long) parent_pid);
    char parent_path[PATH_MAX];
    memset(parent_path, 0, PATH_MAX);
    readlink(proc_file_name, parent_path, PATH_MAX);
    if (parent_path[PATH_MAX - 1]) {
        return false;
    }
    return !strcmp(allowed_parent, parent_path);
}

int main(int argc, char *argv[]) {
    uid_t run_uid = 0;
    setuid(0);
    if (argc != 4) {
        fprintf(stderr, "Error: usage: helper user chroot program\n");
        return 1;
    }
    if (!is_allowed()) {
        fprintf(stderr, "Error: not allowed to run by security reasons\n");
        return 1;
    }
    if (!find_user_uid(argv[1], &run_uid)) {
        fprintf(stderr, "Error: can't find specified user\n");
        return 1;
    }
    if (chdir(argv[2])) {
        fprintf(stderr, "Error: can't change directory\n");
        return 1;
    }
    if (chroot(".")) {
        fprintf(stderr, "Error: can't chroot\n");
        return 1;
    }
    setuid(run_uid);
    if (execl(argv[3], argv[3], nullptr)) {
        fprintf(stderr, "Can't start program\n");
        return 1;
    }
}
