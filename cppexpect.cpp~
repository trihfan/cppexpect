#include "cppexpect.h"
#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#define __USE_BSD
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <iostream>

using namespace std::chrono;

bool cppexpect::cppexpect::start(const std::string& command)
{
    if (is_running())
    {
        return false;
    }
    fdm = posix_openpt(O_RDWR);
    if (fdm < 0)
    {
        fprintf(stderr, "Error %d on posix_openpt()\n", errno);
        return false;
    }

    auto rc = grantpt(fdm);
    if (rc != 0)
    {
        fprintf(stderr, "Error %d on grantpt()\n", errno);
        return false;
    }

    rc = unlockpt(fdm);
    if (rc != 0)
    {
        fprintf(stderr, "Error %d on unlockpt()\n", errno);
        return false;
    }

    // Open the slave PTY
    fds = open(ptsname(fdm), O_RDWR);

    // Fork to launch the command
    child_pid = fork();
    if (child_pid)
    {
        // Close the slave side of the PTY
        close(fds);
        return true;
    }

    launch_as_child(command);
    return true;
}

void cppexpect::cppexpect::stop()
{
    if (child_pid && is_running())
    {
        kill(child_pid, SIGINT);
    }
    child_pid = 0;
}

bool cppexpect::cppexpect::is_running() const
{
    return child_pid > 0 && waitpid(child_pid, NULL, WNOHANG) == 0;
}

int cppexpect::cppexpect::expect(const std::string& output, std::chrono::milliseconds timeout)
{
    // Set a file descriptor set to read data
    fd_set fd_in {};
    FD_SET(0, &fd_in);
    FD_SET(fdm, &fd_in);

    // Read buffer
    char input[255];

    auto start = steady_clock::now();
    while (duration_cast<milliseconds>(steady_clock::now() - start) < timeout)
    {
        // Check if there is something to read, so we won't block on read()
        auto rc = select(fdm + 1, &fd_in, NULL, NULL, NULL);
        if (rc == -1)
        {
            std::cout <<  "Error " << errno << " on select()" << std::endl;
            return -1;
        }

        // If data on master side of PTY
        if (FD_ISSET(fdm, &fd_in))
        {
            rc = read(fdm, input, sizeof(input));
            if (rc == -1)
            {
                std::cout <<  "Error " << errno << " on read master PTY" << std::endl;
                return -1;
            }

            // Output to stdout on debug
#ifndef NDEBUG
            std::cout << input;
#endif

            // Check for corresponding string
            if (std::string(input).find(output) != std::string::npos)
            {
                return 0;
            }
        }
    }

    return -1;
}

void cppexpect::cppexpect::write(const std::string& value)
{
    ::write(fdm, value.c_str(), value.size());
}

void cppexpect::cppexpect::write_line(const std::string& value)
{
    write(value + "\n");
}

void cppexpect::cppexpect::launch_as_child(const std::string& command)
{
    // Child
    struct termios slave_orig_term_settings; // Saved terminal settings
    struct termios new_term_settings; // Current terminal settings

    // Close the master side of the PTY
    close(fdm);

    // Save the defaults parameters of the slave side of the PTY
    tcgetattr(fds, &slave_orig_term_settings);

    // Set RAW mode on slave side of PTY
    new_term_settings = slave_orig_term_settings;
    cfmakeraw (&new_term_settings);
    tcsetattr (fds, TCSANOW, &new_term_settings);

    // The slave side of the PTY becomes the standard input and outputs of the child process
    close(0); // Close standard input (current terminal)
    close(1); // Close standard output (current terminal)
    close(2); // Close standard error (current terminal)

    dup(fds); // PTY becomes standard input (0)
    dup(fds); // PTY becomes standard output (1)
    dup(fds); // PTY becomes standard error (2)

    // Now the original file descriptor is useless
    close(fds);

    // Make the current process a new session leader
    setsid();

    // As the child is a session leader, set the controlling terminal to be the slave side of the PTY
    // (Mandatory for programs like the shell to make them manage correctly their outputs)
    ioctl(0, TIOCSCTTY, 1);

    // Execution of the program
    exit(system(command.c_str()));
}
