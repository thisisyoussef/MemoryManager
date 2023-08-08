

#include <fcntl.h>

#include <functional>
#include <unistd.h>
#include <cmath>
#include <vector>
using namespace std;

int bestFit(int sizeInWords, void *list);
int worstFit(int sizeInWords, void *list);

class Node
{
public:
    int head = 0;

    int length = 0;
    bool isHole = true;

    Node *next = nullptr;
    Node *prev = nullptr;

    Node(int _ishead, bool _isHole, int _length, Node *_prev, Node *_next)
    {
        this->head = _ishead;
        this->next = _next;
        this->prev = _prev;
        this->isHole = _isHole;
        this->length = _length;
    }
};
class MemoryManager
{
private:
    uint8_t *strt = nullptr;
    uint8_t *bMap = nullptr;
    uint16_t *list = nullptr;

    size_t wordSize = 0;

    unsigned sizeWords = 0;
    unsigned memLimit = sizeWords * wordSize;

    function<int(int, void *)> allocFunction;

    Node *node = nullptr;

public:
    MemoryManager(unsigned size, function<int(int, void *)> allocator);
    void shutdown();
    ~MemoryManager();

    void *getList();

    void free(void *address);
    void setAllocator(function<int(int, void *)> allocator);
    int dumpMemoryMap(char *fileName);
    void *getBitmap();
    unsigned getWordSize();

    unsigned getMemoryLimit();
    void initialize(size_t _size);
    void *allocate(size_t byteSize);
    void *getMemoryStart();
};
