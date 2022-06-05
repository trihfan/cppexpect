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
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <thread>

using namespace std::chrono;

void cppexpect::cppexpect::set_redirect_child_output(bool redirect)
{
    redirect_child_output = redirect;
}

bool cppexpect::cppexpect::start(const std::string& command)
{
    if (is_running())
    {
        return false;
    }

    fdm = posix_openpt(O_RDWR);
    if (fdm < 0)
    {
        std::cout << "Error " << errno << " on posix_openpt()" << std::endl;
        return false;
    }

    if (grantpt(fdm) != 0)
    {
        std::cout << "Error " << errno << " on grantpt()" << std::endl;
        return false;
    }

    if (unlockpt(fdm) != 0)
    {
        std::cout << "Error " << errno << " on unlockpt()" << std::endl;
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

bool cppexpect::cppexpect::stop(std::chrono::milliseconds timeout)
{
    bool result = true;
    if (child_pid && is_running())
    {
        // Ask for termination
        if (kill(child_pid, SIGTERM) == -1)
        {
            std::cout <<  "Error " << errno << " on kill(SIGTERM) child process" << std::endl;
            result = false;
        }
        else
        {
            result = join(timeout);
        }
    }
    child_pid = 0;
    return result;
}

bool cppexpect::cppexpect::is_running() const
{
    return child_pid > 0 && waitpid(child_pid, NULL, WNOHANG) == 0;
}

bool cppexpect::cppexpect::join(std::chrono::milliseconds timeout)
{
    int read_bytes = 0;
    char buffer[255];
    auto start = steady_clock::now();
    while (is_running() || read_bytes > 0)
    {
        if (duration_cast<milliseconds>(steady_clock::now() - start) >= timeout)
        {
            return false;
        }
        if (redirect_child_output)
        {
            read_bytes = read_output(buffer, sizeof(buffer));
        }
        std::this_thread::yield();
    }
    return true;
}

int cppexpect::cppexpect::expect(const std::regex& pattern, std::chrono::milliseconds timeout)
{
    return expect(std::vector<std::regex>{ pattern });
}

int cppexpect::cppexpect::expect(const std::vector<std::regex>& patterns, std::chrono::milliseconds timeout)
{
    return expect_loop([&patterns](const std::string& output)
    {
        // Search values
        int i = 0;
        for (auto it = patterns.begin(); it != patterns.end(); ++it, i++)
        {
            if (std::regex_match(output, *it))
            {
                return i;
            }
        }
        return -1;
    }, timeout);
}

int cppexpect::cppexpect::expect_exact(const std::string& value, std::chrono::milliseconds timeout)
{
    return expect_exact(std::vector<std::string>{ value });
}

int cppexpect::cppexpect::expect_exact(const std::vector<std::string>& values, std::chrono::milliseconds timeout)
{
    return expect_loop([&values](const std::string& output)
    {
        // Search values
        int i = 0;
        for (auto it = values.begin(); it != values.end(); ++it, i++)
        {
            if (output.find(*it) != std::string::npos)
            {
                return i;
            }
        }
        return -1;
    }, timeout);
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

int cppexpect::cppexpect::read_output(char* buffer, size_t buffer_len)
{
    // Check if there is something to read, so we won't block on read()
    fd_set fd_in {};
    FD_SET(fdm, &fd_in);
    timeval timeout { 1, 0 };
    auto desc_set_count = select(fdm + 1, &fd_in, NULL, NULL, &timeout);
    if (desc_set_count == -1)
    {
        std::cout <<  "Error " << errno << " on select()" << std::endl;
        return -1;
    }

    // If data on master side of PTY
    if (FD_ISSET(fdm, &fd_in))
    {
        // Read output
        auto read_bytes = ::read(fdm, buffer, buffer_len);
        if (read_bytes == -1)
        {
            std::cout <<  "Error " << errno << " on read master PTY" << std::endl;
            return -1;
        }

        // Output to stdout
        if (redirect_child_output)
        {
            std::cout << std::string(buffer, read_bytes);
            std::cout.flush();
        }
        return read_bytes;
    }
    return 0;
}

int cppexpect::cppexpect::expect_loop(std::function<int(const std::string&)>&& loop_function, std::chrono::milliseconds timeout)
{
    // Start the loop
    current_output.clear();
    current_output.reserve(255);
    auto read_start = steady_clock::now();
    char buffer[255];

    // Loop
    while (duration_cast<milliseconds>(steady_clock::now() - read_start) < timeout)
    {
        // Read output
        int read_bytes = read_output(buffer, sizeof(buffer));
        if (read_bytes != 0)
        {
            current_output += std::string(buffer, read_bytes);

            // Run expect function
            int result = loop_function(current_output);
            if (result >= 0)
            {
                return result;
            }
        }
    }
    return -1;
}


const std::string& cppexpect::cppexpect::get_last_output() const
{
    return current_output;
}
