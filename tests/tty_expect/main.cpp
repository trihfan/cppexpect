#include "cppexpect.h"

int main(int argc, char** argv)
{
    cppexpect::cppexpect expect;
    expect.set_timeout(std::chrono::seconds(30));
    expect.set_redirect_child_output(true);
    expect.start("sudo openconnect");
    if (expect.expect_exact("Password:") != 0)
    {
        return 1;
    }

    expect.stop();
    expect.wait_for();
    return 0;
}
