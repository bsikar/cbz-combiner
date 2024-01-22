```
sudo apt update
sudo apt install libzip-dev bear libpng-dev libjpeg-dev
bear -- make

# good command to find all leaks
make valgrind VAL_ARGS="--leak-check=full --show-leak-kinds=all -s" ARGS="../../../Attack_on_Titan/* -v" > valgrind_output.txt 2>&1


# how to actually find leaks, valgrind + gdb

tmux
-> then split screen

-> on one screen
make valgrind VAL_ARGS="--vgdb=yes --vgdb-error=0" ARGS="../../../Attack_on_Titan/*"

-> on another screen
make debug

=> in gdb
=> set non-stop off
=> target remote | vgdb

(you cannot r only c, it has to restart since its running in valgrind)

=> monitor leak_check
=> monitor who_points_at

Hook up your program under GDB and Valgrind
Put a break at where you think the memory is lost
Continue there and run a leak check
If there is no leak yet, slowly proceed forward, doing a leak check after each step or at every new breakpoint
Once you see a leak, that leak occurred between the last stop and this
Restart and take even smaller steps, with leak checks in between every one of them, an pinpoint the exact statement that creates the leak.
```

| Command   | Short Form | Description |
|-----------|------------|-------------|
| next      | n          | Executes the current line of code and stops at the next line. Steps over function calls. |
| nexti     | ni         | Similar to `next`, but operates at the assembly instruction level. Steps over assembly instructions. |
| step      | s          | Executes the current line of code and stops at the first instruction of the next line. Steps into function calls if present. |
| stepi     | si         | Similar to `step`, but operates at the assembly instruction level. Steps into the next assembly instruction. |

press <C-x> o to switch to other screen in gdb after running `tui enable` 

in the tui:

layout asm
and you can do <C-x> NUMBER so <C-x> 2



also to kill the valgrind instance you could do ^Z see what [X] it is like [2] then do kill %2
