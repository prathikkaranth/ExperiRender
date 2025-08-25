#include <vk_engine.h>

int main() {
    VulkanEngine engine;

    engine.init();

    engine.run();

    engine.cleanup();

    return 0;
}