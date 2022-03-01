#include "cppexpect.h"

int main(int argc, char** argv)
{
    cppexpect::cppexpect expect;
    expect.start("sudo ls");
    if (expect.expect("Password:", std::chrono::seconds(3)) != 0)
    {
        return 1;
    }

    expect.write("23vivegohan?T1\n");
    expect.expect("blabla");
    return 0;
}
