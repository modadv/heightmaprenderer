#include "common/entry/entry_p.h"
#include "common/entry/entry_qt.h"

int main(int _argc, char** _argv)
{
    int result = entry::main(_argc, const_cast<const char**>(_argv));

    if (entry::s_engine)
    {
        delete entry::s_engine;
        entry::s_engine = nullptr;
    }

    if (entry::s_app)
    {
        delete entry::s_app;
        entry::s_app = nullptr;
    }

    return result;
}