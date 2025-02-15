#include <iostream>
#include <vector>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <csignal>
#include <thread>
#include <atomic>
#include <dlfcn.h>
#include <cstdlib>
#include <linux/input.h>
#include <linux/joystick.h>
#include <fcntl.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <list>
#include <thread>
#include "structs.h"

const uint16_t VENDOR  = 0x0547;
const uint16_t PRODUCT = 0x1002;

std::atomic<bool> running(true);

const uint8_t STATE_CAB_PLAYER_1 = 1;
const uint8_t STATE_CAB_PLAYER_2 = 3;
const uint8_t STATE_PLAYER_1 = 0;
const uint8_t STATE_PLAYER_2 = 2;

const uint8_t PAD_LU = 0x01;
const uint8_t PAD_RU = 0x02;
const uint8_t PAD_CN = 0x04;
const uint8_t PAD_LD = 0x08;
const uint8_t PAD_RD = 0x10;

const uint8_t CAB_SERVICE = 0x40;
const uint8_t CAB_TEST = 0x02;
const uint8_t CAB_COIN = 0x04;
const uint8_t CAB_CLEAR = 0x80;

uint8_t IOSTATE[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

struct piu_bind {
    uint16_t keycode;
    uint8_t state;
    uint8_t bit;
};

struct ltekpad_mapping {
    int button;
    const char* key;
    uint8_t state;
    uint8_t bit;
};

const std::list<piu_bind> binds = {
    { KEY_Q,  STATE_PLAYER_1, PAD_LU },
    { KEY_E,  STATE_PLAYER_1, PAD_RU },
    { KEY_S,  STATE_PLAYER_1, PAD_CN },
    { KEY_Z,  STATE_PLAYER_1, PAD_LD },
    { KEY_C,  STATE_PLAYER_1, PAD_RD },
    { KEY_F5, STATE_CAB_PLAYER_1, CAB_COIN },

    { KEY_KP7,  STATE_PLAYER_2, PAD_LU },
    { KEY_KP9,  STATE_PLAYER_2, PAD_RU },
    { KEY_KP5,  STATE_PLAYER_2, PAD_CN },
    { KEY_KP1,  STATE_PLAYER_2, PAD_LD },
    { KEY_KP3,  STATE_PLAYER_2, PAD_RD },
    { KEY_F6,    STATE_CAB_PLAYER_2, CAB_COIN },

    { KEY_F1, STATE_CAB_PLAYER_1, CAB_TEST },
    { KEY_F2, STATE_CAB_PLAYER_1, CAB_SERVICE },
    { KEY_F3, STATE_CAB_PLAYER_1, CAB_CLEAR },
};

const std::list<ltekpad_mapping> ltek_binds = {
    {0, "q", STATE_PLAYER_1, PAD_LU},  // Ltek button LEFT UP
    {1, "e", STATE_PLAYER_1, PAD_RU},  // Ltek button RIGHT UP
    {4, "s", STATE_PLAYER_1, PAD_CN},  // Ltek button CENTER
    {2, "z", STATE_PLAYER_1, PAD_LD},  // Ltek button LEFT DOWN
    {3, "c", STATE_PLAYER_1, PAD_RD}   // Ltek button RIGHT DOWN
};

bool is_real_pad_connected = false;
bool was_emulated_device_added = false;


void handle_ltekpad() {
    int ltek_fd = open("/dev/input/js0", O_RDONLY);
    if (ltek_fd == -1) {
        std::cerr << "Can't open ltekPad" << std::endl;
        return;
    }

    struct js_event event;
    while (true) {
        if (read(ltek_fd, &event, sizeof(event)) == sizeof(event)) {
            if (event.type == JS_EVENT_BUTTON) {
                for (const auto& mapping : ltek_binds) {
                    if (event.number == mapping.button) {
                        if (event.value) {
                            // button pressed
                            IOSTATE[mapping.state] &= ~mapping.bit;
                        } else {
                            // button not pressed
                            IOSTATE[mapping.state] |= mapping.bit;
                        }
                    }
                }
            }
        }
    }
    close(ltek_fd);
}

void __attribute__((constructor)) init_ltekpad() {
    std::thread ltek_thread(handle_ltekpad);
    ltek_thread.detach();
}

std::vector<std::string> open_all_keyboards() {
    const char* input_dir = "/dev/input/";
    DIR* dir = opendir(input_dir);
    if (!dir) {
        perror("Failed to open /dev/input/");
        return {};
    }

    std::vector<std::string> keyboard_paths;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            std::string device_path = std::string(input_dir) + "/" + entry->d_name;

            int fd = open(device_path.c_str(), O_RDONLY);
            if (fd < 0) {
                std::cerr << "Failed to open " << device_path << std::endl;
                continue;
            }

            unsigned long ev_bits[(EV_MAX + 7) / 8] = {0};
            if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
                close(fd);
                continue;
            }

            if (ev_bits[EV_KEY / 8] & (1 << (EV_KEY % 8))) {
                unsigned long key_bits[(KEY_MAX + 7) / 8] = {0};
                if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(key_bits)), key_bits) >= 0) {
                    if (key_bits[KEY_A / 8] & (1 << (KEY_A % 8)) ||
                        key_bits[KEY_ENTER / 8] & (1 << (KEY_ENTER % 8))) {
                        std::cout << "Keyboard detected: " << device_path << std::endl;
                        keyboard_paths.push_back(device_path);
                    }
                }
            }

            close(fd);
        }
    }

    closedir(dir);
    return keyboard_paths;
}

void evdev_thread() {
    std::vector<std::string> devices = open_all_keyboards();

    if (devices.empty()) {
        std::cerr << "No keyboards found!" << std::endl;
        return;
    }

    std::vector<int> fds;
    for (const auto& device : devices) {
        int fd = open(device.c_str(), O_RDONLY);
        if (fd < 0) {
            std::cerr << "Failed to open keyboard device: " << device << std::endl;
            continue;
        }
        fds.push_back(fd);
    }

    struct input_event ev;
    while (running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        int max_fd = -1;

        for (int fd : fds) {
            FD_SET(fd, &read_fds);
            if (fd > max_fd) {
                max_fd = fd;
            }
        }

        int ready = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);
        if (ready < 0) {
            std::cerr << "Select error!" << std::endl;
            break;
        }

        for (size_t i = 0; i < fds.size(); ++i) {
            int fd = fds[i];
            if (FD_ISSET(fd, &read_fds)) {
                ssize_t n = read(fd, &ev, sizeof(ev));
                if (n == (ssize_t)sizeof(ev)) {
                    if (ev.type == EV_KEY && ev.value <= 3) {
                        //std::cout << "Key Pressed: " << ev.code << std::endl; //debug print

                        if (ev.code == KEY_ESC && ev.value != 0) { 
                           std::cout << "ESC pressed. Terminating Game..." << std::endl;
                           std::raise(SIGTERM); 
                           return; 
                        }

                        for (const auto& bind : binds) {
                            if (ev.code == bind.keycode && ev.value != 2) {
                                IOSTATE[bind.state] ^= bind.bit;
                                //std::cout << "Updated IOSTATE for keycode: " << ev.code << std::endl; //debug print
                            }
                        }
                    }
                }
            }
        }
    }

    for (int fd : fds) {
        close(fd);
    }
}

void add_emulated_device() {
    if (!usb_busses) {
        std::cerr << "usb_busses was NULL" << std::endl;
        return;
    }
    std::cout << "======= KRT - Konmairo Rhythm Team ======" << std::endl;
    for (usb_bus* bus = usb_busses; bus; bus = bus->next) {
        for (usb_device* device = bus->devices; device; device = device->next) {
            if (device->descriptor.idVendor == VENDOR && device->descriptor.idProduct == PRODUCT) {
                is_real_pad_connected = true;
                std::cout << "Real dance pad detected. Not creating emulated device." << std::endl;
                return;
            }
        }
    }

    usb_device* emu_device = new usb_device;
    emu_device->config = new usb_config_descriptor;
    emu_device->config->interface = new usb_interface;
    emu_device->descriptor.idVendor = VENDOR;
    emu_device->descriptor.idProduct = PRODUCT;

    LIST_ADD(usb_busses->devices, emu_device);

    was_emulated_device_added = true;
    std::cout << "Added emulated device." << std::endl;
    std::thread(evdev_thread).detach();
}

extern "C" usb_dev_handle* usb_open(usb_device* dev) {
    static auto o_usb_open = reinterpret_cast<decltype(&usb_open)>(dlsym(RTLD_NEXT, "usb_open"));

    usb_dev_handle* ret = nullptr;
    if (is_real_pad_connected) {
        ret = o_usb_open(dev);
    } else {
        ret = new usb_dev_handle;
    }

    return ret;
}


extern "C" int usb_claim_interface(usb_dev_handle *dev, int interface) {
    static auto o_usb_claim_interface = reinterpret_cast<decltype(&usb_claim_interface)>(dlsym(RTLD_NEXT, "usb_claim_interface"));

    if (!o_usb_claim_interface) {
        std::cerr << "o_usb_claim_interface was NULL" << std::endl;
        std::exit(1);
    }

    int ret = 0;
    if (is_real_pad_connected) {
        ret = o_usb_claim_interface(dev, interface);
    }

    return ret;
}

extern "C" int usb_find_busses() {
    static auto o_usb_find_busses = reinterpret_cast<decltype(&usb_find_busses)>(dlsym(RTLD_NEXT, "usb_find_busses"));

    if (!o_usb_find_busses) {
        std::cerr << "o_usb_find_busses was NULL" << std::endl;
        std::exit(1);
    }

    int ret = 0;
    if (!was_emulated_device_added || is_real_pad_connected) {
        ret = o_usb_find_busses();
    }

    return ret;
}

extern "C" int usb_find_devices() {
    static auto o_usb_find_devices = reinterpret_cast<decltype(&usb_find_devices)>(dlsym(RTLD_NEXT, "usb_find_devices"));

    if (!o_usb_find_devices) {
        std::cerr << "o_usb_find_busses was NULL" << std::endl;
        std::exit(1);
    }

    int ret = 0;
    if (!was_emulated_device_added) {
        ret = o_usb_find_devices();
        add_emulated_device();
        if (!is_real_pad_connected) {
            ret = 1;
        }
    }

    return ret;
}

extern "C" int usb_set_configuration(usb_dev_handle* dev, int configuration) {
    static auto o_usb_set_configuration = reinterpret_cast<decltype(&usb_set_configuration)>(dlsym(RTLD_NEXT, "usb_set_configuration"));

    if (!o_usb_set_configuration) {
        std::cerr << "o_usb_set_configuration was NULL" << std::endl;
        std::exit(1);
    }

    int ret = 0;
    if (is_real_pad_connected) {
        ret = o_usb_set_configuration(dev, configuration);
    }

    return ret;
}

extern "C" int usb_control_msg(usb_dev_handle *dev, int requesttype, int request, int value, int index, char *bytes, int size, int timeout) {
    static auto o_usb_control_msg = reinterpret_cast<decltype(&usb_control_msg)>(dlsym(RTLD_NEXT, "usb_control_msg"));

    if (!o_usb_control_msg) {
        std::cerr << "o_usb_control_msg was NULL" << std::endl;
        std::exit(1);
    }

    int ret = 8;
    if (is_real_pad_connected) {
        ret = o_usb_control_msg(dev, requesttype, request, value, index, bytes, size, timeout);
    }

    if (requesttype == 0xC0) {
        for (int i = 0; i < sizeof(IOSTATE); i++) {
            is_real_pad_connected ? bytes[i] &= IOSTATE[i] : bytes[i] = IOSTATE[i];
        }
    }

    return ret;
}
