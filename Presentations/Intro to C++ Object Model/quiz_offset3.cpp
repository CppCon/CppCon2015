#include <iostream>
#include <cmath>
using std::cout;

struct Complex
{
  virtual ~Complex() = default;
  virtual float Abs() { return std::hypot(real, imag); }
  float real;
  float imag;
};

struct Derived : public Complex
{
  virtual ~Derived() = default;
  virtual float Abs() { return std::hypot(std::hypot(real, imag), angle); }
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
