#include <core/app.h>

// The engine loop lives in core::App; main only constructs it and runs it.
int main() {
    core::App app;
    return app.run();
}