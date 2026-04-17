
#include "cpu/pred/yags.hh"

#include "base/bitfield.hh"
#include "base/intmath.hh"

namespace gem5
{

namespace branch_prediction
{

YAGSBP::YAGSBP(const YAGSBPParams &params)
    : BPredUnit(params),
      globalHistoryReg(params.numThreads, 0),
      globalHistoryBits(ceilLog2(params.global_predictor_size)),
      globalPredictorSize(params.global_predictor_size),
      globalCtrBits(params.global_counter_bits),
      globalCtrs(globalPredictorSize, SatCounter8(globalCtrBits))
{

    if (!isPowerOf2(globalPredictorSize)) {
        fatal("Invalid global history predictor size.\n");
    }
    historyRegisterMask = mask(globalHistoryBits);
    globalHistoryMask = globalPredictorSize - 1;
    takenThreshold = (1ULL << (globalCtrBits - 1)) - 1;
    takenTCacheThreshold = (1ULL << (1)) - 1;
    takenNTCacheThreshold = (1ULL << (1)) - 1;
}

/*
 * For an unconditional branch we set its history such that
 * everything is set to taken. I.e., its choice predictor
 * chooses the taken array and the taken array predicts taken.
 */
void
YAGSBP::uncondBranch(ThreadID tid, Addr pc, void *&bp_history)
{
    BPHistory *history = new BPHistory;
    history->globalHistoryReg = globalHistoryReg[tid];
    history->finalPred = true;
    bp_history = static_cast<void *>(history);
}

void
YAGSBP::updateHistories(ThreadID tid, Addr pc, bool uncond, bool taken,
                          Addr target, const StaticInstPtr &inst,
                          void *&bp_history)
{
    assert(uncond || bp_history);
    if (uncond) {
        uncondBranch(tid, pc, bp_history);
    }
    updateGlobalHistReg(tid, taken);
}

void
YAGSBP::squash(ThreadID tid, void *&bp_history)
{
    BPHistory *history = static_cast<BPHistory *>(bp_history);
    globalHistoryReg[tid] = history->globalHistoryReg;

    delete history;
    bp_history = nullptr;
}

/*
 * Here we lookup the actual branch prediction. A hash of
 * the global history register and a branch's PC is used to
 * index into the counter, which both present a prediction.
 */
bool
YAGSBP::lookup(ThreadID tid, Addr branchAddr, void *&bp_history)
{
    unsigned globalHistoryIdx =
        (((branchAddr >> instShiftAmt) ^ globalHistoryReg[tid]) &
         globalHistoryMask);
    unsigned patternHistoryIdx =( branchAddr >> instShiftAmt);
    assert(globalHistoryIdx < globalPredictorSize);
    //assert(patternHistoryIdx < globalPredictorSize);
    
    bool PHTprediction = globalCtrs[patternHistoryIdx] > takenThreshold;
    bool NTCacheprediction = NTcache[globalHistoryIdx] > takenNTCacheThreshold;
    bool TCacheprediction = Tcache[globalHistoryIdx] > takenTCacheThreshold;
    bool TCacheHit = Tcache_tag[globalHistoryIdx] == patternHistoryIdx;
    bool NTCacheHit = NTcache_tag[globalHistoryIdx] == patternHistoryIdx;
    
    bool final_prediction = PHTprediction ? (~NTCacheHit & PHTprediction) | (NTCacheHit &  NTCacheprediction): (~TCacheHit & PHTprediction) | (TCacheHit &  TCacheprediction);
    
    BPHistory *history = new BPHistory;
    history->globalHistoryReg = globalHistoryReg[tid];
    history->finalPred = final_prediction;
    bp_history = static_cast<void *>(history);

    return final_prediction;
}

/* Updates the counter values based on the actual branch
 * direction.
 */
void
YAGSBP::update(ThreadID tid, Addr branchAddr, bool taken, void *&bp_history,
                 bool squashed, const StaticInstPtr &inst, Addr target)
{
    assert(bp_history);

    BPHistory *history = static_cast<BPHistory *>(bp_history);
    
    
    
    // We do not update the counters speculatively on a squash.
    // We just restore the global history register.
    if (squashed) {
        globalHistoryReg[tid] = (history->globalHistoryReg << 1) | taken;
        return;
    }

    unsigned globalHistoryIdx =
        (((branchAddr >> instShiftAmt) ^ history->globalHistoryReg) &
         globalHistoryMask);
    
    assert(globalHistoryIdx < globalPredictorSize);
    unsigned patternHistoryIdx =( branchAddr >> instShiftAmt);
    bool PHTprediction = globalCtrs[patternHistoryIdx] > takenThreshold;
    bool NTCacheprediction = NTcache[globalHistoryIdx] > takenNTCacheThreshold;
    bool TCacheprediction = Tcache[globalHistoryIdx] > takenTCacheThreshold;
    bool TCacheHit = Tcache_tag[globalHistoryIdx] == patternHistoryIdx;
    bool NTCacheHit = NTcache_tag[globalHistoryIdx] == patternHistoryIdx;
    
    if(NTCacheHit || (~taken & PHTprediction)){
        NTcache[globalHistoryIdx]++;
    }else if (NTCacheHit || (taken & PHTprediction)){
        NTcache[globalHistoryIdx]--;
    } else if (~NTCacheHit & PHTprediction){
        NTcache_tag[globalHistoryIdx] = patternHistoryIdx;
    }
    
    if(TCacheHit || (~taken & PHTprediction)){
        Tcache[globalHistoryIdx]++;
    }else if (TCacheHit || (taken & PHTprediction)){
        Tcache[globalHistoryIdx]--;
    } else if (~TCacheHit & PHTprediction){
        Tcache_tag[globalHistoryIdx] = patternHistoryIdx;
    }
    
    if (taken) {
        globalCtrs[patternHistoryIdx]++;
    } else {
        globalCtrs[patternHistoryIdx]--;
    }
    delete history;
    bp_history = nullptr;
}

void
YAGSBP::updateGlobalHistReg(ThreadID tid, bool taken)
{
    globalHistoryReg[tid] = taken ? (globalHistoryReg[tid] << 1) | 1
                                  : (globalHistoryReg[tid] << 1);
    globalHistoryReg[tid] &= historyRegisterMask;
}

} // namespace branch_prediction
} // namespace gem5
