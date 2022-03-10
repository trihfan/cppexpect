#include <string>
#include <chrono>
#include <unistd.h>
#include <regex>
#include <functional>
#include <vector>
#include <limits>

namespace cppexpect
{
    static constexpr std::chrono::milliseconds inf_timeout = std::chrono::milliseconds(std::numeric_limits<int64_t>::max());

    class cppexpect
    {
    public:
        // Redirect output
        void set_redirect_child_output(bool redirect);

        // Start/stop
        bool start(const std::string& command);
        bool stop(std::chrono::milliseconds timeout = inf_timeout);
        bool is_running() const;

        // Wait until the child process end
        bool join(std::chrono::milliseconds timeout = inf_timeout);

        // Read and expect patterns
        int expect(const std::regex& pattern, std::chrono::milliseconds timeout = std::chrono::seconds(5));
        int expect(const std::vector<std::regex>& patterns, std::chrono::milliseconds timeout = std::chrono::seconds(5));

        // Read and expect exact string
        int expect_exact(const std::string& value, std::chrono::milliseconds timeout = std::chrono::seconds(5));
        int expect_exact(const std::vector<std::string>& values, std::chrono::milliseconds timeout = std::chrono::seconds(5));

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

        // Launch the specific command while being in the child process
        void launch_as_child(const std::string& command);

        // Try to read the child output and return the number of read bytes or -1 in case of error
        int read_output(char* buffer, size_t buffer_len);

        // Loop and read child process output until the timeout is reached or loop_function return -1
        // the loop function is called for each line outputed by the child process
        int expect_loop(std::function<int(const std::string&)>&& loop_function, std::chrono::milliseconds timeout);
    };
}
