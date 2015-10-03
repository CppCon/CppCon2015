#include <iostream>
using std::cout;

struct Complex
{
  float real;
  float imag;
};

int main()
{
  Complex c;
  cout<<"address of c: "<<&c<<"\n";
  cout<<"address of c.real: "<<&c.real<<"\n";
  cout<<"address of c.imag: "<<&c.imag<<"\n";
}