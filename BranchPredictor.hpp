#ifndef __BRANCH_PREDICTOR_HPP__
#define __BRANCH_PREDICTOR_HPP__

#include <vector>
#include <bitset>
#include<cassert>
#include<random>

using namespace std;

struct BranchPredictor {
    virtual bool predict(uint32_t pc) = 0;
    virtual void update(uint32_t pc, bool taken) = 0;
};

struct SaturatingBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> table;

    SaturatingBranchPredictor(int value) : table(1 << 14, value) {}

    bool predict(uint32_t pc) {
        // your code here
        uint32_t lsb14=(pc & (uint32_t)((1<<14)-1));
        if(table[lsb14]==bitset<2>(2) || table[lsb14]==bitset<2>(3)) return true;
        else return false;
    }

    void update(uint32_t pc, bool taken) {
        // your code here
        uint32_t lsb14=(pc & (uint32_t)((1<<14)-1));
        if(taken)
        {
            if(table[lsb14]==bitset<2>(0)) table[lsb14]=bitset<2>(1);
            else if(table[lsb14]==bitset<2>(1)) table[lsb14]=bitset<2>(2);
            else if(table[lsb14]==bitset<2>(2)) table[lsb14]=bitset<2>(3);
        }
        else
        {
            if(table[lsb14]==bitset<2>(1)) table[lsb14]=bitset<2>(0);
            else if(table[lsb14]==bitset<2>(2)) table[lsb14]=bitset<2>(1);
            else if(table[lsb14]==bitset<2>(3)) table[lsb14]=bitset<2>(2);
        }
    }
};

struct BHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    BHRBranchPredictor(int value) : bhrTable(1 << 2, value), bhr(value) {}

    bool predict(uint32_t pc) {
        // your code here
        int val=(int)(bhr.to_ulong());
        if(bhrTable[val]==bitset<2>(2) || bhrTable[val]==bitset<2>(3)) return true;
        else return false;
    }

    void update(uint32_t pc, bool taken) {
        // your code here
        int val=(int)(bhr.to_ulong());
        if(taken)
        {
            if(bhrTable[val]==bitset<2>(0)) bhrTable[val]=bitset<2>(1);
            else if(bhrTable[val]==bitset<2>(1)) bhrTable[val]=bitset<2>(2);
            else if(bhrTable[val]==bitset<2>(2)) bhrTable[val]=bitset<2>(3);
        }
        else
        {
            if(bhrTable[val]==bitset<2>(1)) bhrTable[val]=bitset<2>(0);
            else if(bhrTable[val]==bitset<2>(2)) bhrTable[val]=bitset<2>(1);
            else if(bhrTable[val]==bitset<2>(3)) bhrTable[val]=bitset<2>(2);
        }
        val=val%2;
        val=val*2;
        if(taken) val+=1;
        bhr=bitset<2>(val);
    }
};

struct SaturatingBHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    std::vector<std::bitset<2>> table;
    std::vector<std::bitset<2>> combination;
    SaturatingBHRBranchPredictor(int value, int size) : bhrTable(1 << 2, value), bhr(value), table(1 << 14, value), combination(size, value) {
        assert(size <= (1 << 16));
    }

    bool predict(uint32_t pc) {
        // your code here
        int val=(int)(bhr.to_ulong());
        uint32_t lsb14=(pc & (uint32_t)((1<<14)-1));

        if((bhrTable[val]==bitset<2>(3)) && (table[lsb14]==bitset<2>(3))) return true;
        else if((bhrTable[val]==bitset<2>(3)) && (table[lsb14]==bitset<2>(2))) return true;
        else if((bhrTable[val]==bitset<2>(2)) && (table[lsb14]==bitset<2>(3))) return true;
        else if((bhrTable[val]==bitset<2>(2)) && (table[lsb14]==bitset<2>(2))) return true;

        if(((bhrTable[val]==bitset<2>(0))) && (table[lsb14]==bitset<2>(0))) return false;
        else if((bhrTable[val]==bitset<2>(0)) && (table[lsb14]==bitset<2>(1))) return false;
        else if((bhrTable[val]==bitset<2>(1)) && (table[lsb14]==bitset<2>(0))) return false;
        else if((bhrTable[val]==bitset<2>(0)) && (table[lsb14]==bitset<2>(0))) return false;

        else
        {
            int rnd=rand()%2;
            if(rnd) return true;
            else return false;
        }
    }

    void update(uint32_t pc, bool taken) {
        // your code here

        uint32_t lsb14=(pc & (uint32_t)((1<<14)-1));

        if(taken)
        {
            if(table[lsb14]==bitset<2>(0)) table[lsb14]=bitset<2>(1);
            else if(table[lsb14]==bitset<2>(1)) table[lsb14]=bitset<2>(2);
            else if(table[lsb14]==bitset<2>(2)) table[lsb14]=bitset<2>(3);
        }
        else
        {
            if(table[lsb14]==bitset<2>(1)) table[lsb14]=bitset<2>(0);
            else if(table[lsb14]==bitset<2>(2)) table[lsb14]=bitset<2>(1);
            else if(table[lsb14]==bitset<2>(3)) table[lsb14]=bitset<2>(2);
        }

        int val=(int)(bhr.to_ulong());

        if(taken)
        {
            if(bhrTable[val]==bitset<2>(0)) bhrTable[val]=bitset<2>(1);
            else if(bhrTable[val]==bitset<2>(1)) bhrTable[val]=bitset<2>(2);
            else if(bhrTable[val]==bitset<2>(2)) bhrTable[val]=bitset<2>(3);
        }
        else
        {
            if(bhrTable[val]==bitset<2>(1)) bhrTable[val]=bitset<2>(0);
            else if(bhrTable[val]==bitset<2>(2)) bhrTable[val]=bitset<2>(1);
            else if(bhrTable[val]==bitset<2>(3)) bhrTable[val]=bitset<2>(2);
        }
        val=val%2;
        val=val*2;
        if(taken) val+=1;
        bhr=bitset<2>(val);
    }
};

#endif