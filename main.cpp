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

extern "C" { // see github issue response: https://github.com/jordansissel/xdotool/issues/63#issuecomment-136887557
    #include <xdo.h>
}

#define EV_PRESSED 1
#define EV_RELEASED 0

using std::map;

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

void go(std::vector<std::string> input_files) {
    xdo_t * xdo = xdo_new(NULL);
    char event_file[30];
    
    std::vector<struct libevdev*> devs;
    for(auto input_file : input_files) {
        sprintf(event_file, "/dev/input/event%s", input_file.substr(5).c_str());
        int fd = open(event_file, O_RDONLY);
        struct libevdev * dev = NULL;
        int rc = libevdev_new_from_fd(fd, &dev);
        if (rc < 0) {
            fprintf(stderr, "Could not create evdev for file %s\n", event_file);
            continue;
        }
        devs.push_back(dev);
    }

    map<int, bool> keymap = {
        {KEY_CAPSLOCK, false},
        {KEY_A, false},
        {KEY_S, false},
        {KEY_W, false},
        {KEY_D, false},
        {KEY_SPACE, false},
    };

    while(1) {
        for (auto dev : devs) {
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
            int speed = 4;
            if (keymap[KEY_SPACE]) {
                speed *= 5;
            }
            int delta_x = 0;
            int delta_y = 0;
            if (keymap[KEY_A]) {
                // travel left
                delta_x += -speed;
            }
            if (keymap[KEY_D]) {
                // travel right
                delta_x += speed;
            }
            if (keymap[KEY_W]) {
                // travel up
                delta_y += -speed;
            }
            if (keymap[KEY_S]) {
                // travel down
                delta_y += speed;
            }
            if(delta_x || delta_y) {
                int ret = xdo_move_mouse_relative(xdo, delta_x, delta_y);
                if (ret) {
                    fprintf(stderr, "Error moving mouse: %d\n", ret);
                }
            }
        }
        usleep(1000000/30); // 30 fps
    }
}

std::vector<std::string> splitStringByDelimiter(std::string str, char delimiter) {
    std::vector<std::string> result;
    std::istringstream ss(str);
    std::string token;
    while(getline(ss, token, delimiter)) {
        result.push_back(token);
    }
    return result;
}

std::string toLowercase(std::string str) {
    std::string result = "";
    for(int i = 0; i < str.size(); i++) {
        result += tolower(str[i]);
    }
    return result;
}

int main(int argc, char * argv[]) {
    std::string devices_output = exec("cat /proc/bus/input/devices");

    std::istringstream ss(devices_output);
    std::string line;

    std::string input_file;
    std::string phys_input_file;
    std::vector<std::string> input_files;

    bool put = false;
    bool dontput = false;
    while(getline(ss, line, '\n')) {
        
        if(line.size() == 0) {
            // decide to push back or not and reset variables
            if(put && !dontput) {
                input_files.push_back(input_file);
            }
            put = dontput = false;
            input_file = "";
            phys_input_file = "";
        }

        if(line[0] == 'S') {
            input_file = splitStringByDelimiter(line, '/').back();
        } else if(line[0] == 'B') {
            if(line == "B: EV=120013") {
                put = true;
            }
        } else if(line[0] == 'N') {
            line = toLowercase(line);
            if(line.find("keyboard") != std::string::npos) {
                put = true;
            } else if(line.find("mouse") != std::string::npos) {
                dontput = true;
            } else if(line.find("transceiver") != std::string::npos) {
                put = true;
            }
        } else if(line[0] == 'P') {
            phys_input_file = splitStringByDelimiter(line, '/').back();
            if(phys_input_file != "input0") {
                dontput = true;
            }
        }
    }

    go(input_files);
}
