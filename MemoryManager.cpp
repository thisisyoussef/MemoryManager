#include "MemoryManager.h"
using namespace std;

MemoryManager::MemoryManager(unsigned wordSize, function<int(int, void *)> allocator)
{
    this->wordSize = wordSize;
    this->allocFunction = allocator;
}
MemoryManager::~MemoryManager()
{

    sbrk(-(sizeWords * wordSize));
    strt = nullptr;
    Node *temp = this->node;
    Node *temp2;
    while (temp)
    {
        temp2 = temp->next;
        delete temp;
        temp = temp2;
    }
    this->node = nullptr;
}

void MemoryManager::initialize(size_t sizeInWords)
{
    int maxSize = 65536;
    if (sizeInWords < maxSize)
    {
        this->sizeWords = sizeInWords;
        if (strt != nullptr)
        {
            sbrk(-(sizeWords * wordSize));
            strt = nullptr;
            Node *temp = this->node;
            Node *temp2;
            while (temp)
            {
                temp2 = temp->next;
                delete temp;
                temp = temp2;
            }
            this->node = nullptr;
        }
        strt = (uint8_t *)sbrk(sizeWords * wordSize);
    }
}

void MemoryManager::shutdown()
{
    sbrk(-(sizeWords * wordSize));
    strt = nullptr;
    Node *temp = this->node;
    Node *temp2;
    while (temp != nullptr)
    {
        temp2 = temp->next;
        delete temp;
        temp = temp2;
    }
    this->node = nullptr;
}

void *MemoryManager::allocate(size_t sizeInBytes)
{
    if (this->node == nullptr)
    {
        this->node = new Node(0, true, sizeWords, nullptr, nullptr);
    }
    int length = ceil((float)sizeInBytes / wordSize);
    int offset = allocFunction(length, (void *)getList());
    delete[] list;
    if (offset == -1)
    {
        return nullptr;
    }
    Node *temp = this->node;

    while (temp != nullptr && temp->head != offset)
    {
        temp = temp->next;
    }
    if (length == temp->length)
    {
        temp->isHole = false;
    }
    else
    {
        Node *temp1 = temp->next;
        temp->isHole = false;
        temp->next = new Node(temp->head + length, true, temp->length - length, temp, temp1);
        temp->length = length;
    }
    return this->strt + (offset * wordSize);
}

void MemoryManager::free(void *address)
{
    Node *temp = node;
    int offset = ceil(((uint8_t *)address - this->strt) / wordSize);

    while (temp != nullptr)
    {
        if (temp->head == offset)
        {
            temp->isHole = true;
        }
        temp = temp->next;
    }

    Node *tmp = node;
    Node *temp2 = nullptr;
    while (tmp != nullptr)
    {

        while (tmp->isHole && tmp->next != nullptr && tmp->next->isHole)
        {
            temp2 = tmp->next->next;
            tmp->length += tmp->next->length;
            delete tmp->next;
            tmp->next = temp2;
        }
        tmp = tmp->next;
    }
}

void MemoryManager::setAllocator(std::function<int(int, void *)> allocator)
{
    this->allocFunction = allocator;
}

int MemoryManager::dumpMemoryMap(char *filename)
{
    int file = open(filename, O_CREAT | O_WRONLY, 0600);

    if (file < 0)
    {
        return -1;
    }

    uint16_t *hole_List = static_cast<uint16_t *>(getList());
    uint16_t length = *hole_List++;
    string s;
    for (int i = 0; i < length * 2; i += 2)
    {
        if (i == 0)
        {
            s += "[" + to_string(hole_List[i]) + ", " + to_string(hole_List[i + 1]) + "]";
        }
        else
        {
            s += " - [" + to_string(hole_List[i]) + ", " + to_string(hole_List[i + 1]) + "]";
        }
    }
    char *buff = const_cast<char *>(s.c_str());
    write(file, buff, s.length());
    close(file);
    delete[] this->list;
    return 0;
}

void *MemoryManager::getList()
{
    Node *temp = this->node;
    uint16_t count;

    vector<uint16_t> vec = {};
    uint16_t l = 0;

    while (temp != nullptr)
    {
        if (temp->isHole)
        {
            l++;
            vec.push_back(temp->head);
            vec.push_back(temp->length);
        }
        temp = temp->next;
    }
    int lSize = (2 * l) + 1;
    this->list = new uint16_t[lSize];
    this->list[0] = l;
    for (count = 0; count < l * 2; count++)
    {
        this->list[count + 1] = vec[count];
    }
    return this->list;
}

void *MemoryManager::getBitmap()
{
    int s = ceil((float)sizeWords / 8);

    Node *temp = this->node;
    vector<bool> vec = {};
    while (temp)
    {
        for (int i = 0; i < temp->length; i++)
        {
            vec.push_back(!temp->isHole);
        }
        temp = temp->next;
    }
    int pos = 2;
    uint8_t count = 0;

    int bit_map_size = 2 + s;
    this->bMap = new uint8_t(bit_map_size);
    for (int i = 0; i < vec.size(); i += 8)
    {
        for (int j = 0; j < 8; j++)
        {
            if (i + j < vec.size())
            {
                count += pow(2, j) * vec[i + j];
            }
        }
        bMap[pos] = count;
        count = 0;
        pos++;
    }

    bMap[1] = (s >> 8) & ((1 << 8) - 1);
    bMap[0] = s & ((1 << 8) - 1);
    return this->bMap;
}

int bestFit(int sizeInWords, void *list)
{
    int diff = INT16_MAX;
    int offset = -1;
    uint16_t *hole_List = static_cast<uint16_t *>(list);
    uint16_t length = *hole_List++;
    for (uint16_t i = 1; i < (length)*2; i += 2)
    {
        if (hole_List[i] >= sizeInWords && hole_List[i] < diff)
        {
            diff = hole_List[i];
            offset = hole_List[i - 1];
        }
    }
    return offset;
}

int worstFit(int sizeInWords, void *list)
{

    uint16_t *holesList = static_cast<uint16_t *>(list);
    uint16_t len = *holesList++;
    int diff = -1;
    int offset = -1;
    for (uint16_t i = 1; i < (len)*2; i += 2)
    {
        if (holesList[i] >= sizeInWords && holesList[i] > diff)
        {
            diff = holesList[i];
            offset = holesList[i - 1];
        }
    }
    return offset;
}

unsigned MemoryManager::getWordSize()
{
    return wordSize;
}
void *MemoryManager::getMemoryStart()
{
    return strt;
}
unsigned MemoryManager::getMemoryLimit()
{
    return sizeWords * wordSize;
}
