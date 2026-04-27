
#ifndef __CPU_PRED_YAGS_PRED_HH__
#define __CPU_PRED_YAGS_PRED_HH__

#include "base/sat_counter.hh"
#include "cpu/pred/bpred_unit.hh"
#include "params/YAGSBP.hh"

namespace gem5
{

namespace branch_prediction
{

/**
 * Implements a gshare branch predictor. The gshare predictor takes hash
 * of global history and the program counter to access the n-bit counter
 * to predict the branch (taken or not).This was a precursor to the bi-mode
 * branch predictor model, which handles destructive aliasing by seperating
 * taken and not taken counters.
 */

class YAGSBP : public ConditionalPredictor
{
  public:
    YAGSBP(const YAGSBPParams &params);
    bool lookup(ThreadID tid, Addr pc, void *&bp_history);
    void updateHistories(ThreadID tid, Addr pc, bool uncond, bool taken,
                         Addr target, const StaticInstPtr &inst,
                         void *&bp_history);
    void squash(ThreadID tid, void *&bp_history);
    void update(ThreadID tid, Addr pc, bool taken, void *&bp_history,
                bool squashed, const StaticInstPtr &inst, Addr target);

  private:
    void updateGlobalHistReg(ThreadID tid, bool taken);
    void uncondBranch(ThreadID tid, Addr pc, void *&bp_history);

    struct BPHistory
    {
        unsigned globalHistoryReg;
        bool finalPred;
    };

    std::vector<unsigned> globalHistoryReg;
    unsigned globalHistoryBits;
    unsigned historyRegisterMask;

    unsigned globalPredictorSize;
    unsigned globalCtrBits;
    unsigned globalHistoryMask;

    std::vector<SatCounter8> globalCtrs;
    unsigned takenThreshold;
    
    // YAGS Implementation
    
    std::vector<SatCounter8> Tcache;
    std::vector<unsigned> Tcache_tag;
    unsigned takenTCacheThreshold;
    
    std::vector<SatCounter8> NTcache;
    std::vector<unsigned> NTcache_tag;
    unsigned takenNTCacheThreshold;
};

} // namespace branch_prediction
} // namespace gem5
#endif // __CPU_PRED_YAGS_PRED_HH__
