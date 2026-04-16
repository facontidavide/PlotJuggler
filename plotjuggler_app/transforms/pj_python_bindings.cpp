#include <nanobind/nanobind.h>

namespace nb = nanobind;

static double first_fun()
{
  return 1.0;
}

NB_MODULE(pj, m)
{
  m.def("first_fun", &first_fun);
}
