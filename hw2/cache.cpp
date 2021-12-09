#include <list>
#include <vector>

using std::list;

class Block{
public:
    Block(unsigned long address, bool dirty = false, bool valid = false) 
        : address(address), dirty(dirty), valid(valid) { }
    ~Block() = default;
    bool isDirty() { return dirty; }
    unsigned long int getAddress() { return address; }
    bool isValid() { return valid; }
    void setAddress(unsigned long n_address) { address = n_address; }
    void setDirty(bool n_dirty) { dirty = n_dirty; }
    void setValid() { valid = false; }
private:
    bool valid;
    bool dirty;
    unsigned long int address;
};

class CacheRow
{
public:
    CacheRow(int block_size, int assoc);
    ~CacheRow() = default;
    bool search();
    Block& update(unsigned long new_address, bool write); //return old_address
    
private:
    list<Block> address;
    int block_size;
};

class LevelCache
{
private:
    std::vector<CacheRow> cache_sets;
    int number_of_rows;
    int set_size;
    int block_size;

public:
    LevelCache();
    ~LevelCache() = default;
    bool search();
    bool checkDirty();   
    int insert(); //dirty + address
    void update();
};


/*******************************************************************************/



class MemCache
{
private:
    bool allocate;
    int mem_cyc;
    int l1_cyc;
    int l2_cyc;
    LevelCache L1;
    LevelCache L2;
    
public:
    MemCache(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Assoc, unsigned L2Assoc,unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc);
    ~MemCache() = default;
    void read(unsigned long int address);
    void write(unsigned long int address);
    void getRates(double& L1MissRate, double& L2MissRate,double& avgAccTime);
};

MemCache::MemCache(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Assoc, unsigned L2Assoc,unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc): allocate(WrAlloc), mem_cyc(MemCyc), l1_cyc(L1Cyc), l2_cyc(L2Cyc), L1(L1Size, L1Assoc, BSize), L2(L2Size, L2Assoc, BSize) {};
void MemCache::read(unsigned long int address);
void MemCache::write(unsigned long int address);
void MemCache::getRates(double& L1MissRate, double& L2MissRate,double& avgAccTime);