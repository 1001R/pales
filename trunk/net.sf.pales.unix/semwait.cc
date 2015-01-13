#include "semaphore.h"
#include <string>
#include <memory>
#include <unistd.h>

using namespace std;

static unique_ptr<Semaphore> SEM_CANCEL;

int main() {
    SEM_CANCEL.reset(new Semaphore(string("/pales:deadbeef"), O_CREAT, 0640, 0, true));
    sleep(10);
    SEM_CANCEL.reset();
    _exit(0);
    return 0;
}
