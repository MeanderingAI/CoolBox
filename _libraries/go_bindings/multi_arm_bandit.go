package coolboxgo

/*
#cgo CXXFLAGS: -std=c++17
#cgo CPPFLAGS: -I${SRCDIR} -I${SRCDIR}/abi
#cgo darwin LDFLAGS: -lc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#cgo linux LDFLAGS: -lstdc++ -L${SRCDIR}/cbridge/build -lcoolboxbridge
#include <stdlib.h>
#include "abi/multi_arm_bandit.h"
*/
import "C"

import "fmt"

func NewBanditArm(trueRewardProb float64) (*BanditArm, error) {
	var cErr *C.char
	handle := C.coolbox_create_bandit_arm(C.double(trueRewardProb), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("bandit arm handle is nil")
	}
	model := &BanditArm{handle: handle}
	return withFinalizer(model, (*BanditArm).finalize), nil
}

func (b *BanditArm) Pull() (float64, error) {
	if b == nil || b.handle == nil {
		return 0, fmt.Errorf("bandit arm is nil")
	}
	var out C.double
	var cErr *C.char
	ok := C.coolbox_bandit_arm_pull(b.handle, &out, &cErr)
	if err := cError(cErr); err != nil {
		return 0, err
	}
	if ok == 0 {
		return 0, fmt.Errorf("bandit pull failed")
	}
	return float64(out), nil
}

func (b *BanditArm) Update(reward float64) error {
	if b == nil || b.handle == nil {
		return fmt.Errorf("bandit arm is nil")
	}
	var cErr *C.char
	ok := C.coolbox_bandit_arm_update(b.handle, C.double(reward), &cErr)
	if err := cError(cErr); err != nil {
		return err
	}
	if ok == 0 {
		return fmt.Errorf("bandit update failed")
	}
	return nil
}

func (b *BanditArm) EstimatedProbability() float64 {
	if b == nil || b.handle == nil {
		return 0
	}
	return float64(C.coolbox_bandit_arm_estimated_prob(b.handle))
}

func (b *BanditArm) PullCount() int {
	if b == nil || b.handle == nil {
		return 0
	}
	return int(C.coolbox_bandit_arm_pull_count(b.handle))
}

func (b *BanditArm) TrueProbability() float64 {
	if b == nil || b.handle == nil {
		return 0
	}
	return float64(C.coolbox_bandit_arm_true_prob(b.handle))
}

func (b *BanditArm) finalize() { b.Close() }

func (b *BanditArm) Close() {
	if b != nil && b.handle != nil {
		C.coolbox_free_bandit_arm(b.handle)
		b.handle = nil
	}
}

func NewEpsilonGreedyAgent(trueProbs []float64, epsilon float64, seed int64) (*BanditAgent, error) {
	if len(trueProbs) == 0 {
		return nil, fmt.Errorf("at least one bandit arm is required")
	}
	var cErr *C.char
	handle := C.coolbox_create_epsilon_greedy_agent(cDoublePtr(trueProbs), C.size_t(len(trueProbs)), C.double(epsilon), C.double(0), C.longlong(seed), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("bandit agent handle is nil")
	}
	model := &BanditAgent{handle: handle}
	return withFinalizer(model, (*BanditAgent).finalize), nil
}

func NewUCBAgent(trueProbs []float64, c float64) (*BanditAgent, error) {
	if len(trueProbs) == 0 {
		return nil, fmt.Errorf("at least one bandit arm is required")
	}
	var cErr *C.char
	handle := C.coolbox_create_ucb_agent(cDoublePtr(trueProbs), C.size_t(len(trueProbs)), C.double(c), C.double(0), C.longlong(0), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("bandit agent handle is nil")
	}
	model := &BanditAgent{handle: handle}
	return withFinalizer(model, (*BanditAgent).finalize), nil
}

func NewThompsonSamplingAgent(trueProbs []float64, seed int64) (*BanditAgent, error) {
	if len(trueProbs) == 0 {
		return nil, fmt.Errorf("at least one bandit arm is required")
	}
	var cErr *C.char
	handle := C.coolbox_create_thompson_sampling_agent(cDoublePtr(trueProbs), C.size_t(len(trueProbs)), C.double(0), C.double(0), C.longlong(seed), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("bandit agent handle is nil")
	}
	model := &BanditAgent{handle: handle}
	return withFinalizer(model, (*BanditAgent).finalize), nil
}

func NewDecayingEpsilonGreedyAgent(trueProbs []float64, initialEpsilon, decayRate float64, seed int64) (*BanditAgent, error) {
	if len(trueProbs) == 0 {
		return nil, fmt.Errorf("at least one bandit arm is required")
	}
	var cErr *C.char
	handle := C.coolbox_create_decaying_epsilon_greedy_agent(cDoublePtr(trueProbs), C.size_t(len(trueProbs)), C.double(initialEpsilon), C.double(decayRate), C.longlong(seed), &cErr)
	if err := cError(cErr); err != nil {
		return nil, err
	}
	if handle == nil {
		return nil, fmt.Errorf("bandit agent handle is nil")
	}
	model := &BanditAgent{handle: handle}
	return withFinalizer(model, (*BanditAgent).finalize), nil
}

func (a *BanditAgent) RunSimulation(steps int) error {
	if a == nil || a.handle == nil {
		return fmt.Errorf("bandit agent is nil")
	}
	var cErr *C.char
	ok := C.coolbox_bandit_agent_run_simulation(a.handle, C.int(steps), &cErr)
	if err := cError(cErr); err != nil {
		return err
	}
	if ok == 0 {
		return fmt.Errorf("bandit simulation failed")
	}
	return nil
}

func (a *BanditAgent) Results() (SimulationResult, error) {
	if a == nil || a.handle == nil {
		return SimulationResult{}, fmt.Errorf("bandit agent is nil")
	}
	count := int(C.coolbox_bandit_agent_arm_count(a.handle))
	trueProbs := make([]float64, count)
	estimated := make([]float64, count)
	pullsC := make([]C.int, count)
	var cErr *C.char
	ok := C.coolbox_bandit_agent_get_results(
		a.handle,
		cDoublePtr(trueProbs),
		cDoublePtr(estimated),
		cCIntPtr(pullsC),
		C.size_t(count),
		&cErr,
	)
	if err := cError(cErr); err != nil {
		return SimulationResult{}, err
	}
	if ok == 0 {
		return SimulationResult{}, fmt.Errorf("collecting bandit results failed")
	}

	result := SimulationResult{Bandits: make([]BanditStats, count)}
	pulls := fromCIntSlice(pullsC)
	for i := range result.Bandits {
		result.Bandits[i] = BanditStats{
			TrueProbability:      trueProbs[i],
			EstimatedProbability: estimated[i],
			TimesPulled:          pulls[i],
		}
	}
	return result, nil
}

func (a *BanditAgent) finalize() { a.Close() }

func (a *BanditAgent) Close() {
	if a != nil && a.handle != nil {
		C.coolbox_free_bandit_agent(a.handle)
		a.handle = nil
	}
}
