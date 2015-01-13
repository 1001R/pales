#include "file.h"

int main() {
    File f("/tmp/foo", O_CREAT, 0640);
    f.write("foo", 3);
    f.sync();
    return 0; 
}
