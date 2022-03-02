#include <string>
#include <chrono>
#include <initializer_list>
#include <unistd.h>
#include <regex>
#include <functional>

namespace cppexpect
{
    class cppexpect
    {
    public:
        // Redirect output
        void set_redirect_child_output(bool redirect);

        // Start/stop
        bool start(const std::string& command);
        void stop();
        bool is_running() const;

        // Wait until the child process end
        void wait_for();

        // Timeout for expect calls
        void set_timeout(std::chrono::milliseconds timeout);

        // Read and expect patterns
        int expect(const std::regex& pattern);
        int expect(std::initializer_list<std::regex> patterns);

        // Read and expect exact string
        int expect_exact(const std::string& value);
        int expect_exact(std::initializer_list<std::string> values);

        // Send bytes
        void write(const std::string& value);
        void write_line(const std::string& value);

    private:
        // Flag to redirect child output to father output
        bool redirect_child_output = false;

        // File descriptor for child process
        int fdm, fds;
        fd_set fd_in;

        // Child process pid
        pid_t child_pid = 0;

        // Timeout
        std::chrono::milliseconds timeout = std::chrono::seconds(30);

        // Launch the specific command while being in the child process
        void launch_as_child(const std::string& command);

        // Try to read the child output and return the number of read bytes or -1 in case of error
        int read_output(char* buffer, size_t buffer_len);

        // Loop and read child process output until the timeout is reached or loop_function return -1
        // the loop function is called for each line outputed by the child process
        int expect_loop(std::function<int(const std::string&)>&& loop_function);
    };
}
