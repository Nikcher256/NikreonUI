#include "Engine/UI/BasicWidgets.hpp"

#include <iostream>

int main()
{
    Engine::ProgressBar loading{"loading", 0.65f};
    loading.setBounds({12.0f, 12.0f}, {180.0f, 18.0f});
    std::cout << "NikreonUI::Core sample: loading=" << loading.value() << '\n';
}
