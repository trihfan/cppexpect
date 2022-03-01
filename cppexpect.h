#include <string>
#include <chrono>
#include <initializer_list>
#include <unistd.h>

namespace cppexpect
{
    class cppexpect
    {
    public:
        // Start/stop
        bool start(const std::string& command);
        void stop();
        bool is_running() const;

        // Timeout for expect calls
        void set_timeout(std::chrono::milliseconds timeout);

        // Read bytes
        int expect(const std::string& output);

        // Send bytes
        void write(const std::string& value);
        void write_line(const std::string& value);

    private:
        int fdm;
        int fds;
        pid_t child_pid = 0;
        std::chrono::milliseconds timeout = std::chrono::seconds(30);

        void launch_as_child(const std::string& command);

        int expect(const std::string& output);
    };
}
