#include<iostream>
#include<cstdint>
using namespace std;

int main() {
    uint8_t zeroth_bit = 1;
    uint8_t number = 0b11000001;
    number = (number >> 1) + (zeroth_bit << 7);
    cout << static_cast<unsigned>(number) << endl;
}