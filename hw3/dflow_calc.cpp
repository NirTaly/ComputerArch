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
    unsigned int depth;
    unsigned int dep1;
    unsigned int dep2;
public:
    InstDep(): depth(0), dep1(0), dep2(0) {}
    InstDep(unsigned int depth,unsigned int dep1,unsigned int dep2): depth(depth), dep1(dep1), dep2(dep2) {}
    InstDep(const InstDep& inst_dep): depth(inst_dep.depth), dep1(inst_dep.dep1), dep2(inst_dep.dep2){}
    InstDep& operator=(const InstDep& instdep) = default;

    ~InstDep() = default;
    void setDepth(unsigned int depth) {this->depth = depth;}
    void setDep1(unsigned int dep1)  {this->dep1 = dep1;}
    void setDep2(unsigned int dep2) {this->dep2 = dep2;}
    unsigned int getDepth() {return depth;}
    unsigned int getDep1()  {return dep1;}
    unsigned int getDep2() {return dep2;}
};



class Graph
{
private:
    vector<InstDep> depTree;
    unsigned int numOfInsts;

    const int ENTRY = -1;

    bool isInstLegal(unsigned int instruction) { return instruction >= 0 && instruction < numOfInsts;}

public:
    Graph(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts);
    ~Graph() = default;
    int getInstDepth(unsigned int theInst);
    int getInstDeps(unsigned int theInst, int *src1DepInst, int *src2DepInst);
    int getProgDepth();
};
Graph::Graph(const unsigned int opsLatency[], const InstInfo progTrace[], unsigned int numOfInsts): depTree(numOfInsts + 2), numOfInsts(numOfInsts)
{


    // adding the entery
    depTree[numOfInsts] = InstDep(0,numOfInsts,numOfInsts);

    //
    vector<unsigned int> regLastCng(MAXREGISTERS,numOfInsts);
    unsigned int depth, dep1, dep2;
    unsigned int max_depth = 0;
    unsigned int last_inst = numOfInsts;
    unsigned int i;
    for (i = 0; i < numOfInsts; i++)
    {
        //inst i dependecies
        dep1 = regLastCng[progTrace[i].src1Idx];
        dep2 = regLastCng[progTrace[i].src2Idx];

        //inst i depth
        depth = (depTree[dep1].getDepth() > depTree[dep2].getDepth()) ? depTree[dep1].getDepth() : depTree[dep2].getDepth();
        depth += opsLatency[progTrace[i].opcode];

        //update vectors
        depTree[i] = InstDep(depth, dep1, dep2);
        regLastCng[progTrace[i].dstIdx] = i;
        //calc the max depth
        if (depth  > max_depth)
        {
            depth = max_depth;
            last_inst = i;
        }
    }

    //adding exit
    depTree[numOfInsts + 1] = InstDep(max_depth, last_inst, last_inst);

}


int Graph::getInstDepth(unsigned int theInst)
{
    if (!isInstLegal(theInst))
        return -1;
    if ( depTree[depTree[theInst].getDep1()].getDepth() > depTree[depTree[theInst].getDep2()].getDepth())
        return depTree[depTree[theInst].getDep1()].getDepth();
    if ( depTree[depTree[theInst].getDep2()].getDepth()  == numOfInsts)
        return -1;
    return depTree[depTree[theInst].getDep2()].getDepth();
}
int Graph::getInstDeps(unsigned int theInst, int *src1DepInst, int *src2DepInst)
{
    if (!isInstLegal(theInst))
        return -1;
    *src1DepInst = (depTree[theInst].getDep1() != numOfInsts) ? depTree[theInst].getDep1() : -1;
    *src2DepInst = (depTree[theInst].getDep2() != numOfInsts) ? depTree[theInst].getDep2() : -1;
    return depTree[theInst].getDepth();
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


