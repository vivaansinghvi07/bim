#include <string>

#ifndef EDITOR_BUFFER
#define EDITOR_BUFFER

class Buffer {
        std::string filename;
        std::string contents;
public:
        Buffer(std::string& file_name);
        void save();
};

#endif // !EDITOR_BUFFER
