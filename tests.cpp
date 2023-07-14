#include<iostream>
#include<cstdint>
using namespace std;

int main() {
    uint8_t f = 0b00000000;
    f |= (1 << 7);
    cout << static_cast<unsigned>(f) << endl;
}