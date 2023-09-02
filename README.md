# keyboard-cursor-navigator

C++ program that allows you to move the mouse cursor using your keyboard

This is a program that can be turned into a daemon service which allows you to move your mouse cursor using CapsLock + AWSD keys, with Space to accelerate the speed and X as left-click. 

The binding can be changed by changing the code, but my current setup uses CapsLock as an additional Hyper key (a modifier key, like Ctrl). You can also do that by creating your own variant of the keyboard layout, see [this link](https://forum.endeavouros.com/t/how-to-convert-caps-lock-to-hyper-key/24575) to find out how to do so. 

## Device Input Files
This program works by consuming keyboard events from OS devices input files. The devices in question are listed in the output of `cat /proc/bus/input/`. The program doesn't know which files to observe keyboard events, but it uses some heuristics, for example the one described in [this stackoverflow answer](https://stackoverflow.com/questions/29678011/determine-linux-keyboard-events-device).

If your input is not detected, it is likely that the heuristics failed. This can happen more likely if you are using a bluetooth/external keyboard. To fix it, you can try modifying the code to include specifically your keyboard device name.

## Dependencies
This program requires `xdo` and `evdev` libs as dependencies, and autokey
Also, ensure you have a hidden file in your ~/ directory called ".Xauthority"

## Installation

After cloning this repository and entering the root directory,
1. Install dependencies
    ```bash
    sudo apt install libxdo-dev libevdev-dev
    ```
2. Go to `keyboard-cursor-navigator.service` and replace "\<username\>" with your username.
3. Move `keyboard-cursor-navigator.service` to `/etc/systemd/system/` (or any other systemd path of your preference), by running: 
    
    ```bash
    sudo mv keyboard-cursor-navigator.service /etc/systemd/system/
    ```
4. Build this program by running:
    ```bash
    sudo make build
    ```
    You can edit Makefile to customize the location of the binary, but remember to update the path in keyboard-cursor-navigator.service file as well.
5. Activate your daemon by running:
    ```bash
    sudo systemctl enable keyboard-cursor-navigator.service
    ```
6. Done! Check that you can move your mouse by pressing CapsLock + AWSD

## Running locally

You can also run locally by just doing step 1 and 4, but in 4 you instead run `sudo make && ./main`


## Preventing AWSD to emit letters using Autokeys
This is a hack to prevent the keyboard navigation emitting the letters A, W, S and D, even when your cursor is focused in some editable area. To do that, I use [Autokeys](https://github.com/autokey/autokey) to create noop hotkeys for each combination CapsLock+A, CapsLock+W, CapsLock+S, CapsLock+D, CapsLock+X and finally CapsLock+Space.

## Contributing

Contributions are accepted! Just open a issue with your suggestion/request or go ahead and fork the repository and create a pull request, and I will review. Contact me on lucas.turci@gmail.com if I take too long to review.

Here is a list of improvements available to implement:
- Improve the way we search for devices to observe (maybe have a way for the user to configure the list of keyboards, maybe just open all input files)
- Have the program update the list of files when devices are connected or disconnected.
- Include commands for drag, scroll, right-click
- Add capability to customize the key bindings of commands.