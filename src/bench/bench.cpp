#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "book/compact_book.hpp"
#include "book/order_book.hpp"

namespace {
using namespace lob;
using Clock = std::chrono::steady_clock;
constexpr int repetitions = 5;
constexpr int cycles = 10000;
using Samples = std::vector<std::uint64_t>;
volatile std::size_t observed = 0; // keep resulting state observable
Order make(OrderId id, Side side, Price price, Quantity qty = 20,
           OrderType type = OrderType::Limit) {
    Order o{}; o.id=id; o.side=side; o.price=price; o.quantity=qty; o.type=type;
    return o;
}

template <class Book>
void oneRun(const std::string& name) {
    for (int run=0; run<repetitions; ++run) {
        Book book{};
        Samples add, cancel, modify, match;
        add.reserve(cycles); cancel.reserve(cycles); modify.reserve(cycles); match.reserve(cycles);
        OrderId id=1;
        // Warm the book, caches, and timing path outside measured events.
        for (int i=0;i<2000;++i) {
            auto o=make(id++, (i%2)?Side::Buy:Side::Sell,
                        (i%2)?9800-i%30:10200+i%30);
            if (!book.addOrder(o)) throw "prefill rejected";
        }
        for (int i=0;i<cycles;++i) {
            // Keep the operation mix stable and bounded. We measure the event
            // call only, not trace creation, output, CSV I/O or sorting.
            const auto created=id++;
            auto o=make(created,Side::Buy,9700+i%17);
            auto start=Clock::now();
            bool ok=book.addOrder(o);
            auto end=Clock::now();
            if (!ok) throw "add rejected";
            add.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
            start=Clock::now(); ok=book.cancelOrder(created); end=Clock::now();
            if (!ok) throw "cancel rejected";
            cancel.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
            // Modify one standing sell order repeatedly. Replacement loses
            // priority. Benchmark calls it only on a known live ID.
            const OrderId target=1;
            start=Clock::now(); ok=book.replaceOrder(target,10205+i%7,20); end=Clock::now();
            if (!ok) throw "modify rejected";
            modify.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
            auto offer=make(id++,Side::Sell,9000,1);
            if (!book.addOrder(offer)) throw "offer rejected";
            auto buy=make(id++,Side::Buy,0,1,OrderType::Market);
            start=Clock::now(); ok=book.addOrder(buy); end=Clock::now();
            if (!ok) throw "market rejected";
            match.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end-start).count());
        }
        observed = observed + book.restingOrderCount();
        if (!book.checkInvariants()) throw "invariant failed";
        auto report=[&](const char* operation, Samples& samples) {
            std::sort(samples.begin(),samples.end());
            auto at=[&](double p){return samples[static_cast<std::size_t>(std::ceil(samples.size()*p))-1];};
            std::uint64_t sum=0;for(auto s:samples) sum+=s;
            std::cout<<name<<','<<run+1<<','<<operation<<','<<samples.size()<<','
                     <<at(.5)<<','<<at(.99)<<','<<at(.999)<<','
                     <<std::fixed<<std::setprecision(1)
                     <<(1e9*static_cast<double>(samples.size())/static_cast<double>(sum))<<'\n';
        };
        report("add",add); report("cancel",cancel); report("modify",modify); report("match",match);
    }
}
} // namespace
int main() {
    try {
        std::cout<<"implementation,run,operation,n,p50_ns,p99_ns,p999_ns,events_per_s_call_only\n";
        oneRun<OrderBook>("baseline");
        oneRun<CompactBook>("compact");
    } catch (const char* reason) {
        std::cerr<<"Benchmark aborted: "<<reason<<'\n';return 1;
    }
}
