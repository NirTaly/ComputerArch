#include <iostream>
#include <algorithm>
#include <list>
#include <vector>

using std::list;

const bool WRITE = true;
const bool READ = false;

static uint32_t log2(uint32_t n)
{
    int count = 0;
    for (; n != 0; count++, n >>= 1)
    {
    }
    return count - 1;
}

class Block
{
public:
    Block(unsigned long address = 0, bool dirty = false, bool valid = false) 
        : address(address), dirty(dirty), valid(valid) { }
    ~Block() = default;
    bool isDirty() const                    { return dirty;         }
    unsigned long int getAddress() const    { return address;       }
    bool isValid() const                    { return valid;         }
    void setAddress(unsigned long n_address){ address = n_address;  }
    void setDirty(bool n_dirty)             { dirty = n_dirty;      }
    void setValid()                         { valid = false;        }

    // For Debuging
    friend std::ostream& operator<< (std::ostream& out, const Block& b) { 
		out << "address = " << b.address << ", dirty = " << std::boolalpha << b.dirty << ", valid = " << b.valid;
		return out;
    }
private:
    bool valid;
    bool dirty;
    unsigned long int address;
};

class CacheRow
{
public:
    CacheRow(int block_size, int assoc);

    bool search(unsigned long int tag);
    Block& findBlock(unsigned long int tag);
    Block& update(unsigned long new_address, bool write); //return old_address
    bool isDirty(unsigned long int tag);

    // For Debuging
    friend std::ostream& operator<< (std::ostream& out, const CacheRow& cr) { 
        for (const Block& b : cr.address_list)
        {
		    out << b << " | ";
        }
		return out;
    }
private:
    list<Block> address_list;
    int block_size;
};

class LevelCache
{
public:
    LevelCache(unsigned int size, unsigned int assoc, unsigned int block_size);
    bool search(unsigned long int address);
    bool isDirty(unsigned long int address);
    /**
     * @brief insert address to cache.
     * if already in: make block as LRU
     * else: insert new block , and make as LRU
     * 
     * @param address address of new op.
     * @return the overwriten block (prev).
     * if prev.address == address -> same block and only LRU update
     */
    Block &insert(unsigned long int address);

    /**
     * @brief insert new address to cache
     * 
     * @param address 
     * @param old_dirty 
     * @param dirty  true if we write, false if we read.
     * @return old address (LRU address),
     */
    unsigned long int insert(unsigned long int address, bool &old_dirty, bool dirty = false);

    /**
     * @brief address is already in cache, so need to update place in LRU (become front of the list)
     * 
     * @param address 
     * @param dirty true if we write, false if we read.
     */
    void update(unsigned long int address, bool dirty = false);
    void remove(unsigned long int address);
    int getNumberOfAccess() { return num_of_access; }
    int getNumberOfMiss() { return num_of_miss; }

    // For Debuging
    friend std::ostream& operator<< (std::ostream& out, const LevelCache& lc) { 
        for (int i = 0; i < lc.cache_sets.size(); i++)
        {
		    out << i << "\t:" << lc.cache_sets[i] << std::endl;
        }
		return out;
    }
private:
    std::vector<CacheRow> cache_sets;
    unsigned int set_size;
    unsigned long int num_of_access;
    unsigned int num_of_miss;
    uint64_t set_mask;
};

/*******************************************************************************/

class MemCache
{
private:
    bool allocate;
    unsigned int mem_cyc;
    unsigned int l1_cyc;
    unsigned int l2_cyc;
    LevelCache L1;
    LevelCache L2;
    unsigned int num_of_access;
    unsigned int num_of_mem_access;
    unsigned int block_s;

    void L1Insert(unsigned long int block_address, bool dirty = false);
    void L2Insert(unsigned long int block_address, bool dirty = false);
    void writeAllocate(unsigned long int block_address);
    void writeNoAllocate(unsigned long int block_address);

public:
    MemCache(unsigned int MemCyc, unsigned int BSize, unsigned int L1Size, unsigned int L2Size, unsigned int L1Assoc, unsigned int L2Assoc, unsigned int L1Cyc, unsigned int L2Cyc, unsigned int WrAlloc);
    ~MemCache() = default;
    void read(unsigned long int address);
    void write(unsigned long int address);
    void getRates(double &L1MissRate, double &L2MissRate, double &avgAccTime);
};

/*********************************************************************************************/
/*									MemCache functions   									 */
/*********************************************************************************************/

/**
 * @brief Inserts the new block to L1. if the old block was dirty it updates L2.
 * 
 * @param block_address the new block to insert
 * @param dirty true if we write to the block
 */
void MemCache::L1Insert(unsigned long int block_address, bool dirty = false)
{
    bool old_dirty = false;                                                     //                    tag  set
    unsigned long int old_address = L1.insert(block_address, old_dirty, dirty); // need to send as:   xxxx yyy      without offset
    if (old_dirty)
        L2.update(old_address);
}

/**
 * @brief Inserts the new block to L2. 
 * if the old block was dirty in the theory we need to update mem - we done nothing
 * we also need to remove the old block from L1. In the theory if it is dirty we have to update the mem.
 * 
 * @param block_address the new block to insert
 * @param dirty true if we write to the block
 */
void MemCache::L2Insert(unsigned long int block_address, bool dirty = false)
{
    bool old_dirty = false;                                                     //                    tag  set
    unsigned long int old_address = L2.insert(block_address, old_dirty, dirty); // need to send as:   xxxx yyy
    L1.remove(old_address);
}

/**
 * @brief execute write with allocation
 * 
 * @param block_address the address of the block to write to
 */
void MemCache::writeAllocate(unsigned long int block_address)
{
    if (L1.search(block_address))
        L1.update(block_address, true);
    else
    {
        L1Insert(block_address, true);
        if (L2.search(block_address))
            L2.update(block_address);
        else
        {
            L2Insert(block_address);
            num_of_mem_access++;
        }
    }
}

/**
 * @brief execute write without allocate
 * 
 * @param block_address the address of the block to write to
 */
void MemCache::writeNoAllocate(unsigned long int block_address)
{
    if (L1.search(block_address))
        L1.update(block_address, true);
    else if (L2.search(block_address))
        L2.update(block_address, true);
    else
        num_of_mem_access++;
}

/**
 * @brief Construct a new Mem Cache:: Mem Cache object
 * 
 * @param MemCyc number of cycel to access the memory
 * @param BSize Block size (log2)
 * @param L1Size level 1 cache size (log2)
 * @param L2Size level 2 cache size (log2)
 * @param L1Assoc level1 associtivity (log2)
 * @param L2Assoc level2 associtivity (log2)
 * @param L1Cyc number of cycel to access the level1 cache
 * @param L2Cyc number of cycel to access the level2 cache
 * @param WrAlloc true if write-allocate, false if write through
 */
MemCache::MemCache(unsigned int MemCyc, unsigned int BSize, unsigned int L1Size, unsigned int L2Size, unsigned int L1Assoc, unsigned int L2Assoc, unsigned int L1Cyc, unsigned int L2Cyc, unsigned int WrAlloc)
    : allocate(WrAlloc), mem_cyc(MemCyc), l1_cyc(L1Cyc), l2_cyc(L2Cyc), L1(L1Size, L1Assoc, BSize), L2(L2Size, L2Assoc, BSize), num_of_access(0), num_of_mem_access(0), block_s(BSize) {}

/**
 * @brief simulate read request
 * 
 * @param address the address to read from
 */
void MemCache::read(unsigned long int address)
{
    num_of_access++;
    unsigned long int block_address = address >> block_s;
    bool dirty = false;
    if (L1.search(block_address))
    {
        L1.update(block_address);
    }
    else if (L2.search(block_address))
    {
        L2.update(block_address);
        L1Insert(block_address);
    }
    else
    {
        num_of_mem_access++;
        L2Insert(block_address);
        L1Insert(block_address);
    }
}

/**
 * @brief simulate write request
 * 
 * @param address the address to write to
 */
void MemCache::write(unsigned long int address)
{
    num_of_access++;
    unsigned long int block_address = address >> block_s;
    if (allocate)
        writeAllocate(block_address);
    else
        writeNoAllocate(block_address);
}

/**
 * @brief calculate and return the rates in the end of the simulation
 * 
 * @param L1MissRate  miss rate of level1 cache
 * @param L2MissRate  miss rate of level2 cache
 * @param avgAccTime  avarage access time
 */
void MemCache::getRates(double &L1MissRate, double &L2MissRate, double &avgAccTime)
{
    double L1_miss = L1.getNumberOfMiss();
    int L2_access = L2.getNumberOfAccess();
    double L2_miss = L2.getNumberOfMiss();
    L1MissRate = L1_miss / num_of_access;
    L2MissRate = L2_miss / L2_access;
    double tot_time = (num_of_access * l1_cyc) + (L2_access * l2_cyc) + (num_of_mem_access * mem_cyc);
    avgAccTime = tot_time / num_of_access;
}

/*********************************************************************************************/
/*									LevelCache functions   									 */
/*********************************************************************************************/

LevelCache::LevelCache(unsigned int size, unsigned int assoc, unsigned int block_size)
    : cache_sets((1 << block_size) / assoc, CacheRow(block_size, assoc)), set_size(log2(cache_sets.size())),
      num_of_access(0), num_of_miss(0)
{
    set_mask = INT64_MAX;
    set_mask >>= (8 * sizeof(int64_t)) - set_size;
}

bool LevelCache::search(unsigned long int address) //if miss you need to update the number of miss + access
{
    num_of_access++;

    uint64_t set = address & set_mask;
    uint64_t tag = address >> set_size;

    bool found = cache_sets[set].search(tag);
    num_of_miss += found ? 0 : 1;

    return found;
}

bool LevelCache::isDirty(unsigned long int address)
{
    uint64_t set = address & set_mask;
    uint64_t tag = address >> set_size;

    return cache_sets[set].isDirty(tag);
}

Block &LevelCache::insert(unsigned long int address)
{
    uint64_t set = address & set_mask;
    uint64_t tag = address >> set_size;

    return cache_sets[set].update(tag, WRITE);
}

unsigned long int LevelCache::insert(unsigned long int address,bool &old_dirty, bool dirty)
{
    Block &block = insert(address);
    old_dirty = block.isDirty();

    return block.getAddress();
}

void LevelCache::update(unsigned long int address, bool dirty)
{
    insert(address);
}

/*******************************************************************************/
CacheRow::CacheRow(int block_size, int assoc) : address_list(assoc), block_size(block_size) {}

bool CacheRow::search(unsigned long int tag)
{
    try
    {
        Block& b = findBlock(tag);
        return true;
    }
    catch(const std::exception&)
    {
        return false;
    }
}
Block& CacheRow::findBlock(unsigned long int tag)
{
    auto iter = std::find_if(address_list.begin(),address_list.end(), [tag](const Block& b){ return b.getAddress() == tag; });
    
    if (iter == address_list.end()) throw std::runtime_error("Not Found");

    return *iter;
}
Block& CacheRow::update(unsigned long new_address, bool write)
{
    
}
bool CacheRow::isDirty(unsigned long int tag)
{
    auto iter = std::find_if(address_list.begin(),address_list.end(), [tag](const Block& b){ return (b.getAddress() == tag && b.isDirty()); });
    
    return iter != address_list.end();
}
