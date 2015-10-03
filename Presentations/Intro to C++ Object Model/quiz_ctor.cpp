#include <iostream>
using std::cout;

struct Erdos
{
  Erdos() { whoAmIReally(); }
  virtual void whoAmIReally() { cout<<"I really am Erdos\n"; }
};

struct Fermat : public Erdos
{
  virtual void whoAmIReally() { cout<<"I really am Fermat\n"; }
};

int main()
{
  Erdos e;
  Fermat f;
}
