#include <algorithm>
#include <cstring>
#include <unordered_map>

extern "C" {
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <unistd.h>
}

#include "common.hpp"
#include "answer_generator.hpp"

static bool not_responding;

static const std::unordered_map<std::string, std::string> mime_types = {
    {"html", "text/html"},
    {"css", "text/css"},
    {"js", "application/javascript"},
    {"json", "application/json"},
    {"ttf", "application/font-sfnt"},
    {"pdf", "application/pdf"},
    {"svg", "image/svg+xml"},
    {"png", "image/png"},
    {"jpg", "image/jpeg"},
    {"gif", "image/gif"},
    {"ogg", "audio/ogg"},
    {"mp3", "audio/mpeg"},
    {"mkv", "video/x-matroska"},
    {"txt", "text/plain"},
    {"c", "text/x-csrc"},
    {"cpp", "text/x-c++src"},
    {"md", "text/x-markdown"}
};

std::string determine_mime(const std::string &file_name) {
    size_t length = file_name.size();
    size_t last_point = length - 1;
    while (last_point != 0 && file_name[last_point] != '.') {
        --last_point;
    }
    if (file_name[last_point] == '.') {
        ++last_point;
        std::string extension = std::string(
            &file_name[last_point],
            length - last_point
        );
        if (mime_types.find(extension) != mime_types.end()) {
            return mime_types.at(extension);
        }
    }
    return "application/octet-stream";
}

STAT file_type(const std::string &path) {
    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) == -1) {
        return STAT::NOT_EXIST;
    }
    if (S_ISREG(path_stat.st_mode)) {
        return STAT::REGULAR;
    }
    if (S_ISDIR(path_stat.st_mode)) {
        return STAT::DIRECTORY;
    }
    return STAT::UNKNOWN;
}

static void timer_handler(int) {
    not_responding = true;
}

std::string get_output(
    const std::string &program,
    const char *envp[],
    const std::string &chroot
) {
    std::string output;
    int fd[2];
    pipe(fd);
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);
        if (chroot.size()) {
            execl("/bin/cp", "/bin/cp", program.c_str(),
            (chroot + "/" + basename(program)).c_str(), nullptr);
            execle("/usr/bin/helper", "/usr/bin/helper", PROJECT_NAME,
            chroot.c_str(), basename(program).c_str(), nullptr, envp);
        } else {
            execle(program.c_str(), program.c_str(), nullptr, envp);
        }
        exit(0);
    } else {
        close(fd[1]);
        struct itimerval itv;
        itv.it_interval.tv_sec = 0;
        itv.it_interval.tv_usec = 0;
        itv.it_value.tv_sec = 1;
        itv.it_value.tv_usec = 0;
        struct sigaction act;
        memset(&act, 0, sizeof(act));
        act.sa_handler = timer_handler;
        act.sa_flags = !SA_RESTART;
        sigaction(SIGALRM, &act, nullptr);
        setitimer(ITIMER_REAL, &itv, nullptr);
        char buffer[BUFFER_SIZE];
        for (;;) {
            ssize_t count = read(fd[0], buffer, sizeof(buffer));
            if (not_responding) {
                kill(pid, SIGKILL);
                return generate_error(500, "");
            }
            if (count == 0) {
                break;
            } else {
                output += std::string(buffer, count);
            }
        }
        close(fd[0]);
        wait(0);
    }
    return output;
}

std::string hostname() {
    struct utsname s;
    uname(&s);
    return s.nodename;
}

std::string pwd() {
    char buffer[PATH_MAX];
    getcwd(buffer, PATH_MAX);
    return buffer;
}

bool from_string(const std::string &s, uint16_t *n) {
    if (s.size() == 0) {
        return false;
    }
    errno = 0;
    char *endptr;
    long res = strtol(s.c_str(), &endptr, 10);
    *n = res;
    if (errno || *endptr || res < 0 || res != *n) {
        return false;
    }
    return true;
}

std::string basename(const std::string &path) {
    return std::string(
        std::find(path.rbegin(), path.rend(), '/').base(),
        path.end()
    );
}
