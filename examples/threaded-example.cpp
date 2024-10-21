#include "tracer.hpp"

void func();

int main()
{
    TRACE_START("/tmp/trace.json");
    func();
    TRACE_STOP();
    return 0;
}
