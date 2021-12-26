/* 046267 Computer Architecture - Winter 20/21 - HW #3               */
/* Implementation (skeleton)  for the dataflow statistics calculator */
#include <iostream>
#include <exception>
#include <algorithm>
#include <iterator>
#include <vector>

#include "dflow_calc.h"

using std::vector;

const int MAXREGISTERS = 32;

class InstDep
{
private:
    int depth;
    int dep1;
    int dep2;
public:
    InstDep(): depth(0), dep1(0), dep2(0) {}
    InstDep(int depth,int dep1,int dep2): depth(depth), dep1(dep1), dep2(dep2) {}
    InstDep& operator=(const InstDep& instdep) = default();
    ~InstDep() = default;
    int setDepth(int depth) {this->depth = depth;}
    int setDep1(int dep1)  {this->dep1 = dep1;}
    int setDep2(int dep2) {this->dep2 = dep2;}
    int getDepth() {return depth;}
    int getDep1()  {return dep1;}
    int getDep2() {return dep2;}
};



class Graph
{
private:
    vector<InstDep> depTree;
    unsigned int opsLatency[MAX_OPS];
    unsigned int numOfInsts;

    const int ENTRY = -1;

    bool isInstLegal(int instruction) { return instruction >= 0 && instruction < numOfInsts;}
public:
    Graph(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts);
    ~Graph();
    int getInstDepth(unsigned int theInst);
    int getInstDeps(unsigned int theInst, int *src1DepInst, int *src2DepInst);
    int getProgDepth();
};
Graph:: Graph(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts): depTree(numOfInsts + 2), opsLatency(), numOfInsts(numOfInsts), 
{
    //coping opLatency
    for(int i =0; i< MAX_OPS; i++)
    {
        this->opsLatency[i] = opsLatency[i];
    }

    // adding the entery
    depTree[numOfInsts] = InstDep(0,-1,-1);

    //
    vector<int> regLastCng(MAXREGISTERS,numOfInsts);
    int depth, dep1, dep2;
    for (int i = 0; i< numOfInsts; i++)
    {
        dep1 = regLastCng[progTrace[i].src1Idx];
        dep2 = regLastCng[progTrace[i].src2Idx];
        depth = (depTree[dep1].getDepth() > depTree[dep2].getDepth()) ? depTree[dep1].getDepth() : depTree[dep2].getDepth();
        depth += opsLatency[progTrace[i].opcode];
        depTree[numOfInsts] = InstDep(depth, dep1, dep2);
        regLastCng[progTrace[i].dstIdx] = i;
    }
}
Graph:: ~Graph()
{
}

int Graph::getInstDepth(unsigned int theInst)
{
    if (!isInstLegal(theInst)) return -1;

    if (ENTRY == depTree[theInst].getOp()) 
        return opsLatency[depTree[theInst].getOp()];
    
    int dep1 = depTree[theInst].getDep1();
    int dep2 = depTree[theInst].getDep2();

    // need to check if dep# isnt valid?

    int depth1 = getInstDepth(dep1);
    int depth2 = getInstDepth(dep2);

    return std::max(depth1,depth2) + opsLatency[depTree[theInst].getOp()];
}
int Graph::getInstDeps(unsigned int theInst, int *src1DepInst, int *src2DepInst)
{
    
}
int Graph::getProgDepth()
{
    return getInstDepth(numOfInsts + 1);
}


/*********************************************************************************************************************/
/*********************************************************************************************************************/

ProgCtx analyzeProg(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts) {
    try{
        Graph* graph = new Graph(opsLatency, progTrace, numOfInsts);
        return graph;
    }catch(const std::exception&) {}
    
    return PROG_CTX_NULL;
}

void freeProgCtx(ProgCtx ctx) {
    Graph* graph = static_cast<Graph*>(ctx);

    delete graph;
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {
    Graph* graph = static_cast<Graph*>(ctx);

    return graph->getInstDepth(theInst);
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {
    Graph* graph = static_cast<Graph*>(ctx);

    return graph->getInstDeps(theInst, src1DepInst, src2DepInst);
}

int getProgDepth(ProgCtx ctx) {
    Graph* graph = static_cast<Graph*>(ctx);

    return graph->getProgDepth();
}


