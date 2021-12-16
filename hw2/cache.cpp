#include <iostream>
#include <algorithm>
#include <list>
#include <vector>

#ifndef CACHE_CPP
#define CACHE_CPP

using std::list;

const bool WRITE = true;
const bool READ = false;
/**
static uint32_t log2(uint32_t n)
{
    int count = 0;
    for (; n != 0; count++, n >>= 1)
    {
    }
    return count - 1;
}**/

class Block
{
public:
    Block(unsigned long address = 0, bool dirty = false, bool valid = false) 
        : address(address), dirty(dirty), valid(valid) { }
    
    bool isDirty() const                    { return dirty;                 }
    unsigned long int getAddress() const    { return address;               }
    bool isValid() const                    { return valid;                 }
    void setAddress(unsigned long n_address){ address = n_address;          }
    void setDirty(bool n_dirty)             { dirty = n_dirty;              }
    void setValid()                         { valid = false;                }
    bool operator==(const Block& other)     {return other.address==address; }

    // For Debuging
    friend std::ostream& operator<< (std::ostream& out, const Block& b) { 
		// out << "address = " << b.address << ", dirty = " << std::boolalpha << b.dirty << ", valid = " << b.valid;
		out << "(" << b.address;
        if (!b.valid)
            out << ", *";
        if (b.dirty)
            out << ", D";
        
        out << ")";

		return out;
    }
private:
    unsigned long int address;
    bool dirty;
    bool valid;
};

class CacheRow
{
public:
    CacheRow(int assoc);

    bool search(unsigned long int tag);
    Block& findBlock(unsigned long int tag);
    Block update(unsigned long new_address, bool write);
    Block insert(unsigned long new_address, bool write);
    void remove(unsigned long int address);
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
    // unsigned long int block_size;
};

class LevelCache
{
public:
    LevelCache(unsigned int size, unsigned int assoc);
    bool search(unsigned long int address);
    bool isDirty(unsigned long int address);
    /**
     * @brief insert address to cache.
     * if already in: make block as Most Recently Used
     * else: insert new block , and make as Most Recently Used
     * 
     * @param address address of new op.
     * @return the overwriten block (prev).
     * if prev.address == address -> same block and only Most Recently Used update
     */
    Block insert(unsigned long int address, bool dirty);

    /**
     * @brief insert new address to cache
     * 
     * @param address 
     * @param old_dirty 
     * @param dirty  true if we write, false if we read.
     * @return old address (LRU address),
     */
    unsigned long int insert(unsigned long int address, bool &old_dirty, bool dirty);

    /**
     * @brief address is already in cache, so need to update place in Most Recently Used (become front of the list)
     * 
     * @param address 
     * @param dirty true if we write, false if we read.
     */
    bool update(unsigned long int address, bool dirty = false);
    void remove(unsigned long int address);
    int getNumberOfAccess() { return num_of_access; }
    int getNumberOfMiss() { return num_of_miss; }

    // For Debuging
    friend std::ostream& operator<< (std::ostream& out, const LevelCache& lc) { 
        for (size_t i = 0; i < lc.cache_sets.size(); i++)
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

    void L1Insert(unsigned long int block_address, bool dirty);
    void L2Insert(unsigned long int block_address, bool dirty);
    void writeAllocate(unsigned long int block_address);
    void writeNoAllocate(unsigned long int block_address);

    
public:
    MemCache(unsigned int MemCyc, unsigned int BSize, unsigned int L1Size, unsigned int L2Size, unsigned int L1Assoc, unsigned int L2Assoc, unsigned int L1Cyc, unsigned int L2Cyc, unsigned int WrAlloc);
    ~MemCache() = default;
    void read(unsigned long int address);
    void write(unsigned long int address);
    void getRates(double &L1MissRate, double &L2MissRate, double &avgAccTime);

    friend std::ostream& operator<<(std::ostream& os, const MemCache& cache){
            os << "L1" << std::endl;
            os << cache.L1 << std::endl;
            os << "L2" << std::endl;
            os << cache.L2 << std::endl;
            return os;
    }
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
void MemCache::L1Insert(unsigned long int block_address, bool dirty)
{
    bool old_dirty = false;    

    // think should use the Block insert() function - where can see if <old_block> was invalid/dirty/fine
    unsigned long int old_address = L1.insert(block_address, old_dirty, dirty);
    if (old_dirty)
        L2.update(old_address,old_dirty);
}

/**
 * @brief 
 * if the old block was dirty in the theory we need to update mem - we done nothing
 * we also need to remove the old block from L1. In the theory if it is dirty we have to update the mem.
 * 
 * @param block_address the new block to insert
 * @param dirty true if we write to the block
 */
void MemCache::L2Insert(unsigned long int block_address, bool dirty)
{
    // bool old_dirty = false;
    
    // unsigned long int old_address = L2.insert(block_address, old_dirty, dirty); 
    
    Block old_address = L2.insert(block_address,dirty); 
    if (old_address.isValid())
    {
        if (L1.isDirty(old_address.getAddress()))
            L2.insert(old_address.getAddress(),true);
        L1.remove(old_address.getAddress()); 
    }
    L2.insert(block_address,dirty); 
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
            L2Insert(block_address,false);
            num_of_mem_access++;
        }
        L1.insert(block_address, true);

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
    : allocate(WrAlloc), mem_cyc(MemCyc), l1_cyc(L1Cyc), l2_cyc(L2Cyc), L1(L1Size - BSize, L1Assoc), L2(L2Size - BSize, L2Assoc), num_of_access(0), num_of_mem_access(0), block_s(BSize) {}

/**
 * @brief simulate read request
 * 
 * @param address the address to read from
 */
void MemCache::read(unsigned long int address)
{
    num_of_access++;    // shouldnt it be inside every cache?
    unsigned long int block_address = address >> block_s;
    // bool dirty = false;
    if (L1.search(block_address))   //should refer to dirty bit?
    {
        L1.update(block_address, L1.isDirty(block_address));   
    }
    else if (L2.search(block_address))    //should refer to dirty bit?
    {
        bool old_dirty = L2.update(block_address);
        L1Insert(block_address,old_dirty); //mabe update if it was dirty
    }
    else
    {
        num_of_mem_access++;
        L2Insert(block_address,false);
        L1Insert(block_address,false);
    }
}

/**
 * @brief simulate write request
 * 
 * @param address the address to write to
 */
void MemCache::write(unsigned long int address)
{
    num_of_access++;    // shouldnt it be inside every cache?
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

    /**
    std::cout << "L1 access " << num_of_access;
    std::cout << ", L1 miss " << L1_miss;
    std::cout << ", L2 access " << L2_access;
    std::cout << ", L2 miss " << L2_miss;
    std::cout << ", mem access " << num_of_mem_access << std::endl;
    **/
   
    L1MissRate = L1_miss / num_of_access;
    L2MissRate = L2_miss / L2_access;
    double tot_time = (num_of_access * l1_cyc) + (L2_access * l2_cyc) + (num_of_mem_access * mem_cyc);
    avgAccTime = tot_time / num_of_access;
}

/*********************************************************************************************/
/*									LevelCache functions   									 */
/*********************************************************************************************/

LevelCache::LevelCache(unsigned int size, unsigned int assoc)
    : cache_sets((1<<(size-assoc)), CacheRow(assoc)), set_size(size- assoc),
      num_of_access(0), num_of_miss(0)
{
    set_mask = INT64_MAX;
    set_mask = (set_size == 0) ? 0 : set_mask>>(((8 * sizeof(int64_t)) - set_size) -1); // need check
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

Block LevelCache::insert(unsigned long int address,bool dirty)
{
    uint64_t set = address & set_mask;
    uint64_t tag = address >> set_size;

    return cache_sets[set].insert(tag, dirty);
}

unsigned long int LevelCache::insert(unsigned long int address,bool &old_dirty, bool dirty)
{
    uint64_t set = address & set_mask;
    uint64_t tag = address >> set_size;
    
    Block rem_block = cache_sets[set].insert(tag,dirty);

    old_dirty = rem_block.isDirty();
    return ((rem_block.getAddress() << set_size) + set);
}

bool LevelCache::update(unsigned long int address, bool dirty)
{
    uint64_t set = address & set_mask;
    uint64_t tag = address >> set_size;

    Block rem_block = cache_sets[set].update(tag,dirty);

    return rem_block.isDirty();
}
void LevelCache::remove(unsigned long int address)
{
    uint64_t set = address & set_mask;
    uint64_t tag = address >> set_size;

    
    cache_sets[set].remove(tag);
}
/*********************************************************************************************/
/*									CacheRow functions   									 */
/*********************************************************************************************/
CacheRow::CacheRow(int assoc) : address_list(1<<assoc) {}

bool CacheRow::search(unsigned long int tag)
{
    try
    {
        Block& b = findBlock(tag);
        (void)b;    // only for compiler not to scream, if block found -> dont throw
    }
    catch(const std::exception&)
    {
        return false;
    }

    return true;
}
Block& CacheRow::findBlock(unsigned long int tag)
{
    auto iter = std::find_if(address_list.begin(),address_list.end(), [tag](const Block& b){ return b.getAddress() == tag && b.isValid(); });
    
    if (iter == address_list.end()) throw std::runtime_error("Not Found");

    return *iter;
}
Block CacheRow::update(unsigned long new_address, bool write)
{
    Block curr_block = findBlock(new_address); 
    address_list.erase(std::find(address_list.begin(),address_list.end(),curr_block));

    curr_block.setDirty(write);
    
    address_list.push_front(curr_block);

    return curr_block;
}
Block CacheRow::insert(unsigned long tag, bool write)
{
    auto iter = std::find_if(address_list.begin(),address_list.end(), [tag](const Block& b){ return b.getAddress() == tag || !b.isValid(); });
    Block rem_block;

    if (iter != address_list.end())
    {
      rem_block = *iter;
      address_list.erase(iter);
    } 
    else
    {
        rem_block = address_list.back(); // return LRU
        address_list.pop_back();
    } 

    Block new_block(tag,write,true);
    address_list.push_front(new_block);
    
    return rem_block;
}
void CacheRow::remove(unsigned long int address)
{
    try
    {
        Block curr_block = findBlock(address); 
        address_list.erase(std::find(address_list.begin(),address_list.end(),curr_block));
        address_list.push_back(Block());
    }
    catch(const std::exception& e) { }
}
bool CacheRow::isDirty(unsigned long int tag)
{
    try
    {
        Block& b = findBlock(tag);
        return b.isDirty();
    }
    catch(const std::exception&)
    {
        return false;
    }
}

#endif