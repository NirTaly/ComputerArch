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
    unsigned int numOfInsts;

    const int ENTRY = -1;

    bool isInstLegal(int instruction) { return instruction >= 0 && instruction < numOfInsts;}

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
    depTree[numOfInsts] = InstDep(0,-1,-1);

    //
    vector<int> regLastCng(MAXREGISTERS,numOfInsts);
    int depth, dep1, dep2;
    int max_depth = 0;
    int last_inst = numOfInsts;
    int i;
    for (i = 0; i< numOfInsts; i++)
    {
        //inst i dependecies
        dep1 = regLastCng[progTrace[i].src1Idx];
        dep2 = regLastCng[progTrace[i].src2Idx];

        //inst i depth
        depth = (depTree[dep1].getDepth() > depTree[dep2].getDepth()) ? depTree[dep1].getDepth() : depTree[dep2].getDepth();
        depth += opsLatency[progTrace[i].opcode];

        //update vectors
        depTree[numOfInsts] = InstDep(depth, dep1, dep2);
        regLastCng[progTrace[i].dstIdx] = i;
        //calc the max depth
        if (depth  > max_depth)
        {
            depth = max_depth;
            last_inst = i;
        }
    }

    //adding exit
    depTree[numOfInsts + 1] = InstDep(max_depth, i, i);

}


int Graph::getInstDepth(unsigned int theInst)
{
    if ( depTree[depTree[theInst].getDep1()].getDepth() > depTree[depTree[theInst].getDep2()].getDepth())
        return depTree[depTree[theInst].getDep1()].getDepth();
    return depTree[depTree[theInst].getDep2()].getDepth();
}
int Graph::getInstDeps(unsigned int theInst, int *src1DepInst, int *src2DepInst)
{
    src1DepInst = depTree[theInst].getDep1();
    src2DepInst = depTree[theInst].getDep2();
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


