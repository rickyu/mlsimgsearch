#include <cstdlib>
#include <ctime>
#include <string>
#include "string_util.h"

class ServiceRandomGenerator {
  public:
    ServiceRandomGenerator();
    ~ServiceRandomGenerator();
    void Init(int max);
    int GetRandom();
  private:
    int max_number_;
    int* candidates_;
    int cur_max_;
};

unsigned int iptoint(std::string ip);
