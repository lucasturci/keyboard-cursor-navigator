# keyboard-cursor-navigator

C++ program that allows you to move the mouse cursor using your keyboard

This is a program that can be turned into a daemon service which allows you to move your mouse cursor using CapsLock + AWSD keys, with Space to accelerate the speed and X as left-click. 

The binding can be changed by changing the code, but my current setup uses CapsLock as an additional Hyper key (a modifier key, like Ctrl). You can also do that by creating your own variant of the keyboard layout, see [this link](https://forum.endeavouros.com/t/how-to-convert-caps-lock-to-hyper-key/24575) to find out how to do so. 

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