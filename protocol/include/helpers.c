/// BEGIN TIME
LARGE_INTEGER frequency;        // ticks per second
LARGE_INTEGER t1, t2;           // ticks

void StartCounter()
{
    if(!QueryPerformanceFrequency(&frequency))
      printf("QueryPerformanceFrequency failed!\n");

    QueryPerformanceCounter(&t1);
}
double GetCounter()
{
    QueryPerformanceCounter(&t2);
    return (t2.QuadPart-t1.QuadPart) * 1000.0/frequency.QuadPart;
}
/// END TIME


uint32_t Hash(char *key, size_t len)
{
    uint32_t hash, i;
    for(hash = i = 0; i < len; ++i)
    {
        hash += key[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}