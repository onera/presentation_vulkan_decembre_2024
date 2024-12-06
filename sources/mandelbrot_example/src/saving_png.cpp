#include <iostream>
#include "vk_computing.hpp"
#include "lodepng.h"

namespace vulkan {
void 
ComputingPipeline::save_rendered_image(Pixel* pmapped_memory, std::string const& filename)
{
    std::cout << "Calcul image definitive" << std::endl << std::flush; 
    // on lit les données de couleurs du buffer qu'on transforme en octets et on sauve les données dans un tableau: 
    std::vector<unsigned char> image; image.reserve(width * height * 4);
#   pragma omp for simd
    for (int i = 0; i < width * height; i += 1) 
    {
        image.push_back((unsigned char)(255.0f * (pmapped_memory[i].r)));
        image.push_back((unsigned char)(255.0f * (pmapped_memory[i].g)));
        image.push_back((unsigned char)(255.0f * (pmapped_memory[i].b)));
        image.push_back((unsigned char)(255.0f * (pmapped_memory[i].a)));
    }

    // Enfin, on sauvegarde le tableau de couleurs dans un fichier png :
    unsigned error = lodepng::encode(filename, image, width, height);
    if (error) std::cerr << "Encoder error " << error << ": " << lodepng_error_text(error) << std::endl;
}
}