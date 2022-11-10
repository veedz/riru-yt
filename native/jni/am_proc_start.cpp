// Code are copied from https://gist.github.com/vvb2060/a3d40084cd9273b65a15f8a351b4eb0e#file-am_proc_start-cpp

#include <unistd.h>
#include <string>
#include <cinttypes>
#include <android/log.h>
#include <sys/system_properties.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include "crawl_procfs.hpp"
#include "cus.hpp"
#include <libgen.h>
#include <sys/mount.h>

using namespace std;

int myself;

extern "C" {

struct logger_entry {
    uint16_t len;      /* length of the payload */
    uint16_t hdr_size; /* sizeof(struct logger_entry) */
    int32_t pid;       /* generating process's pid */
    uint32_t tid;      /* generating process's tid */
    uint32_t sec;      /* seconds since Epoch */
    uint32_t nsec;     /* nanoseconds */
    uint32_t lid;      /* log id of the payload, bottom 4 bits currently */
    uint32_t uid;      /* generating process's uid */
};

#define LOGGER_ENTRY_MAX_LEN (5 * 1024)
struct log_msg {
    union [[gnu::aligned(4)]] {
        unsigned char buf[LOGGER_ENTRY_MAX_LEN + 1];
        struct logger_entry entry;
    };
};

[[gnu::weak]] struct logger_list *android_logger_list_alloc(int mode, unsigned int tail, pid_t pid);
[[gnu::weak]] void android_logger_list_free(struct logger_list *list);
[[gnu::weak]] int android_logger_list_read(struct logger_list *list, struct log_msg *log_msg);
[[gnu::weak]] struct logger *android_logger_open(struct logger_list *list, log_id_t id);

typedef struct [[gnu::packed]] {
    int32_t tag;  // Little Endian Order
} android_event_header_t;

typedef struct [[gnu::packed]] {
    int8_t type;   // EVENT_TYPE_INT
    int32_t data;  // Little Endian Order
} android_event_int_t;

typedef struct [[gnu::packed]] {
    int8_t type;     // EVENT_TYPE_STRING;
    int32_t length;  // Little Endian Order
    char data[];
} android_event_string_t;

typedef struct [[gnu::packed]] {
    int8_t type;  // EVENT_TYPE_LIST
    int8_t element_count;
} android_event_list_t;

// 30014 am_proc_start (User|1|5),(PID|1|5),(UID|1|5),(Process Name|3),(Type|3),(Component|3)
typedef struct [[gnu::packed]] {
    android_event_header_t tag;
    android_event_list_t list;
    android_event_int_t user;
    android_event_int_t pid;
    android_event_int_t uid;
    android_event_string_t process_name;
//  android_event_string_t type;
//  android_event_string_t component;
} android_event_am_proc_start;

}

void mount_daemon(int pid, const char *path){
    if (fork_dont_care()==0){
        struct stat ppid_st, pid_st;
        int i=0;
        do {
       	    if (i>=300000) _exit(0);
            if (read_ns(pid,&pid_st) == -1 ||
                read_ns(parse_ppid(pid),&ppid_st) == -1)
                _exit(0);
            usleep(10);
            i++;
        } while (pid_st.st_ino == ppid_st.st_ino &&
                pid_st.st_dev == ppid_st.st_dev);
        
        // stop process
        kill(pid, SIGSTOP);
        if (!switch_mnt_ns(pid)){
            char src[1024];
            char dest[1024];
            snprintf(src, 1024, "%s/revanced.apk", dirname(path));
            snprintf(dest, 1024, "%s/base.apk", dirname(path));
            mount(src, dest, nullptr, MS_BIND, nullptr);
        }
        kill(pid, SIGCONT);
        _exit(0);
    }
}



void ProcessBuffer(struct logger_entry *buf, const char *path) {
    auto *eventData = reinterpret_cast<const unsigned char *>(buf) + buf->hdr_size;
    auto *event_header = reinterpret_cast<const android_event_header_t *>(eventData);
    if (event_header->tag != 30014) return;
    auto *am_proc_start = reinterpret_cast<const android_event_am_proc_start *>(eventData);
    if (!is_youtube(am_proc_start->uid.data)) return;
    mount_daemon(am_proc_start->pid.data, path);
}

[[noreturn]] void Run(const char *path) {
    while (true) {
        bool first;
        __system_property_set("persist.log.tag", "");

        unique_ptr<logger_list, decltype(&android_logger_list_free)> logger_list{
            android_logger_list_alloc(0, 1, 0), &android_logger_list_free};
        auto *logger = android_logger_open(logger_list.get(), LOG_ID_EVENTS);
        if (logger != nullptr) [[likely]] {
            first = true;
        } else {
            continue;
        }

        struct log_msg msg{};
        while (true) {
            if (android_logger_list_read(logger_list.get(), &msg) <= 0) [[unlikely]] {
                break;
            }
            if (first) [[unlikely]] {
                first = false;
                continue;
            }

            ProcessBuffer(&msg.entry, path);
        }

        sleep(1);
    }
}

void kill_other(struct stat me){
    crawl_procfs([=](int pid) -> bool {
   	    struct stat st;
        char path[128];
   	    sprintf(path, "/proc/%d/exe", pid);
        if (stat(path,&st)!=0)
            return true;
        if (pid == myself)
            return true;
        if (st.st_dev == me.st_dev && st.st_ino == me.st_ino) {
       	    fprintf(stderr, "Killed: %d\n", pid);
       	    kill(pid, SIGKILL);
        }
        return true;
    });
}

int main(int argc, char *argv[]) {
    if (getuid()!=0) return 1;
    struct stat me;
    myself = self_pid();
    
    if (stat(argv[0],&me)!=0)
        return 1;

    if (argc > 1 && argv[1] == "--stop"sv) {
        kill_other(me);
        return 0;
    }

    kill_other(me);
    if (fork_dont_care()==0){
        fprintf(stderr, "New daemon: %d\n", self_pid());
        if (switch_mnt_ns(1))
            _exit(0);
        signal(SIGTERM, SIG_IGN);
        Run(argv[0]);
        _exit(0);
    }
    return 0;
}
