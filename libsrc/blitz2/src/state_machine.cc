#include <boost/lexical_cast.hpp>
#include <blitz2/state_machine.h>

namespace blitz2 {
int ActionArcs::CheckAndConvertToVec(std::vector<state_id_t>* state_ids) const {
  using boost::lexical_cast;
  using namespace std;
  vector<OneArc> arcs = arcs_;
  sort(arcs.begin(), arcs.end());
  size_t count = arcs.size();
  state_ids->resize(count);
  for (size_t i=0; i<count; ++i) {
    if (arcs[i].result != static_cast<action_result_t>(i)) {
      last_error_ = "result " + lexical_cast<string>(i) + " lost";
      return -1;
    }
    (*state_ids)[i] = arcs[i].state;
  }
  return 0;
}
} // namespace blitz2.
