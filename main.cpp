#include <libevdev-1.0/libevdev/libevdev.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <unistd.h>
#include <ctime>
#include <thread>
#include <string>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <csignal>
#include <sys/resource.h>
#include <errno.h>
#include <string.h>

extern "C" { // see github issue response: https://github.com/jordansissel/xdotool/issues/63#issuecomment-136887557
    #include <xdo.h>
}

#define EV_PRESSED 1
#define EV_RELEASED 0
#define BOOST 800
#define SPACE_INTERVAL 10
#define BOOST_MAX_SPEED 1500
#define MAX_SPEED 1000

#define ACC 8

using std::map;
using std::pair;

// devices
std::vector<pair<struct libevdev*, std::string>> devs;
xdo_t * xdo;

std::vector<int> cleanup_signals = {SIGINT, SIGTERM, SIGQUIT, SIGABRT, SIGHUP, SIGSEGV, SIGPIPE};

void close_evdev(struct libevdev* dev) {
    int fd = libevdev_get_fd(dev);

    if(fd >= 0) {
        libevdev_free(dev);

        // Set the file descriptor to non-blocking mode before closing
        int flags = fcntl(fd, F_GETFL);
        flags |= O_NONBLOCK;
        fcntl(fd, F_SETFL, flags);
        
        close(fd);
    }
}

void cleanup(int signum) {
    if(std::find(cleanup_signals.begin(), cleanup_signals.end(), signum) != cleanup_signals.end()) {
        for(auto [dev, _] : devs) {
            close_evdev(dev);
        }
        devs.clear();
    }
    xdo_free(xdo);
    exit(signum);
}

// Initialize core dump file limits for post-mortem debugging
void init() {
    struct rlimit limit;
    limit.rlim_cur = RLIM_INFINITY;
    limit.rlim_max = RLIM_INFINITY;
    int rv = setrlimit(RLIMIT_CORE, &limit);
    if (rv) {
        fprintf(stderr, "Unable to set limit for core dump file size: %s\n", strerror(errno));
    }
}

std::string exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    FILE* pipe = popen(cmd, "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof buffer, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;
}

void go(int device_count) {
    xdo = xdo_new(NULL);
    xdo->close_display_when_freed = 1;
    char event_file[30];
    
    for(int i = 0; i < device_count; ++i) {
        sprintf(event_file, "/dev/input/event%d", i);
        int fd = open(event_file, O_RDONLY);
        if(fd < 0) {
            fprintf(stderr, "Could not open file descriptor for %s\n", event_file);
            continue;
        }
        struct libevdev * dev = NULL;
        int rc = libevdev_new_from_fd(fd, &dev);
        if (rc < 0) {
            fprintf(stderr, "Could not create evdev for file %s\n", event_file);
            close(fd);
            continue;
        }
        if(libevdev_has_event_type(dev, EV_KEY) && libevdev_has_event_code(dev, EV_KEY, KEY_CAPSLOCK)) {
            fprintf(stderr, "Registering dev from event%d\n", i);
            devs.emplace_back(dev, event_file);
        } else {
            close_evdev(dev);
        }
    }

    map<int, bool> keymap = {
        {KEY_CAPSLOCK, false},
        {KEY_A, false},
        {KEY_S, false},
        {KEY_W, false},
        {KEY_D, false},
        {KEY_SPACE, false},
    };

    std::signal(SIGINT, cleanup);

    int cur_speed = 400;
    int last_speed;
    int space_interval_tick = -1;
    bool boost_enabled = false;

    while(1) {
        for (int i = 0; i < devs.size(); ++i) {
            auto [dev, name] = devs[i];
            while (libevdev_has_event_pending(dev)) {
                struct input_event ev;
                int rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
                if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
                    if (ev.type == EV_KEY) {
                        if (ev.value == EV_PRESSED) {
                            if (keymap.find(ev.code) != keymap.end()) {
                                keymap[ev.code] = true;
                            }
                            if(keymap[KEY_CAPSLOCK]) {
                                if(ev.code == KEY_X) {
                                    int ret = xdo_click_window(xdo, CURRENTWINDOW, 1);
                                    if (ret) {
                                        fprintf(stderr, "Error clicking mouse: %d\n", ret);
                                    }
                                }
                            }
                            if(ev.code == KEY_SPACE) {
                                if(space_interval_tick < SPACE_INTERVAL) {
                                    cur_speed = last_speed;
                                    cur_speed += BOOST;
                                    boost_enabled = true;
                                }
                                space_interval_tick = 0;
                            }
                        } else if(ev.value == EV_RELEASED) {
                            if (keymap.find(ev.code) != keymap.end()) {
                                keymap[ev.code] = false;
                                // Key pressed, perform action
                            }
                        }
                    }
                }
            }
        }

        // different actions
        if(keymap[KEY_CAPSLOCK]) {
            
            int delta_x = 0;
            int delta_y = 0;
            bool moving = false;
            if (keymap[KEY_A]) {
                // travel left
                delta_x += -cur_speed;
                moving = true;
            }
            if (keymap[KEY_D]) {
                // travel right
                delta_x += cur_speed;
                moving = true;
            }
            if (keymap[KEY_W]) {
                // travel up
                delta_y += -cur_speed;
                moving = true;
            }
            if (keymap[KEY_S]) {
                // travel down
                delta_y += cur_speed;
                moving = true;
            }
            if(!moving) {
                cur_speed = 400;
            }
            if (keymap[KEY_SPACE]) {
                cur_speed += ACC;
                if(cur_speed > (boost_enabled? BOOST_MAX_SPEED : MAX_SPEED)) {
                    cur_speed = boost_enabled? BOOST_MAX_SPEED : MAX_SPEED;
                }
                delta_x *= 5;
                delta_y *= 5;
                
                last_speed = cur_speed;
            } else {
                space_interval_tick++;
                if(space_interval_tick > SPACE_INTERVAL) {
                    space_interval_tick = SPACE_INTERVAL;
                }
                boost_enabled = false;
                cur_speed = 400;
            }
            if(delta_x || delta_y) {
                int ret = xdo_move_mouse_relative(xdo, delta_x/100, delta_y/100);
                if (ret) {
                    fprintf(stderr, "Error moving mouse: %d\n", ret);
                }
            }
        }
        usleep(1000000/30); // 30 fps
    }
}

int main(int argc, char * argv[]) {
    init();
    std::string device_count_str = exec("cat /proc/bus/input/devices | grep '^$' | wc -l");
    int device_count = stoi(device_count_str);
    go(device_count);
}
