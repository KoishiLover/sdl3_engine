#include "engine/engine_frame.h"


int main() {
    auto&  instance = IENGINE ;
    if (!instance.Init()) {
        instance.Destroy();
    }
    instance.Run();
    return 0;

}