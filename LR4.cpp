#include <iostream>
#include <string>

#include "Header.h"
using namespace std;
using namespace std::filesystem;

int main()
{
    if (!exists(current_path() / "input")) {
        cout << "Input directory doesn't exist" << endl;
        return 0;
    }
    if (!exists(current_path() / "output")) {
        create_directory(current_path() / "output");
    }

    char fileNameIter = '0';
    for (const directory_entry dir : directory_iterator(current_path() / "input")) {
        FileHandler file(dir.path(), current_path() / "output", fileNameIter);
        file.handleFile();
        fileNameIter++;
    }
    return 0;
}