#include <iostream>
using std::cout;

struct Complex
{
  float real;
  float imag;
};

struct Derived : public Complex
{
  float angle;
};

int main()
{
  Derived d;
  cout<<"address of d: "<<(&d)<<"\n";
  cout<<"address of d.real: "<<(&d.real)<<"\n";
  cout<<"address of d.imag: "<<(&d.imag)<<"\n";
  cout<<"address of d.angle: "<<(&d.angle)<<"\n";
}