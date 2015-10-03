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
  cout<<"sizeof(float): "<<sizeof(float)<<"\n";
  cout<<"sizeof(void*): "<<sizeof(void*)<<"\n";
  cout<<"sizeof(Complex): "<<sizeof(Complex)<<"\n";
  cout<<"sizeof(Derived): "<<sizeof(Derived)<<"\n";
}
