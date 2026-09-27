#include <cstdio>

// TODO(ayush, milestone 4): the replay CLI.
//   replay --input data/sample.csv --symbol XYZ --log out/trades.log [--bench]
// Steps: parse args (no library needed), stream the file line by line through
// the parser into ReplayEngine, write the trade/audit log, and print the
// determinism hash (e.g. a simple FNV-1a over the log bytes) so CI can diff
// two runs. Keep --bench mode separate from normal replay.
int main(int /*argc*/, char** /*argv*/) {
    std::puts("replay: not implemented yet (milestone 4)");
    return 0;
}
