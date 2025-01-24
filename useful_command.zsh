#! /usr/bin/zsh

alias pico_build="cmake -B build -S . && cmake --build build -j8"
alias pico_clean="rm -rf build"
alias pico_upload="sudo picotool load -f build/hmi_pico_stp.elf"
alias pico_monitor="tio -b 115200 /dev/ttyACM0"
alias pico_reboot="sudo picotool reboot -f"
alias pico_build_and_upload="pico_build && pico_upload"