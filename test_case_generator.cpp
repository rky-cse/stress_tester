
#include <bits/stdc++.h>
using namespace std;
typedef long long ll;
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>
#include <random>
// Global random engine seeded with current time
mt19937_64 gen(chrono::system_clock::now().time_since_epoch().count());

// 1. Function for generating a random number in the range [min, max] (using long long)
long long randomNumberGeneration(long long min, long long max) {
    uniform_int_distribution<long long> dis(min, max);
    return dis(gen);
}

// 2. Function for generating a random lowercase string of given size
string lowercaseStringGeneration(int size) {
    string result;
    for (int i = 0; i < size; ++i) {
        result += static_cast<char>('a' + randomNumberGeneration(0, 25));
    }
    return result;
}
     // Function to generate random test cases with constraints
    void generate_test_case() {///////////////////////////////update it
        int n=randomNumberGeneration(1,10);
        int k=randomNumberGeneration(1,1000000000);
        cout<<1<<"\n";
    	cout<<n<<" "<<k<<"\n";
        for(int i=0;i<n;i++){
            cout<<randomNumberGeneration(1,1000000000)<<' ';
        }


       // fout<<lowercaseStringGeneration(randomNumberGeneration(1,100));
       
        cout << endl;
    }
    int main(){
        //freopen("stress_tester_input_test_case.txt","w",stdout);
        generate_test_case();

        return 0;
    }
