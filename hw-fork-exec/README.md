# `fork` and `exec`

The purpose of this assignment is to give you hands-on experience with
`fork()`, `exec()`, and `pipe()` system calls, by walking you through various
iterative exercises and examining the resulting output.  Read the entire
assignment before beginning!


# Preparation

 1. Read the following in preparation for this assignment:

    - Sections 8.2 - 8.4 and 10.8 - 10.10 in the book

    Additionally, man pages for the following are also referenced throughout
    the assignment:

    - `fork()`
    - `execve()`
    - `pipe()`
    - `dup2()`

 2. Run `make` to build two executables: `fork` and `exec`.  These are programs
    that illustrate the system calls `fork()` and `exec()`.

 3. Start a tmux session.  Create two panes, such that the window looks like
    this:

    ```
    ----------------------------
    |  command   |   system    |
    | execution  |  analysis   |
    |            |             |
    ----------------------------
    ```


# Part 1: `exec()` Overview

Open `exec.c`, and look at what it does.  Then answer the following questions.
Note that you will be _testing_ the behavior of `exec.c` in Part 2, so you
might want to revisit these questions after you go through that part.

 1. *Briefly describe the behavior of `exec.c`.*

 2. *At what point will the final `printf()` statement get executed?*


# Part 2: `exec()` Experimentation

In the next steps, you will be using the `ps` command to examine how a process
associated with the `exec` program changes over time. Because of this, you will
want to read all of problems 3 through 6 before you start.

 3. In the left ("command execution") pane of your tmux window, run the `exec`
    program, passing `/bin/cat` as the first command-line argument. *Show your
    terminal commands and the output.*

 4. In the right ("system analysis") pane of your tmux window, run the `ps`
    command, first during the initial 30-second `sleep()` call, then again after
    the first 30 seconds is over, but before the end of the program.

    Use the `-p` and `-o` options when you run `ps` so that, respectively:

    - only the process with the PID output by `exec` is shown
    - the `ps` output is formatted to have the following fields:
      - `user`: the username of the user running the process
      - `pid`: the process ID of the process
      - `ppid`: the process ID of the parent process
      - `state`: the state of the process, e.g., "Running", "Sleep", "Zombie"
      - `ucmd`: the command executed

    Use the man page for `ps` for more on how to use these options.

    *Show your terminal commands and the output.*

 5. *Which fields (if any) have changed in the output of the two ps commands?
    Briefly explain.*
    
    (You can use `ctrl`+`d` to signal end of file (EOF), so the program will
    run to completion)

 6. Run the `exec` program again, but this time using a non-existent program
    (e.g., `/does-not-exist`) as an argument.  *Show the output, and briefly
    explain what happened.*

Now would be a good time to review questions 1 and 2, both to confirm or update
your answers and to check your understanding.


# Part 3: `fork()` Overview

Open `fork.c`, and look at what it does.  Then answer the following questions.
Note that you will be _testing_ the behavior of `fork.c` in Part 4, so you will
want to consider these questions as you go through that part.

 7. *Briefly describe its behavior.*

 8. *Which sections (i.e., of "A", "B", "C", and "D") are run by the parent
    process?*

 9. *Which sections (i.e., of "A", "B", "C", and "D") are run by the child
    process?*


# Part 4: `fork()` Experimentation

In the next few steps, you will be using the `ps` command to examine how the
process(es) associated with the `fork` program change over time. Because of
this, you will want to read all of problems 10 through 17 before you start.

 10. In the left pane of your tmux window, run the `fork` program.  In the
     right pane, run the `ps` command _twice_, according to the following
     timing:
     - during the 30 seconds of sleep immediately following the printing of
       "Section B" and "Section C".
     - after "Section B done sleeping" is printed.

     Use the `-p`, `-o`, and `--forest` options when you run `ps` so that,
     respectively:

     - only the processes with the PIDs output by `fork` are shown
     - the `ps` output is formatted to have the following fields:
       - `user`: the username of the user running the process
       - `pid`: the process ID of the process
       - `ppid`: the process ID of the parent process
       - `state`: the state of the process, e.g., "Running", "Sleep", "Zombie"
       - `ucmd`: the command executed
     - the process ancestry is illustrated

     Use the man page for `ps` for more on how to use these options.

     *Show the two `ps` commands you used, each followed by its respective
     output.*

 11. *What is different between the output of the two `ps` commands?  Briefly
     explain.*
    
 12. If you were to run the `fork` and `ps` commands from question 10 again at
     the same times as you did before, *what special line of code could you add
     to `fork.c` to eliminate the process with state "Z" from the output of the
     second `ps` command?*

 13. Referring to the previous question, *where would this line most
     appropriately go?*  Remember, you want the output of the _first_ `ps`
     command to look the same as it did before; only the output of the second
     `ps` command should be different.

 14. Add the line of code referenced in question 12 to the location referenced
     in question 13.  Then call `make` to re-compiled `fork.c`. (Note that you
     may also need to add a few `#include` statements at the top of the file
     for it to compile and run properly.  See the man page for the function to
     learn which to include.)

     Follow the instructions from question 10 again to verify your answers to
     questions 12 and 13.

     *Show the two `ps` commands you used, each followed by its respective
     output.*

 15. *What is different between the output of the two `ps` commands?  Briefly
     explain.*
    
 16. Modify `fork.c` according to the following:

     - Comment out the line of code you added in question 14.
     - Replace the single call to `sleep()` in Section B with two 30-second
       `sleep()` calls, back-to-back.
     - Replace the two back-to-back calls to `sleep()` in Section C with a
       single 30-second `sleep()` call.

     Re-`make`, then run `fork` in the left pane of your tmux window.  In the
     right pane, run the `ps` command _twice_, with the same options as in
     question 10, according to the following timing:
     - during the 30 seconds of sleep immediately following the printing of
       "Section B" and "Section C".
     - after "Section C done sleeping" is printed.

     *Show the two `ps` commands, each followed by its respective output.*

 17. *What is different between the output of the two `ps` commands?  Briefly
     explain.*
    
You can now close the right pane in tmux.  Further commands will only use a
single pane.

Now would be a good time to review questions 7 through 9, both to confirm or
update your answers and to check your understanding.


# Part 5: File Descriptor Inheritance and File Description Sharing

In this section, you will learn hands-on how file descriptors are inherited by
child processes, and how two different processes with descriptors referencing
the same system-wide file description can write to the same open file.

 18. Modify `fork.c` according to the following:

     - Comment out _all_ calls to `sleep()`.
     - Comment out _all_ `printf()` calls that print "...done sleeping".
     - Before the call to `fork()`, open the file `fork-output.txt` for writing
       (see the man page for `fopen`).
     - Write "BEFORE FORK\n" to the file before the call to `fork()`.  Call
       `fflush()` on the file stream immediately after writing.

       Note that any data buffered in a file stream will be copied to the
       newly-created process created by `fork()` and that any such data would
       be printed twice--once each process flushes the buffer associated with
       its own copy of that file stream.  Calling `fflush()` makes sure the
       buffer is completely flushed before `fork()` to avoid this potentially
       confusing scenario.
     - In section B, the following, in order:
       - Sleep for 5 seconds
       - write "SECTION B (%d)\n" to the file, replacing "%d" with the
         file descriptor of the newly opened file (see the man page for
         `fileno()`).
     - In section C, do the following, in order:
       - write "SECTION C (%d)\n" to the file you opened, replacing "%d" with
         the file descriptor of the newly opened file.
       - Immediately after writing to the file, call `fclose()` on the file.

         Note that `fclose()` calls `close()` on the file descriptor, after
         flushing the buffer of the file stream (see the man page for
         `fclose()`).
       - sleep for 5 seconds.

     Re-`make` and run the newly recompiled `fork`. *Using `cat`, show the
     contents of the `fork-output.txt` file you created.*

 19. *Based on the contents of `fork-output.txt`, Which process(es) wrote to
     the file?*

 20. *Based on both the contents of `fork-output.txt` and what was written to
     the terminal, which file descriptor(s) were inherited by the child
     process?  (Hint: See also the "Note the following further points" in the
     man page for `fork()`.)*

 21. Consider the timing of the two `fprintf()` calls made in section B and C,
     which caused processes to write to `fork-output.txt`.  The execution of
     one call clearly occurred before the other.

     *Based on the content of `fork-output.txt`, did the later process start
     writing to the file at the beginning, or continue where it left off after
     the previous call?  Why?  (Hint: See the section titled "Open file
     descriptions" in the man page for `open(2)`.)*

 22. Consider the timing of the `fclose()` call in relation to the later call
     to `fprintf()` that  caused processes to write to `fork-output.txt`.  The
     execution of `fclose()` clearly occurred before the second call to
     `fprintf()`.

     *Based on the content of `fork-output.txt`, was the later process able to
     write to the file after it was closed?  Why or why not?  (Hint: See the
     second paragraph in the "DESCRIPTION" section of the man page for
     `close(2)`.)*


# Part 6: Pipes

In this section, you will learn how pipes are created and used to communicate
between different processes.

 23. Modify `fork.c` according to the following:

     - Prior to the call to `fork()`, open a pipe (see the man page for
       `pipe()`).
     - In section B:
       - Close the file descriptor corresponding to the _read_ end of the pipe
         (see the man pages for `pipe()` and `close()`).
       - Write "hello from Section B\n" to the file descriptor corresponding to
         the _write_ end of the pipe (see the man page for `write(2)`).  Note
         that unlike `fputs()`, which takes a null-terminated string (`char *`)
         as input and writes to a buffered file stream (`FILE *`), `write()`
         simply takes a file descriptor, a pointer to a memory location and a
         number of bytes.  Thus, you will need to specify the length of the
         string.  If the length is incorrect, the command will yield unexpected
         results.
     - In section C:
       - Close the file descriptor corresponding to the _write_ end of the
         pipe (see the man pages for `pipe()` and `close()`).
       - Read from file descriptor corresponding to the _read_ end of the pipe
         (see the man page for `read(2)`) into a buffer that you have declared.
         Save the number of bytes read (return value of `read()`), and use that
         value to add a null character after them, so string operations can be
         performed on it (see the man page for `string`).
       - Print the string retrieved from `read()` to stdout.  Note that
         `printf()` and `fprintf()` require a null-terminated string, i.e., to
         know where the string ends.  If you have not properly added the null
         character, the command will yield unexpected results.

     Re-`make` and run the newly recompiled `fork`.  *Show the output of your
     program.*


# Part 7: Combining `fork()` and `exec()`

In this section, you will learn hands-on how file descriptors are inherited by
child processes and maintained after a call to `exec`.

 24. Modify `fork.c` according to the following:

     - Copy the contents of the `main()` function in `exec.c` into `fork.c` in
       such a way that the _child_ process created with the call to `fork()`
       runs the `execve()` call with the first command-line argument passed to
       `fork`. The contents you paste should immediately precede the `exit(0)`
       statement called by the child process.
     - Comment out the 30-second call to `sleep()` copied over from `exec.c`.

     Re-`make` and execute the following:

     ```
     echo foobar | ./fork /bin/cat
     ```

     *Show the output from running the pipeline.*

     Note that you might find that the buffer associated with the open file
     stream (i.e., `FILE *`) is destroyed--as part of the re-initialization
     of the stack and heap, in connection with `execve()`, before it is ever
     flushed.  In this case, the statements you wrote to `fork-output.txt`
     in the child process_before_ calling `execve()` will never make it to the
     file!  You could fix this by using `fflush()` immediately after `fprintf()`.

# Part 8: File Descriptor Duplication

In this section, you will learn hands-on how file descriptors can be duplicated
using `dup2()`.

 25. Modify `fork.c` according to the following:

     - Immediately before calling `execve()`, duplicate the file descriptor
       associated with the file stream you opened in connection with
       `fork-output.txt` such that the standard output of the child process
       goes to file descriptor associated with that stream instead (see the man
       pages for `fileno()` and `dup2()`).  Pay special attention to the
       ordering of the arguments passed to `dup2()`, or this will not work
       properly.

     Re-make and execute the following to show that it works:

     ```
     echo foobar | ./fork /bin/cat
     ```

     *Show the output from running the pipeline. Also show the contents of
     `fork-output.txt`.*
