# HiDPI Fixer for GNOME-based desktops

This application allows you to create a script that acomplishes the following tasks:
- Allow fractional scaling of your display and its components in X11
- The end result looks nicer and is way less buggy than using Wayland
- The generated script is configured to run everytime you log in
- To avoid "race" conditions with GNOME-Settings, you can delay the execution of the script
- You can also instruct the application to modify the `~/.profile` file to correctly scale Qt-based apps (as KDE does)

# Installing/running

You can [download](https://github.com/alex-spataru/HiDPI-Fixer/releases/latest) the latest release as an AppImage, no installation required!
After your download finishes, open a terminal and type the following commands:

    cd Downloads
    chmod +x HiDPI-Fixer.AppImage
    ./HiDPI-Fixer.AppImage

## How does it work?

This application uses a combination of GNOME's `scaling-factor` setting and `xrandr` hacks. I was inspired to write this application after reading some documentation on the [ArchWiki](https://wiki.archlinux.org/index.php/HiDPI#Fractional_Scaling). After some trial and error, I managed to create a script that would adjust the UI scaling to my laptop's display.

Finally, I decided to make this application to automate the process of fixing fractional HiDPI displays. 

## License

This project is released under the MIT license, for more info, click [here](LICENSE.md).

