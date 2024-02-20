#include <cstdlib>
#include <string>
#include <fstream>
#include <iostream>

#include "buffer.hpp"

Buffer::Buffer(std::string& file_name) {
        filename = file_name;  // i don't like this
        contents = std::string();
        std::ifstream file_stream(file_name);
        if (!file_stream.is_open()) {
                std::cout << "File path not found: " << file_name << std::endl;
                std::exit(1);
        }
        std::string temp; 
        while (file_stream.good()) {
                file_stream >> temp;
                contents.append(temp);
        }
}

void Buffer::save() {
        std::ifstream file_stream(filename);
        // TODO
}

