#include <iostream>
#include "vk_computing.hpp"

int main()
{
    vulkan::ComputingPipeline pipeline;
    std::cout << "Calcul mandelbrot sur CPU" << std::endl << std::flush;
    pipeline.cpu_computation();
    std::cout << "=============================================================================================================" << std::endl;
    std::cout << "Calcul mandelbrot sur GPU" << std::endl << std::flush;
    pipeline.run();
    return EXIT_SUCCESS;
}
