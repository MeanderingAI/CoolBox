package coolbox

/*
#include <stdlib.h>
void* coolbox_bandit_arm_new(double true_probability);
void coolbox_bandit_arm_free(void* handle);
int coolbox_bandit_arm_pull(void* handle, double* reward);
int coolbox_bandit_arm_update(void* handle, double reward);
double coolbox_bandit_arm_estimated_probability(const void* handle);
int coolbox_bandit_arm_pull_count(const void* handle);
double coolbox_bandit_arm_true_probability(const void* handle);
*/
import "C"

import (
	"errors"
	"math"
	"unsafe"
)

// BanditArm models a Bernoulli reward source and tracks its empirical mean.
type BanditArm struct {
	handle unsafe.Pointer
}

// NewBanditArm creates a Bernoulli arm with true reward probability p.
func NewBanditArm(p float64) (*BanditArm, error) {
	if math.IsNaN(p) || math.IsInf(p, 0) || p < 0 || p > 1 {
		return nil, errors.New("coolbox: bandit reward probability must be between 0 and 1")
	}
	handle := C.coolbox_bandit_arm_new(C.double(p))
	if handle == nil {
		return nil, errors.New("coolbox: failed to create bandit arm")
	}
	return &BanditArm{handle: handle}, nil
}

// Close releases the native bandit arm.
func (arm *BanditArm) Close() error {
	if arm == nil || arm.handle == nil {
		return nil
	}
	C.coolbox_bandit_arm_free(arm.handle)
	arm.handle = nil
	return nil
}

// Pull samples a reward from the arm.
func (arm *BanditArm) Pull() (float64, error) {
	handle, err := arm.nativeHandle()
	if err != nil {
		return 0, err
	}
	var reward C.double
	if C.coolbox_bandit_arm_pull(handle, &reward) == 0 {
		return 0, errors.New("coolbox: failed to sample bandit reward")
	}
	return float64(reward), nil
}

// Update adds a reward observation to the empirical estimate.
func (arm *BanditArm) Update(reward float64) error {
	handle, err := arm.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_bandit_arm_update(handle, C.double(reward)) == 0 {
		return errors.New("coolbox: failed to update bandit arm")
	}
	return nil
}

// EstimatedProbability returns the empirical mean reward.
func (arm *BanditArm) EstimatedProbability() (float64, error) {
	handle, err := arm.nativeHandle()
	if err != nil {
		return 0, err
	}
	return float64(C.coolbox_bandit_arm_estimated_probability(handle)), nil
}

// PullCount returns the number of reward observations.
func (arm *BanditArm) PullCount() (int, error) {
	handle, err := arm.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_bandit_arm_pull_count(handle)), nil
}

// TrueProbability returns the configured Bernoulli reward probability.
func (arm *BanditArm) TrueProbability() (float64, error) {
	handle, err := arm.nativeHandle()
	if err != nil {
		return 0, err
	}
	return float64(C.coolbox_bandit_arm_true_probability(handle)), nil
}

func (arm *BanditArm) nativeHandle() (unsafe.Pointer, error) {
	if arm == nil || arm.handle == nil {
		return nil, errors.New("coolbox: bandit arm is closed")
	}
	return arm.handle, nil
}
