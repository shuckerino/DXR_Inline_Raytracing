#include <gimslib/types.hpp>
#include <iostream>

using namespace gims;
using namespace std;
int main(int /* argc*/, char /* **argv */)
{
  auto A = glm::eulerAngleZ(glm::radians(45.0f));
  cerr << to_string(A) << "\n"; 
  return 0;
}
