#include <iostream>
using std::cout;

struct Erdos
{
  void whoAmI() { cout<<"I am Erdos\n"; }
  virtual void whoAmIReally() { cout<<"I really am Erdos\n"; }
};

struct Fermat : public Erdos
{
  void whoAmI() { cout<<"I am Fermat\n"; }
  virtual void whoAmIReally() { cout<<"I really am Fermat\n"; }
};

int main()
{
  using std::unique_ptr;
  using std::make_unique;

  unique_ptr<Erdos> e = make_unique<Erdos>();
  e->whoAmI();
  e->whoAmIReally();

  unique_ptr<Fermat> f = make_unique<Fermat>();
  f->whoAmI();
  f->whoAmIReally();
}
