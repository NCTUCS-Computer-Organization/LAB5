#include<iostream>
#include<fstream>
#include<cmath>
#include<vector>
#define u unsigned int
using namespace std;

const int W = 4;
const int K = 1024;
const int M = 1024 * K;

u A[128][128], B[128][128], C[128][128];

class cache {
  public:
    int cache_size, block_size, n_way, lines;
    int log2_cache_size, log2_block_size, log2_n_way, log2_lines;
    int pnt, hit_pnt, miss_pnt;
    int hit, miss;
    vector<vector<u>> content;

    cache(int cache_size, int block_size, int n_way) {
        this->cache_size = cache_size;
        this->block_size = block_size;
        this->n_way = n_way;
        this->lines = cache_size / block_size / n_way;
        this->log2_cache_size = log2(block_size);
        this->log2_block_size = log2(block_size);
        this->log2_n_way = log2(n_way);
        this->log2_lines = log2(lines);

        pnt = 0, hit = 0, miss = 0;
        
        content.resize(lines, vector<u>(n_way));
    }

    bool access(u addr) {
        u index = (addr >> (log2_block_size)) & (lines - 1);
        u tag = addr >> (log2_lines + log2_block_size);

        bool get = false;
        for(auto i = 0; i < n_way; i++) {
            if(content[index][i] == tag) {
                ++hit;
                pnt += hit_pnt;
                content[index].insert(content[index].begin(), content[index][i]);
                content[index].erase(content[index].begin()+i+1);
                get = true;
                break;
            }
        }

        if(!get) {
            ++miss;
            pnt += miss_pnt;
            content[index].erase(content[index].end() - 1);
            content[index].insert(content[index].begin(), tag);
            return false;
        }
        else 
            return true;
    }
};

cache cache_a(512, 32, 8), cache_b(512, 32, 8), cache_c1(128, 16, 8), cache_c2(2*K, 128, 8);
int global_clock = 0;

inline void addi(u& dst, u src1, u src2) {
    ++global_clock;
    dst = src1 + src2;
}

inline void addu(u& dst, u src1, u src2) {
    ++global_clock;
    dst = src1 + src2;
}

inline void slt(u& dst, u src1, u src2) {
    ++global_clock;
    dst = src1 < src2 ? 1 : 0;
}

inline bool beq(u arg0, u arg1) {
    ++global_clock;
    return arg0 == arg1;
}

inline void mul(u& dst, u src1, u src2) {
    ++global_clock;
    dst = src1 * src2;
}

inline void lw(u& dst, u addr) {
    global_clock += 1;
    cache_a.access(addr);
    cache_b.access(addr);
    if(!cache_c1.access(addr))
        cache_c2.access(addr);
}

inline void sw(u src, u addr) {
    global_clock += 1;
    cache_a.access(addr);
    cache_b.access(addr);
    if(!cache_c1.access(addr))
        cache_c2.access(addr);
}

int main(int argc, char** argv) {

    cache_a.hit_pnt = 1 + 2 + 1;
    cache_a.miss_pnt = (1 + 2 + 8 * (1 + 100 + 1 + 2) + 1);

    cache_b.hit_pnt = (1 + 2 + 1);
    cache_b.miss_pnt = (1 + 2 + 1 + 100 + 1 + 2 + 1);

    cache_c1.hit_pnt = 1 + 1 + 1;
    cache_c1.miss_pnt = 0;
    cache_c2.hit_pnt = (1 + 1 + 4 * (1 + 10 + 1 + 1) + 1);
    cache_c2.miss_pnt = (1 + 32 * (1 + 100 + 1 + 10) + 4 * (1 + 10 + 1 + 1) + 1 + 1);

    u reg[32] = {0};

    char* input_path = argv[1];
    char* output_path = argv[2];

    ifstream fin(input_path);
    ofstream fout(output_path);

    //matrix multiplication initialization (push parameter onto stack)
    //	$24 = A[]base, $25 = B[]base, $26 = C[]base

    fin >> hex >> reg[24] >> reg[25] >> reg[26];

    //	$21 = m, $22 = n, $23 = p
    fin >> dec >> reg[21] >> reg[22] >> reg[23];

    for(auto i = 0; i < reg[21]; i++)
        for(auto j = 0; j < reg[22]; j++)
            fin >> A[i][j];
    
    for(auto i = 0; i < reg[22]; i++)
        for(auto j = 0; j < reg[23]; j++)
            fin >> B[i][j];

    //matrix multiplication start
    //	$3 = i, $4 = j, $5 = k
    //	$1 = const 4
    reg[1] = 4;

    addi(reg[1], reg[0], 4);
    addi(reg[3], reg[0], 0);
loop_i:
    slt(reg[6], reg[3], reg[21]);
    if (beq(reg[6], reg[0])) {
        goto c_exit;
    }
    addi(reg[4], reg[0], 0);
loop_j:
    slt(reg[6], reg[4], reg[23]);
    if (beq(reg[6], reg[0])) {
        goto end_j;
    }
    addi(reg[5], reg[0], 0);
loop_k:
    slt(reg[6], reg[5], reg[22]);
    if (beq(reg[6], reg[0])) {
        goto end_k;
    }
    //main work, C[i][j] = C[i][j] + A[i][k]*B[k][j]
    mul(reg[7], reg[3], reg[23]);
    addu(reg[8], reg[7], reg[4]);
    mul(reg[8], reg[8], reg[1]);
    addu(reg[9], reg[8], reg[26]);
    //	temp1 = 4(i*p+j) + C[]base
    lw(reg[10], reg[9]);                        reg[10] = C[reg[3]][reg[4]];
    mul(reg[11], reg[3], reg[22]);
    addu(reg[12], reg[11], reg[5]);
    mul(reg[12], reg[12], reg[1]);
    addu(reg[13], reg[12], reg[24]);
    //	temp2 = 4(i*n+k) + A[]base
    lw(reg[14], reg[13]);                       reg[14] = A[reg[3]][reg[5]];
    mul(reg[15], reg[5], reg[23]);
    addu(reg[16], reg[15], reg[4]);
    mul(reg[16], reg[16], reg[1]);
    addu(reg[17], reg[16], reg[25]);
    //	temp3 = 4(k*p+j) + B[]base
    lw(reg[18], reg[17]);                       reg[18] = B[reg[5]][reg[4]];
    mul(reg[19], reg[18], reg[14]);
    addu(reg[20], reg[10], reg[19]);
    sw(reg[20], reg[9]);                        C[reg[3]][reg[4]] = reg[20];
    //	C[i][j] = C[i][j] + A[i][k]*B[k][j]
    //end work
    addi(reg[5], reg[5], 1);
    ++global_clock;
    goto loop_k;
end_k:
    addi(reg[4], reg[4], 1);
    ++global_clock;
    goto loop_j;
end_j:
    addi(reg[3], reg[3], 1);
    ++global_clock;
    goto loop_i;
c_exit:
    ++global_clock;
    for(auto i = 0; i < reg[21]; i++)
    {
        for(auto j = 0; j < reg[23]; j++)
            fout << C[i][j] <<' ';
        fout << endl;
    }

    fout << global_clock << ' ' << cache_a.pnt << ' ' << cache_b.pnt << ' ' << cache_c1.pnt + cache_c2.pnt << endl;

    // cout << cache_a.hit << ' ' << cache_a.miss << endl;
    // cout << cache_b.hit << ' ' << cache_b.miss << endl;
    // cout << cache_c1.hit << ' ' << cache_c1.miss << endl;
    // cout << cache_c2.hit << ' ' << cache_c2.miss << endl;
    fin.close();
    fout.close();

    return 0;
}