// 0711282 邱頎霖
// 0716077 柯柏揚
#include <bits/stdc++.h>
#include <fstream>
using namespace std;
#define associatively 8

static int matrix_A[2048][2048];
static int matrix_B[2048][2048];
static int matrix_C[2048][2048];


struct cache_content{
	unsigned int tag;
};

class cache{
    public:
        bool flag=false;
        int offset_bit;
        int index_bit;
        int tag_bit;
        int penalty=0;
        int hit_cnt=0; // L1
        int miss_cnt=0;
        int hit_penalty;
        int miss_penalty;
        long long cache_blocks;
        vector<list<cache_content>> cache_data;
        cache(int cache_size,int block_size);
};

cache::cache(int cache_size,int block_size){
    this->offset_bit = log2(block_size);
    this->cache_blocks = cache_size/(block_size*associatively);
    this->index_bit = log2(cache_blocks);
    this->tag_bit = 32-(this->index_bit)-(this->offset_bit);
    this->cache_data.resize(cache_blocks);
    //cout<<"cache_size"<<cache_size<<" "<<"block_size"<<block_size<<" "<<cache_blocks<<endl;
}

cache cache_a(pow(2,9),32);
cache cache_b(pow(2,9),32);
cache cache_c_l1(pow(2,7),16);
cache cache_c_l2(pow(2,12),128);
const int K = 1024;

double log2(double n){  
    return log(n) / log(double(2));
}

void simulate(cache &tmp,long long int addr){           
    unsigned int tag, index, x;

    int offset_bit = tmp.offset_bit;
    int index_bit = tmp.index_bit;
    long long int line = pow(2,index_bit);

    index = (addr >> offset_bit) & (line - 1);
    tag = addr >> (index_bit + offset_bit);

    bool flag=false;
    for(auto it= tmp.cache_data[index].begin() ;it!=tmp.cache_data[index].end() ;it++){
        if(it->tag == tag){
            tmp.penalty+=tmp.hit_penalty;
            tmp.hit_cnt++;
            tmp.cache_data[index].insert(tmp.cache_data[index].begin(),*it);
            it=tmp.cache_data[index].erase(it);
            flag=true;
            break;
        }
    }
    if(!flag){
        tmp.miss_cnt++;
        if(tmp.flag){
            simulate(cache_c_l2,addr);
        }
        else{
            tmp.penalty+=tmp.miss_penalty;
        }
        
        cache_content temp;
        temp.tag=tag;
        reverse(tmp.cache_data[index].begin(),tmp.cache_data[index].end());
        tmp.cache_data[index].push_back(temp);
        reverse(tmp.cache_data[index].begin(),tmp.cache_data[index].end());
        if(tmp.cache_data[index].size()>8) tmp.cache_data[index].pop_back();
    }
}

void mul(long long &tmp1,long long &tmp2,long long &tmp3){
    tmp1=tmp2*tmp3;
    return ;
}

void addu(long long &tmp1,long long &tmp2,long long &tmp3){
    tmp1=tmp2+tmp3;
    return ;
}

void lw(long long &tmp1,long long int tmp2,long long int &tmp3
        ,int i,int j,int k
        ,int index){
    
    simulate(cache_a,tmp3);
    simulate(cache_b,tmp3);
    simulate(cache_c_l1,tmp3);
    if(index==0){
        tmp1=matrix_A[i][k];
    }
    else if(index==1){
        tmp1=matrix_B[k][j];
    }
    else if(index==2){
        tmp1=matrix_C[i][j];
    }
    return ;
}


void sw(long long tmp1,long long int &addr,long long i,long long j){
    simulate(cache_a,addr);
    simulate(cache_b,addr);
    simulate(cache_c_l1,addr);
    matrix_C[i][j]=tmp1;
    return ;
}

int main(int argc, char *argv[]) {
    cin.tie(0), ios::sync_with_stdio(0);
    
    cache_a.hit_penalty=1+2+1;
    cache_a.miss_penalty=(1 + 2 + 8 * (1 + 100 + 1 + 2) + 1);

    cache_b.hit_penalty=(1+2+1);
    cache_b.miss_penalty=(1 + 2 + 1 + 100 + 1 + 2 + 1);

    cache_c_l1.hit_penalty=1 + 1 + 1;
    cache_c_l1.flag=true;
    cache_c_l2.hit_penalty=(1 + 1 + 4 * (1 + 10 + 1 + 1) + 1);
    cache_c_l2.miss_penalty=(1 + 32 * (1 + 100 + 1 + 10) + 4 * (1 + 10 + 1 + 1) + 1 + 1);

    long long clock_cycle=0;
    long long reg[32]={0};
    int n,m,p;

    /*cout<<cache_a.line<<" "<<cache_a.index_bit<<cache_a.offset_bit<<cache_a.tag_bit<<endl;
    cout<<cache_b.line<<endl;
    cout<<cache_c_l1.line<<endl;
    cout<<cache_c_l2.line<<endl;*/

    string input_file=argv[1];
    string output_file=argv[2];

    memset(matrix_A,0,sizeof(matrix_A));
    memset(matrix_B,0,sizeof(matrix_B));
    memset(matrix_C,0,sizeof(matrix_C));


    fstream file;
    file.open(input_file,ios::in);

    file >> hex >> reg[24] >> reg[25] >> reg[26];
    // $24 = A[]base, $25 = B[]base, $26 = C[]base
    
    file >> dec >> reg[21] >> reg[22] >> reg[23]; 
    // $21 = m, $22 = n, $23 = p
  
    m=reg[21];
    n=reg[22];
    p=reg[23];

    //cout<<m<<n<<p<<endl;
    reg[1]=4 ;  // $1 = const 4
    ++clock_cycle;

    for(int i=0;i<m;i++){
        for(int j=0;j<n;j++){
            file>>matrix_A[i][j];
        }
    }
    
    for(int i=0;i<n;i++){
        for(int j=0;j<p;j++){
            file>>matrix_B[i][j];
        }
    }
    file.close();

    // matrix    multiplication
    // addi $3, $0, 0

    ++clock_cycle; //addi $3, $0, 0

    for(int i=0;i<m;++i){
        clock_cycle+=3; 
        // slt $6, $3, $21  beq $6, $0, exit
        // addi $4, $0, 0

        for(int j=0;j<p;++j){
            //addi $5, $0, 0
            clock_cycle+=3;
            // slt $6, $4, $23  beq $6, $0, end_j

            for(int k=0;k<n;++k){

                reg[3]=i;
                reg[4]=j;
                reg[5]=k;
                mul(reg[7],reg[3],reg[23]);
                addu(reg[8],reg[7],reg[4]);
                mul(reg[8],reg[8],reg[1]);
                addu(reg[9],reg[8],reg[26]);
                lw(reg[10],0,reg[9],i,j,k,2); // c=2
                //	temp1 = 4(i*p+j) + C[]base
                
                mul(reg[11],reg[3],reg[22]);
                addu(reg[12],reg[11],reg[5]);
                mul(reg[12],reg[12],reg[1]);
                addu(reg[13],reg[12],reg[24]);
                lw(reg[14],0,reg[13],i,j,k,0); // a=0
                //	temp2 = 4(i*n+k) + A[]base

                mul(reg[15],reg[5],reg[23]);
                addu(reg[16],reg[15],reg[4]);
                mul(reg[16],reg[16],reg[1]);
                addu(reg[17],reg[16],reg[25]);
                lw(reg[18],0,reg[17],i,j,k,1); // b=1
                //	temp3 = 4(k*p+j) + B[]base

                //if(i==0 && j==0) cout<<reg[18]<<" "<<reg[14]<<endl;
                mul(reg[19],reg[18],reg[14]);
                //if(i==0 && j==0) cout<<"Reg: "<<dec<<reg[19]<<" "<<reg[10]<<endl;
                addu(reg[20],reg[10],reg[19]);
                sw(reg[20],reg[9],i,j);
                //	C[i][j] = C[i][j] + A[i][k]*B[k][j]

                clock_cycle+=22;
            }
            clock_cycle+=2;
            clock_cycle+=2;
        }
        clock_cycle+=2;
        clock_cycle+=2;
    }
    clock_cycle+=2;
    ++clock_cycle; //exit

    
    
    file.open(output_file,ios::out);

    for(int i=0;i<m;i++){
        for(int j=0;j<p;j++){
            file<<dec<<matrix_C[i][j]<<" ";
        }
        file<<endl;
    }

    file<<clock_cycle<<" "<<cache_a.penalty<<" "<<cache_b.penalty<<" "<<cache_c_l1.penalty+cache_c_l2.penalty<<endl;
    /*cout<<cache_a.hit_cnt<<" "<<cache_a.miss_cnt<<endl;
    cout<<cache_b.hit_cnt<<" "<<cache_b.miss_cnt<<endl;
    cout<<cache_c_l1.hit_cnt<<" "<<cache_c_l1.miss_cnt<<endl;
    cout<<cache_c_l2.hit_cnt<<" "<<cache_c_l2.miss_cnt<<endl;*/
    file.close();
    //1+8*(1+100+1+2)+2+1=1+8*104+3=4+832=836 

    return 0;
}
