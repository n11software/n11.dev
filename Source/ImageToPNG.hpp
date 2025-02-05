#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"  // For loading images
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"  // For writing images

#include <iostream>
#include <vector>
#include <string>

typedef struct {
    std::vector<unsigned char> buffer;
} WriteContext;

void writeCallback(void *context, void *data, int size) {
    // Cast the context to WriteContext
    WriteContext *writeContext = (WriteContext*)context;
    unsigned char *byteData = (unsigned char*)data;

    // Append the data to the buffer
    writeContext->buffer.insert(writeContext->buffer.end(), byteData, byteData + size);
}

bool convertToPNG(const std::string& inputData, std::string& outputPNGData) {
    int width, height, channels;

    // Load the image from the input string (raw image data)
    unsigned char* image = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(inputData.data()), inputData.size(), &width, &height, &channels, 0);
    if (image == nullptr) {
        std::cerr << "Error loading image from memory." << std::endl;
        return false;
    }

    // Set up the write context (our custom memory buffer)
    WriteContext writeContext;
    
    // Call the function to write the PNG data to memory using the custom write callback
    stbi_write_png_to_func(writeCallback, &writeContext, width, height, channels, image, width * channels);
    
    // Free the loaded image data
    stbi_image_free(image);

    // Copy the PNG data from the buffer to the output string
    outputPNGData.assign(writeContext.buffer.begin(), writeContext.buffer.end());

    return true;
}