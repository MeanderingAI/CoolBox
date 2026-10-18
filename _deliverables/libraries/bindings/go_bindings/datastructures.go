package coolbox

/*
#include <stdlib.h>
#include "ds_bridge.h"
*/
import "C"

import (
	"errors"
	"fmt"
	"math"
	"strings"
	"unsafe"
)

var errDataStructureClosed = errors.New("coolbox: data structure is closed")

// Int64LinkedList wraps the native singly linked list for signed 64-bit values.
type Int64LinkedList struct {
	handle unsafe.Pointer
}

func NewInt64LinkedList() (*Int64LinkedList, error) {
	handle := C.coolbox_ds_list_new()
	if handle == nil {
		return nil, errors.New("coolbox: failed to create linked list")
	}
	return &Int64LinkedList{handle: handle}, nil
}

func (list *Int64LinkedList) Close() error {
	if list != nil && list.handle != nil {
		C.coolbox_ds_list_free(list.handle)
		list.handle = nil
	}
	return nil
}

func (list *Int64LinkedList) PushFront(value int64) error {
	handle, err := list.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_list_push_front(handle, C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to add item to linked list")
	}
	return nil
}

func (list *Int64LinkedList) PushBack(value int64) error {
	handle, err := list.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_list_push_back(handle, C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to add item to linked list")
	}
	return nil
}

func (list *Int64LinkedList) InsertAt(index int, value int64) error {
	handle, err := list.nativeHandle()
	if err != nil {
		return err
	}
	if index < 0 {
		return errors.New("coolbox: list index cannot be negative")
	}
	if C.coolbox_ds_list_insert(handle, C.size_t(index), C.int64_t(value)) == 0 {
		return fmt.Errorf("coolbox: failed to insert at linked-list index %d", index)
	}
	return nil
}

func (list *Int64LinkedList) PopFront() (bool, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return false, err
	}
	var removed C.int
	if C.coolbox_ds_list_pop_front(handle, &removed) == 0 {
		return false, errors.New("coolbox: failed to remove first linked-list item")
	}
	return removed != 0, nil
}

func (list *Int64LinkedList) PopBack() (bool, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return false, err
	}
	var removed C.int
	if C.coolbox_ds_list_pop_back(handle, &removed) == 0 {
		return false, errors.New("coolbox: failed to remove last linked-list item")
	}
	return removed != 0, nil
}

func (list *Int64LinkedList) RemoveAt(index int) (bool, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return false, err
	}
	if index < 0 {
		return false, errors.New("coolbox: list index cannot be negative")
	}
	var removed C.int
	if C.coolbox_ds_list_remove_at(handle, C.size_t(index), &removed) == 0 {
		return false, fmt.Errorf("coolbox: failed to remove linked-list index %d", index)
	}
	return removed != 0, nil
}

func (list *Int64LinkedList) Remove(value int64) (bool, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return false, err
	}
	var removed C.int
	if C.coolbox_ds_list_remove_value(handle, C.int64_t(value), &removed) == 0 {
		return false, errors.New("coolbox: failed to remove linked-list value")
	}
	return removed != 0, nil
}

func (list *Int64LinkedList) At(index int) (int64, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return 0, err
	}
	if index < 0 {
		return 0, errors.New("coolbox: list index cannot be negative")
	}
	var value C.int64_t
	if C.coolbox_ds_list_at(handle, C.size_t(index), &value) == 0 {
		return 0, fmt.Errorf("coolbox: linked-list index %d is out of range", index)
	}
	return int64(value), nil
}

func (list *Int64LinkedList) Front() (int64, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return 0, err
	}
	var value C.int64_t
	if C.coolbox_ds_list_front(handle, &value) == 0 {
		return 0, errors.New("coolbox: linked list is empty")
	}
	return int64(value), nil
}

func (list *Int64LinkedList) Back() (int64, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return 0, err
	}
	var value C.int64_t
	if C.coolbox_ds_list_back(handle, &value) == 0 {
		return 0, errors.New("coolbox: linked list is empty")
	}
	return int64(value), nil
}

func (list *Int64LinkedList) Contains(value int64) (bool, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_list_find(handle, C.int64_t(value)) != 0, nil
}

func (list *Int64LinkedList) Values() ([]int64, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return nil, err
	}
	count := int(C.coolbox_ds_list_values(handle, nil, 0))
	values := make([]C.int64_t, count)
	if count > 0 {
		C.coolbox_ds_list_values(handle, &values[0], C.size_t(count))
	}
	result := make([]int64, count)
	for i, value := range values {
		result[i] = int64(value)
	}
	return result, nil
}

func (list *Int64LinkedList) Len() (int, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_list_size(handle)), nil
}

func (list *Int64LinkedList) Empty() (bool, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_list_empty(handle) != 0, nil
}

func (list *Int64LinkedList) Clear() error {
	handle, err := list.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_list_clear(handle) == 0 {
		return errors.New("coolbox: failed to clear linked list")
	}
	return nil
}

func (list *Int64LinkedList) Reverse() error {
	handle, err := list.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_list_reverse(handle) == 0 {
		return errors.New("coolbox: failed to reverse linked list")
	}
	return nil
}

func (list *Int64LinkedList) nativeHandle() (unsafe.Pointer, error) {
	if list == nil || list.handle == nil {
		return nil, errDataStructureClosed
	}
	return list.handle, nil
}

// Int64HeapKind chooses the ordering used by Int64Heap.
type Int64HeapKind int

const (
	MaxHeap Int64HeapKind = iota
	MinHeap
)

// Int64Heap wraps the native binary heap for signed 64-bit values.
type Int64Heap struct {
	handle unsafe.Pointer
}

func NewInt64Heap(kind Int64HeapKind) (*Int64Heap, error) {
	if kind != MaxHeap && kind != MinHeap {
		return nil, fmt.Errorf("coolbox: invalid heap kind %d", kind)
	}
	handle := C.coolbox_ds_heap_new(C.int(concurrentFlag(kind == MinHeap)))
	if handle == nil {
		return nil, errors.New("coolbox: failed to create heap")
	}
	return &Int64Heap{handle: handle}, nil
}

func (heap *Int64Heap) Close() error {
	if heap != nil && heap.handle != nil {
		C.coolbox_ds_heap_free(heap.handle)
		heap.handle = nil
	}
	return nil
}

func (heap *Int64Heap) Push(value int64) error {
	handle, err := heap.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_heap_push(handle, C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to push onto heap")
	}
	return nil
}

func (heap *Int64Heap) Pop() error {
	handle, err := heap.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_heap_pop(handle) == 0 {
		return errors.New("coolbox: cannot pop an empty heap")
	}
	return nil
}

func (heap *Int64Heap) Top() (int64, error) {
	handle, err := heap.nativeHandle()
	if err != nil {
		return 0, err
	}
	var value C.int64_t
	if C.coolbox_ds_heap_top(handle, &value) == 0 {
		return 0, errors.New("coolbox: heap is empty")
	}
	return int64(value), nil
}

func (heap *Int64Heap) Len() (int, error) {
	handle, err := heap.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_heap_size(handle)), nil
}

func (heap *Int64Heap) Empty() (bool, error) {
	handle, err := heap.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_heap_empty(handle) != 0, nil
}

func (heap *Int64Heap) Clear() error {
	handle, err := heap.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_heap_clear(handle) == 0 {
		return errors.New("coolbox: failed to clear heap")
	}
	return nil
}

func (heap *Int64Heap) nativeHandle() (unsafe.Pointer, error) {
	if heap == nil || heap.handle == nil {
		return nil, errDataStructureClosed
	}
	return heap.handle, nil
}

// IndexedPriorityQueue is a native min-priority queue with unique int64 keys.
type IndexedPriorityQueue struct {
	handle unsafe.Pointer
}

func NewIndexedPriorityQueue() (*IndexedPriorityQueue, error) {
	handle := C.coolbox_ds_indexed_queue_new()
	if handle == nil {
		return nil, errors.New("coolbox: failed to create indexed priority queue")
	}
	return &IndexedPriorityQueue{handle: handle}, nil
}

func (queue *IndexedPriorityQueue) Close() error {
	if queue != nil && queue.handle != nil {
		C.coolbox_ds_indexed_queue_free(queue.handle)
		queue.handle = nil
	}
	return nil
}

func (queue *IndexedPriorityQueue) Push(key int64, priority float64) error {
	handle, err := queue.nativeHandle()
	if err != nil {
		return err
	}
	if !finite(priority) {
		return errors.New("coolbox: priority must be finite")
	}
	if C.coolbox_ds_indexed_queue_push(handle, C.int64_t(key), C.double(priority)) == 0 {
		return errors.New("coolbox: failed to insert into priority queue")
	}
	return nil
}

func (queue *IndexedPriorityQueue) Update(key int64, priority float64) error {
	handle, err := queue.nativeHandle()
	if err != nil {
		return err
	}
	if !finite(priority) {
		return errors.New("coolbox: priority must be finite")
	}
	if C.coolbox_ds_indexed_queue_update(handle, C.int64_t(key), C.double(priority)) == 0 {
		return errors.New("coolbox: failed to update priority")
	}
	return nil
}

func (queue *IndexedPriorityQueue) Pop() error {
	handle, err := queue.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_indexed_queue_pop(handle) == 0 {
		return errors.New("coolbox: cannot pop an empty priority queue")
	}
	return nil
}

func (queue *IndexedPriorityQueue) Top() (key int64, priority float64, err error) {
	handle, err := queue.nativeHandle()
	if err != nil {
		return 0, 0, err
	}
	var nativeKey C.int64_t
	var nativePriority C.double
	if C.coolbox_ds_indexed_queue_top(handle, &nativeKey, &nativePriority) == 0 {
		return 0, 0, errors.New("coolbox: priority queue is empty")
	}
	return int64(nativeKey), float64(nativePriority), nil
}

func (queue *IndexedPriorityQueue) Contains(key int64) (bool, error) {
	handle, err := queue.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_indexed_queue_contains(handle, C.int64_t(key)) != 0, nil
}

func (queue *IndexedPriorityQueue) Len() (int, error) {
	handle, err := queue.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_indexed_queue_size(handle)), nil
}

func (queue *IndexedPriorityQueue) Empty() (bool, error) {
	handle, err := queue.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_indexed_queue_empty(handle) != 0, nil
}

func (queue *IndexedPriorityQueue) Clear() error {
	handle, err := queue.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_indexed_queue_clear(handle) == 0 {
		return errors.New("coolbox: failed to clear priority queue")
	}
	return nil
}

func (queue *IndexedPriorityQueue) nativeHandle() (unsafe.Pointer, error) {
	if queue == nil || queue.handle == nil {
		return nil, errDataStructureClosed
	}
	return queue.handle, nil
}

// Int64Map is a native hash map. Set concurrent=true for the thread-safe variant.
type Int64Map struct {
	handle unsafe.Pointer
}

func NewInt64Map(initialCapacity int, concurrent bool) (*Int64Map, error) {
	if initialCapacity < 1 {
		return nil, errors.New("coolbox: map initial capacity must be positive")
	}
	handle := C.coolbox_ds_map_new(C.int(concurrentFlag(concurrent)), C.size_t(initialCapacity))
	if handle == nil {
		return nil, errors.New("coolbox: failed to create hash map")
	}
	return &Int64Map{handle: handle}, nil
}

func concurrentFlag(concurrent bool) int {
	if concurrent {
		return 1
	}
	return 0
}

func (m *Int64Map) Close() error {
	if m != nil && m.handle != nil {
		C.coolbox_ds_map_free(m.handle)
		m.handle = nil
	}
	return nil
}

func (m *Int64Map) Set(key, value int64) error {
	handle, err := m.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_map_insert(handle, C.int64_t(key), C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to store map entry")
	}
	return nil
}

func (m *Int64Map) Get(key int64) (int64, bool, error) {
	handle, err := m.nativeHandle()
	if err != nil {
		return 0, false, err
	}
	var value C.int64_t
	var found C.int
	if C.coolbox_ds_map_get(handle, C.int64_t(key), &value, &found) == 0 {
		return 0, false, errors.New("coolbox: failed to look up map entry")
	}
	return int64(value), found != 0, nil
}

func (m *Int64Map) Contains(key int64) (bool, error) {
	handle, err := m.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_map_contains(handle, C.int64_t(key)) != 0, nil
}

func (m *Int64Map) Delete(key int64) (bool, error) {
	handle, err := m.nativeHandle()
	if err != nil {
		return false, err
	}
	var removed C.int
	if C.coolbox_ds_map_remove(handle, C.int64_t(key), &removed) == 0 {
		return false, errors.New("coolbox: failed to delete map entry")
	}
	return removed != 0, nil
}

func (m *Int64Map) Len() (int, error) {
	handle, err := m.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_map_size(handle)), nil
}

func (m *Int64Map) Clear() error {
	handle, err := m.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_map_clear(handle) == 0 {
		return errors.New("coolbox: failed to clear map")
	}
	return nil
}

func (m *Int64Map) nativeHandle() (unsafe.Pointer, error) {
	if m == nil || m.handle == nil {
		return nil, errDataStructureClosed
	}
	return m.handle, nil
}

// Int64TreeKind selects the balancing strategy of Int64Tree.
type Int64TreeKind int

const (
	BinarySearchTreeKind Int64TreeKind = iota
	RedBlackTreeKind
	SplayTreeKind
)

// Int64Tree wraps one of the native ordered integer trees.
type Int64Tree struct {
	handle unsafe.Pointer
}

func NewInt64Tree(kind Int64TreeKind) (*Int64Tree, error) {
	if kind < BinarySearchTreeKind || kind > SplayTreeKind {
		return nil, fmt.Errorf("coolbox: invalid tree kind %d", kind)
	}
	handle := C.coolbox_ds_tree_new(C.int(kind))
	if handle == nil {
		return nil, errors.New("coolbox: failed to create ordered tree")
	}
	return &Int64Tree{handle: handle}, nil
}

func (tree *Int64Tree) Close() error {
	if tree != nil && tree.handle != nil {
		C.coolbox_ds_tree_free(tree.handle)
		tree.handle = nil
	}
	return nil
}

func (tree *Int64Tree) Insert(value int64) error {
	handle, err := tree.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_tree_insert(handle, C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to insert into tree")
	}
	return nil
}

func (tree *Int64Tree) Remove(value int64) (bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return false, err
	}
	var removed C.int
	if C.coolbox_ds_tree_remove(handle, C.int64_t(value), &removed) == 0 {
		return false, errors.New("coolbox: failed to remove from tree")
	}
	return removed != 0, nil
}

func (tree *Int64Tree) Contains(value int64) (bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_tree_contains(handle, C.int64_t(value)) != 0, nil
}

func (tree *Int64Tree) Values() ([]int64, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return nil, err
	}
	count := int(C.coolbox_ds_tree_values(handle, nil, 0))
	values := make([]C.int64_t, count)
	if count > 0 {
		C.coolbox_ds_tree_values(handle, &values[0], C.size_t(count))
	}
	result := make([]int64, count)
	for i, value := range values {
		result[i] = int64(value)
	}
	return result, nil
}

func (tree *Int64Tree) Min() (int64, error) {
	return tree.bound(true)
}

func (tree *Int64Tree) Max() (int64, error) {
	return tree.bound(false)
}

func (tree *Int64Tree) bound(minimum bool) (int64, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return 0, err
	}
	var value C.int64_t
	var ok C.int
	if minimum {
		ok = C.coolbox_ds_tree_min(handle, &value)
	} else {
		ok = C.coolbox_ds_tree_max(handle, &value)
	}
	if ok == 0 {
		return 0, errors.New("coolbox: tree is empty")
	}
	return int64(value), nil
}

func (tree *Int64Tree) Len() (int, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_tree_size(handle)), nil
}

func (tree *Int64Tree) Empty() (bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_tree_empty(handle) != 0, nil
}

func (tree *Int64Tree) Clear() error {
	handle, err := tree.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_tree_clear(handle) == 0 {
		return errors.New("coolbox: failed to clear tree")
	}
	return nil
}

func (tree *Int64Tree) nativeHandle() (unsafe.Pointer, error) {
	if tree == nil || tree.handle == nil {
		return nil, errDataStructureClosed
	}
	return tree.handle, nil
}

// ProbabilisticFilterKind selects the Bloom or EGH filter implementation.
type ProbabilisticFilterKind int

const (
	BloomFilterKind ProbabilisticFilterKind = iota
	EGHFilterKind
)

// Int64Filter is a native probabilistic membership filter.
type Int64Filter struct {
	handle unsafe.Pointer
}

func NewInt64Filter(kind ProbabilisticFilterKind, expectedInsertions int, falsePositiveRate float64) (*Int64Filter, error) {
	if kind != BloomFilterKind && kind != EGHFilterKind {
		return nil, fmt.Errorf("coolbox: invalid probabilistic filter kind %d", kind)
	}
	if expectedInsertions < 1 {
		return nil, errors.New("coolbox: expected insertions must be positive")
	}
	if math.IsNaN(falsePositiveRate) || math.IsInf(falsePositiveRate, 0) ||
		falsePositiveRate <= 0 || falsePositiveRate >= 1 {
		return nil, errors.New("coolbox: false-positive rate must be between 0 and 1")
	}
	handle := C.coolbox_ds_filter_new(C.int(kind), C.size_t(expectedInsertions), C.double(falsePositiveRate))
	if handle == nil {
		return nil, errors.New("coolbox: failed to create probabilistic filter")
	}
	return &Int64Filter{handle: handle}, nil
}

func (filter *Int64Filter) Close() error {
	if filter != nil && filter.handle != nil {
		C.coolbox_ds_filter_free(filter.handle)
		filter.handle = nil
	}
	return nil
}

func (filter *Int64Filter) Insert(value int64) error {
	handle, err := filter.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_filter_insert(handle, C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to add item to probabilistic filter")
	}
	return nil
}

func (filter *Int64Filter) Contains(value int64) (bool, error) {
	handle, err := filter.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_filter_contains(handle, C.int64_t(value)) != 0, nil
}

func (filter *Int64Filter) InsertCount() (int, error) {
	handle, err := filter.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_filter_insert_count(handle)), nil
}

func (filter *Int64Filter) ApproximateCount() (int, error) {
	handle, err := filter.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_filter_approximate_count(handle)), nil
}

func (filter *Int64Filter) EstimatedFalsePositiveRate() (float64, error) {
	handle, err := filter.nativeHandle()
	if err != nil {
		return 0, err
	}
	return float64(C.coolbox_ds_filter_estimated_rate(handle)), nil
}

func (filter *Int64Filter) Clear() error {
	handle, err := filter.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_filter_clear(handle) == 0 {
		return errors.New("coolbox: failed to clear probabilistic filter")
	}
	return nil
}

func (filter *Int64Filter) nativeHandle() (unsafe.Pointer, error) {
	if filter == nil || filter.handle == nil {
		return nil, errDataStructureClosed
	}
	return filter.handle, nil
}

// VanEmdeBoasNone is returned when a vEB query has no matching key.
const VanEmdeBoasNone uint64 = ^uint64(0)

// MaxVanEmdeBoasUniverse caps allocation-heavy vEB instances to a practical size.
const MaxVanEmdeBoasUniverse uint64 = 1 << 20

// VanEmdeBoasTree indexes non-negative integers in a bounded universe.
type VanEmdeBoasTree struct {
	handle unsafe.Pointer
}

func NewVanEmdeBoasTree(universeSize uint64) (*VanEmdeBoasTree, error) {
	if universeSize == 0 || universeSize > MaxVanEmdeBoasUniverse {
		return nil, fmt.Errorf("coolbox: vEB universe must be in [1, %d]", MaxVanEmdeBoasUniverse)
	}
	handle := C.coolbox_ds_veb_new(C.uint64_t(universeSize))
	if handle == nil {
		return nil, errors.New("coolbox: failed to create van Emde Boas tree")
	}
	return &VanEmdeBoasTree{handle: handle}, nil
}

func (tree *VanEmdeBoasTree) Close() error {
	if tree != nil && tree.handle != nil {
		C.coolbox_ds_veb_free(tree.handle)
		tree.handle = nil
	}
	return nil
}

func (tree *VanEmdeBoasTree) Insert(value uint64) error {
	handle, err := tree.nativeHandle()
	if err != nil {
		return err
	}
	if value >= uint64(C.coolbox_ds_veb_universe_size(handle)) {
		return errors.New("coolbox: vEB key is outside the universe")
	}
	if C.coolbox_ds_veb_insert(handle, C.uint64_t(value)) == 0 {
		return errors.New("coolbox: failed to insert into vEB tree")
	}
	return nil
}

func (tree *VanEmdeBoasTree) Remove(value uint64) error {
	handle, err := tree.nativeHandle()
	if err != nil {
		return err
	}
	if value >= uint64(C.coolbox_ds_veb_universe_size(handle)) {
		return errors.New("coolbox: vEB key is outside the universe")
	}
	if C.coolbox_ds_veb_remove(handle, C.uint64_t(value)) == 0 {
		return errors.New("coolbox: failed to remove from vEB tree")
	}
	return nil
}

func (tree *VanEmdeBoasTree) Contains(value uint64) (bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_veb_contains(handle, C.uint64_t(value)) != 0, nil
}

func (tree *VanEmdeBoasTree) Min() (uint64, bool, error) {
	return tree.extreme(true)
}

func (tree *VanEmdeBoasTree) Max() (uint64, bool, error) {
	return tree.extreme(false)
}

func (tree *VanEmdeBoasTree) extreme(minimum bool) (uint64, bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return 0, false, err
	}
	value := uint64(C.coolbox_ds_veb_min(handle))
	if !minimum {
		value = uint64(C.coolbox_ds_veb_max(handle))
	}
	return value, value != VanEmdeBoasNone, nil
}

func (tree *VanEmdeBoasTree) Successor(value uint64) (uint64, bool, error) {
	return tree.neighbor(value, true)
}

func (tree *VanEmdeBoasTree) Predecessor(value uint64) (uint64, bool, error) {
	return tree.neighbor(value, false)
}

func (tree *VanEmdeBoasTree) neighbor(value uint64, successor bool) (uint64, bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return 0, false, err
	}
	result := uint64(C.coolbox_ds_veb_successor(handle, C.uint64_t(value)))
	if !successor {
		result = uint64(C.coolbox_ds_veb_predecessor(handle, C.uint64_t(value)))
	}
	return result, result != VanEmdeBoasNone, nil
}

func (tree *VanEmdeBoasTree) UniverseSize() (uint64, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return 0, err
	}
	return uint64(C.coolbox_ds_veb_universe_size(handle)), nil
}

func (tree *VanEmdeBoasTree) Empty() (bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_veb_empty(handle) != 0, nil
}

func (tree *VanEmdeBoasTree) nativeHandle() (unsafe.Pointer, error) {
	if tree == nil || tree.handle == nil {
		return nil, errDataStructureClosed
	}
	return tree.handle, nil
}

// Float64Array is a row-major native xarray supporting one through four dimensions.
type Float64Array struct {
	handle unsafe.Pointer
	shape  []int
}

func NewFloat64Array(shape []int) (*Float64Array, error) {
	if len(shape) < 1 || len(shape) > 4 {
		return nil, errors.New("coolbox: xarray rank must be between 1 and 4")
	}
	limit := int(^uint(0) >> 1)
	total := 1
	cShape := make([]C.size_t, len(shape))
	for i, dimension := range shape {
		if dimension < 1 {
			return nil, fmt.Errorf("coolbox: xarray dimension %d must be positive", i)
		}
		if dimension > limit/total {
			return nil, errors.New("coolbox: xarray size overflows int")
		}
		total *= dimension
		cShape[i] = C.size_t(dimension)
	}
	handle := C.coolbox_ds_array_new(C.size_t(len(shape)), &cShape[0])
	if handle == nil {
		return nil, errors.New("coolbox: failed to create xarray")
	}
	return &Float64Array{handle: handle, shape: append([]int(nil), shape...)}, nil
}

func (array *Float64Array) Close() error {
	if array != nil && array.handle != nil {
		C.coolbox_ds_array_free(array.handle)
		array.handle = nil
	}
	return nil
}

func (array *Float64Array) Shape() ([]int, error) {
	if array == nil || array.handle == nil {
		return nil, errDataStructureClosed
	}
	return append([]int(nil), array.shape...), nil
}

func (array *Float64Array) Len() (int, error) {
	if array == nil || array.handle == nil {
		return 0, errDataStructureClosed
	}
	return int(C.coolbox_ds_array_size(array.handle)), nil
}

func (array *Float64Array) Get(indices ...int) (float64, error) {
	handle, nativeIndices, err := array.validatedIndices(indices)
	if err != nil {
		return 0, err
	}
	var value C.double
	if C.coolbox_ds_array_get(handle, &nativeIndices[0], C.size_t(len(nativeIndices)), &value) == 0 {
		return 0, errors.New("coolbox: xarray index is invalid")
	}
	return float64(value), nil
}

func (array *Float64Array) Set(value float64, indices ...int) error {
	handle, nativeIndices, err := array.validatedIndices(indices)
	if err != nil {
		return err
	}
	if C.coolbox_ds_array_set(handle, &nativeIndices[0], C.size_t(len(nativeIndices)), C.double(value)) == 0 {
		return errors.New("coolbox: xarray index is invalid")
	}
	return nil
}

func (array *Float64Array) validatedIndices(indices []int) (unsafe.Pointer, []C.size_t, error) {
	if array == nil || array.handle == nil {
		return nil, nil, errDataStructureClosed
	}
	if len(indices) != len(array.shape) {
		return nil, nil, fmt.Errorf("coolbox: expected %d xarray indices, got %d", len(array.shape), len(indices))
	}
	nativeIndices := make([]C.size_t, len(indices))
	for i, index := range indices {
		if index < 0 || index >= array.shape[i] {
			return nil, nil, fmt.Errorf("coolbox: xarray index %d is out of bounds for axis %d", index, i)
		}
		nativeIndices[i] = C.size_t(index)
	}
	return array.handle, nativeIndices, nil
}

// Int64FolioQueue is a native thread-safe FIFO with non-blocking dequeue access.
type Int64FolioQueue struct {
	handle unsafe.Pointer
}

func NewInt64FolioQueue() (*Int64FolioQueue, error) {
	handle := C.coolbox_ds_queue_new()
	if handle == nil {
		return nil, errors.New("coolbox: failed to create FIFO queue")
	}
	return &Int64FolioQueue{handle: handle}, nil
}

func (queue *Int64FolioQueue) Close() error {
	if queue != nil && queue.handle != nil {
		C.coolbox_ds_queue_free(queue.handle)
		queue.handle = nil
	}
	return nil
}

func (queue *Int64FolioQueue) Enqueue(value int64) error {
	handle, err := queue.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_queue_enqueue(handle, C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to enqueue item")
	}
	return nil
}

func (queue *Int64FolioQueue) EnqueueBatch(values []int64) error {
	handle, err := queue.nativeHandle()
	if err != nil {
		return err
	}
	nativeValues := make([]C.int64_t, len(values))
	for i, value := range values {
		nativeValues[i] = C.int64_t(value)
	}
	var valuesPointer *C.int64_t
	if len(nativeValues) > 0 {
		valuesPointer = &nativeValues[0]
	}
	if C.coolbox_ds_queue_enqueue_batch(handle, valuesPointer, C.size_t(len(nativeValues))) == 0 {
		return errors.New("coolbox: failed to enqueue batch")
	}
	return nil
}

func (queue *Int64FolioQueue) TryDequeue() (int64, bool, error) {
	handle, err := queue.nativeHandle()
	if err != nil {
		return 0, false, err
	}
	var value C.int64_t
	var found C.int
	if C.coolbox_ds_queue_try_dequeue(handle, &value, &found) == 0 {
		return 0, false, errors.New("coolbox: failed to dequeue item")
	}
	return int64(value), found != 0, nil
}

func (queue *Int64FolioQueue) Len() (int, error) {
	handle, err := queue.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_queue_size(handle)), nil
}

func (queue *Int64FolioQueue) Empty() (bool, error) {
	handle, err := queue.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_queue_empty(handle) != 0, nil
}

func (queue *Int64FolioQueue) nativeHandle() (unsafe.Pointer, error) {
	if queue == nil || queue.handle == nil {
		return nil, errDataStructureClosed
	}
	return queue.handle, nil
}

// Int64ConcurrentLinkedList wraps the native thread-safe singly linked list.
type Int64ConcurrentLinkedList struct {
	handle unsafe.Pointer
}

func NewInt64ConcurrentLinkedList() (*Int64ConcurrentLinkedList, error) {
	handle := C.coolbox_ds_concurrent_list_new()
	if handle == nil {
		return nil, errors.New("coolbox: failed to create concurrent linked list")
	}
	return &Int64ConcurrentLinkedList{handle: handle}, nil
}

func (list *Int64ConcurrentLinkedList) Close() error {
	if list != nil && list.handle != nil {
		C.coolbox_ds_concurrent_list_free(list.handle)
		list.handle = nil
	}
	return nil
}

func (list *Int64ConcurrentLinkedList) PushFront(value int64) error {
	handle, err := list.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_concurrent_list_push_front(handle, C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to add item to concurrent linked list")
	}
	return nil
}

func (list *Int64ConcurrentLinkedList) PushBack(value int64) error {
	handle, err := list.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_concurrent_list_push_back(handle, C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to add item to concurrent linked list")
	}
	return nil
}

func (list *Int64ConcurrentLinkedList) PopFront() (int64, bool, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return 0, false, err
	}
	var value C.int64_t
	var found C.int
	if C.coolbox_ds_concurrent_list_pop_front(handle, &value, &found) == 0 {
		return 0, false, errors.New("coolbox: failed to remove first concurrent-list item")
	}
	return int64(value), found != 0, nil
}

func (list *Int64ConcurrentLinkedList) Remove(value int64) (bool, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return false, err
	}
	var removed C.int
	if C.coolbox_ds_concurrent_list_remove(handle, C.int64_t(value), &removed) == 0 {
		return false, errors.New("coolbox: failed to remove from concurrent linked list")
	}
	return removed != 0, nil
}

func (list *Int64ConcurrentLinkedList) Contains(value int64) (bool, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_concurrent_list_contains(handle, C.int64_t(value)) != 0, nil
}

func (list *Int64ConcurrentLinkedList) Len() (int, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_concurrent_list_size(handle)), nil
}

func (list *Int64ConcurrentLinkedList) Empty() (bool, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_concurrent_list_empty(handle) != 0, nil
}

func (list *Int64ConcurrentLinkedList) nativeHandle() (unsafe.Pointer, error) {
	if list == nil || list.handle == nil {
		return nil, errDataStructureClosed
	}
	return list.handle, nil
}

// Int64Entry is a key/value item returned by an Int64BPlusTree range query.
type Int64Entry struct {
	Key   int64
	Value int64
}

// Int64BPlusTree is the native order-4 B+ tree for signed 64-bit keys and values.
type Int64BPlusTree struct {
	handle unsafe.Pointer
}

func NewInt64BPlusTree() (*Int64BPlusTree, error) {
	handle := C.coolbox_ds_bplus_tree_new()
	if handle == nil {
		return nil, errors.New("coolbox: failed to create B+ tree")
	}
	return &Int64BPlusTree{handle: handle}, nil
}

func (tree *Int64BPlusTree) Close() error {
	if tree != nil && tree.handle != nil {
		C.coolbox_ds_bplus_tree_free(tree.handle)
		tree.handle = nil
	}
	return nil
}

func (tree *Int64BPlusTree) Set(key, value int64) error {
	handle, err := tree.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_bplus_tree_insert(handle, C.int64_t(key), C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to insert into B+ tree")
	}
	return nil
}

func (tree *Int64BPlusTree) Get(key int64) (int64, bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return 0, false, err
	}
	var value C.int64_t
	var found C.int
	if C.coolbox_ds_bplus_tree_get(handle, C.int64_t(key), &value, &found) == 0 {
		return 0, false, errors.New("coolbox: failed to search B+ tree")
	}
	return int64(value), found != 0, nil
}

func (tree *Int64BPlusTree) Delete(key int64) (bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return false, err
	}
	var removed C.int
	if C.coolbox_ds_bplus_tree_remove(handle, C.int64_t(key), &removed) == 0 {
		return false, errors.New("coolbox: failed to delete from B+ tree")
	}
	return removed != 0, nil
}

func (tree *Int64BPlusTree) Entries() ([]Int64Entry, error) {
	return tree.entries(false, 0, 0)
}

func (tree *Int64BPlusTree) Range(minimum, maximum int64) ([]Int64Entry, error) {
	if minimum > maximum {
		return nil, errors.New("coolbox: B+ tree range minimum exceeds maximum")
	}
	return tree.entries(true, minimum, maximum)
}

func (tree *Int64BPlusTree) entries(useRange bool, minimum, maximum int64) ([]Int64Entry, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return nil, err
	}
	rangeFlag := C.int(concurrentFlag(useRange))
	nativeMinimum := C.int64_t(minimum)
	nativeMaximum := C.int64_t(maximum)
	var nativeCount C.size_t
	if C.coolbox_ds_bplus_tree_entries(handle, rangeFlag, nativeMinimum, nativeMaximum,
		&nativeCount, nil, nil, 0) == 0 {
		return nil, errors.New("coolbox: failed to enumerate B+ tree entries")
	}
	count := int(nativeCount)
	result := make([]Int64Entry, count)
	if count > 0 {
		keys := make([]C.int64_t, count)
		values := make([]C.int64_t, count)
		if C.coolbox_ds_bplus_tree_entries(handle, rangeFlag, nativeMinimum, nativeMaximum,
			&nativeCount, &keys[0], &values[0], C.size_t(count)) == 0 {
			return nil, errors.New("coolbox: failed to retrieve B+ tree entries")
		}
		for index := range result {
			result[index] = Int64Entry{Key: int64(keys[index]), Value: int64(values[index])}
		}
	}
	return result, nil
}

func (tree *Int64BPlusTree) Len() (int, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_bplus_tree_size(handle)), nil
}

func (tree *Int64BPlusTree) Clear() error {
	handle, err := tree.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_bplus_tree_clear(handle) == 0 {
		return errors.New("coolbox: failed to clear B+ tree")
	}
	return nil
}

func (tree *Int64BPlusTree) nativeHandle() (unsafe.Pointer, error) {
	if tree == nil || tree.handle == nil {
		return nil, errDataStructureClosed
	}
	return tree.handle, nil
}

// StringRadixTree wraps the native radix tree with string keys and int64 values.
type StringRadixTree struct {
	handle unsafe.Pointer
}

func NewStringRadixTree() (*StringRadixTree, error) {
	handle := C.coolbox_ds_radix_tree_new()
	if handle == nil {
		return nil, errors.New("coolbox: failed to create radix tree")
	}
	return &StringRadixTree{handle: handle}, nil
}

func (tree *StringRadixTree) Close() error {
	if tree != nil && tree.handle != nil {
		C.coolbox_ds_radix_tree_free(tree.handle)
		tree.handle = nil
	}
	return nil
}

func (tree *StringRadixTree) Set(key string, value int64) error {
	handle, cKey, err := tree.nativeKey(key)
	if err != nil {
		return err
	}
	defer C.free(unsafe.Pointer(cKey))
	if C.coolbox_ds_radix_tree_insert(handle, cKey, C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to insert into radix tree")
	}
	return nil
}

func (tree *StringRadixTree) Get(key string) (int64, bool, error) {
	handle, cKey, err := tree.nativeKey(key)
	if err != nil {
		return 0, false, err
	}
	defer C.free(unsafe.Pointer(cKey))
	var value C.int64_t
	var found C.int
	if C.coolbox_ds_radix_tree_get(handle, cKey, &value, &found) == 0 {
		return 0, false, errors.New("coolbox: failed to search radix tree")
	}
	return int64(value), found != 0, nil
}

func (tree *StringRadixTree) Delete(key string) (bool, error) {
	handle, cKey, err := tree.nativeKey(key)
	if err != nil {
		return false, err
	}
	defer C.free(unsafe.Pointer(cKey))
	var removed C.int
	if C.coolbox_ds_radix_tree_remove(handle, cKey, &removed) == 0 {
		return false, errors.New("coolbox: failed to delete from radix tree")
	}
	return removed != 0, nil
}

func (tree *StringRadixTree) nativeKey(key string) (unsafe.Pointer, *C.char, error) {
	if tree == nil || tree.handle == nil {
		return nil, nil, errDataStructureClosed
	}
	if strings.IndexByte(key, 0) >= 0 {
		return nil, nil, errors.New("coolbox: radix tree keys cannot contain NUL bytes")
	}
	return tree.handle, C.CString(key), nil
}

// Int64DoublyLinkedList wraps the native doubly linked list for signed 64-bit values.
type Int64DoublyLinkedList struct {
	handle unsafe.Pointer
}

func NewInt64DoublyLinkedList() (*Int64DoublyLinkedList, error) {
	handle := C.coolbox_ds_doubly_list_new()
	if handle == nil {
		return nil, errors.New("coolbox: failed to create doubly linked list")
	}
	return &Int64DoublyLinkedList{handle: handle}, nil
}

func (list *Int64DoublyLinkedList) Close() error {
	if list != nil && list.handle != nil {
		C.coolbox_ds_doubly_list_free(list.handle)
		list.handle = nil
	}
	return nil
}

func (list *Int64DoublyLinkedList) PushFront(value int64) error {
	handle, err := list.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_doubly_list_push_front(handle, C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to add item to doubly linked list")
	}
	return nil
}

func (list *Int64DoublyLinkedList) PushBack(value int64) error {
	handle, err := list.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_doubly_list_push_back(handle, C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to add item to doubly linked list")
	}
	return nil
}

func (list *Int64DoublyLinkedList) InsertAt(index int, value int64) error {
	handle, err := list.nativeHandle()
	if err != nil {
		return err
	}
	if index < 0 {
		return errors.New("coolbox: list index cannot be negative")
	}
	if C.coolbox_ds_doubly_list_insert(handle, C.size_t(index), C.int64_t(value)) == 0 {
		return fmt.Errorf("coolbox: failed to insert at doubly-linked-list index %d", index)
	}
	return nil
}

func (list *Int64DoublyLinkedList) PopFront() (bool, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return false, err
	}
	var removed C.int
	if C.coolbox_ds_doubly_list_pop_front(handle, &removed) == 0 {
		return false, errors.New("coolbox: failed to remove first doubly-linked-list item")
	}
	return removed != 0, nil
}

func (list *Int64DoublyLinkedList) PopBack() (bool, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return false, err
	}
	var removed C.int
	if C.coolbox_ds_doubly_list_pop_back(handle, &removed) == 0 {
		return false, errors.New("coolbox: failed to remove last doubly-linked-list item")
	}
	return removed != 0, nil
}

func (list *Int64DoublyLinkedList) RemoveAt(index int) (bool, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return false, err
	}
	if index < 0 {
		return false, errors.New("coolbox: list index cannot be negative")
	}
	var removed C.int
	if C.coolbox_ds_doubly_list_remove_at(handle, C.size_t(index), &removed) == 0 {
		return false, fmt.Errorf("coolbox: failed to remove doubly-linked-list index %d", index)
	}
	return removed != 0, nil
}

func (list *Int64DoublyLinkedList) At(index int) (int64, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return 0, err
	}
	if index < 0 {
		return 0, errors.New("coolbox: list index cannot be negative")
	}
	var value C.int64_t
	if C.coolbox_ds_doubly_list_at(handle, C.size_t(index), &value) == 0 {
		return 0, fmt.Errorf("coolbox: doubly-linked-list index %d is out of range", index)
	}
	return int64(value), nil
}

func (list *Int64DoublyLinkedList) Front() (int64, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return 0, err
	}
	var value C.int64_t
	if C.coolbox_ds_doubly_list_front(handle, &value) == 0 {
		return 0, errors.New("coolbox: doubly linked list is empty")
	}
	return int64(value), nil
}

func (list *Int64DoublyLinkedList) Back() (int64, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return 0, err
	}
	var value C.int64_t
	if C.coolbox_ds_doubly_list_back(handle, &value) == 0 {
		return 0, errors.New("coolbox: doubly linked list is empty")
	}
	return int64(value), nil
}

func (list *Int64DoublyLinkedList) Values() ([]int64, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return nil, err
	}
	count := int(C.coolbox_ds_doubly_list_size(handle))
	values := make([]int64, count)
	for i := range values {
		var value C.int64_t
		if C.coolbox_ds_doubly_list_at(handle, C.size_t(i), &value) == 0 {
			return nil, errors.New("coolbox: failed to read doubly linked list")
		}
		values[i] = int64(value)
	}
	return values, nil
}

func (list *Int64DoublyLinkedList) Len() (int, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_doubly_list_size(handle)), nil
}

func (list *Int64DoublyLinkedList) Empty() (bool, error) {
	handle, err := list.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_doubly_list_empty(handle) != 0, nil
}

func (list *Int64DoublyLinkedList) Clear() error {
	handle, err := list.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_doubly_list_clear(handle) == 0 {
		return errors.New("coolbox: failed to clear doubly linked list")
	}
	return nil
}

func (list *Int64DoublyLinkedList) nativeHandle() (unsafe.Pointer, error) {
	if list == nil || list.handle == nil {
		return nil, errDataStructureClosed
	}
	return list.handle, nil
}

// LegacyInt64Heap wraps the DATASTRUCTURE module's older Heap<T, Compare>.
type LegacyInt64Heap struct {
	handle unsafe.Pointer
}

func NewLegacyInt64Heap(kind Int64HeapKind) (*LegacyInt64Heap, error) {
	if kind != MaxHeap && kind != MinHeap {
		return nil, fmt.Errorf("coolbox: invalid heap kind %d", kind)
	}
	handle := C.coolbox_ds_legacy_heap_new(C.int(concurrentFlag(kind == MinHeap)))
	if handle == nil {
		return nil, errors.New("coolbox: failed to create legacy heap")
	}
	return &LegacyInt64Heap{handle: handle}, nil
}

func (heap *LegacyInt64Heap) Close() error {
	if heap != nil && heap.handle != nil {
		C.coolbox_ds_legacy_heap_free(heap.handle)
		heap.handle = nil
	}
	return nil
}

func (heap *LegacyInt64Heap) Push(value int64) error {
	handle, err := heap.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_legacy_heap_push(handle, C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to push onto legacy heap")
	}
	return nil
}

func (heap *LegacyInt64Heap) Pop() error {
	handle, err := heap.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_legacy_heap_pop(handle) == 0 {
		return errors.New("coolbox: cannot pop an empty legacy heap")
	}
	return nil
}

func (heap *LegacyInt64Heap) Top() (int64, error) {
	handle, err := heap.nativeHandle()
	if err != nil {
		return 0, err
	}
	var value C.int64_t
	if C.coolbox_ds_legacy_heap_top(handle, &value) == 0 {
		return 0, errors.New("coolbox: legacy heap is empty")
	}
	return int64(value), nil
}

func (heap *LegacyInt64Heap) Len() (int, error) {
	handle, err := heap.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_legacy_heap_size(handle)), nil
}

func (heap *LegacyInt64Heap) Empty() (bool, error) {
	handle, err := heap.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_legacy_heap_empty(handle) != 0, nil
}

func (heap *LegacyInt64Heap) Clear() error {
	handle, err := heap.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_legacy_heap_clear(handle) == 0 {
		return errors.New("coolbox: failed to clear legacy heap")
	}
	return nil
}

func (heap *LegacyInt64Heap) nativeHandle() (unsafe.Pointer, error) {
	if heap == nil || heap.handle == nil {
		return nil, errDataStructureClosed
	}
	return heap.handle, nil
}

// Int64LsmTree wraps a native log-structured merge tree using int64 keys/values.
type Int64LsmTree struct {
	handle unsafe.Pointer
}

func NewInt64LsmTree(memtableThreshold int) (*Int64LsmTree, error) {
	if memtableThreshold < 1 {
		return nil, errors.New("coolbox: LSM memtable threshold must be positive")
	}
	handle := C.coolbox_ds_lsm_new(C.size_t(memtableThreshold))
	if handle == nil {
		return nil, errors.New("coolbox: failed to create LSM tree")
	}
	return &Int64LsmTree{handle: handle}, nil
}

func (tree *Int64LsmTree) Close() error {
	if tree != nil && tree.handle != nil {
		C.coolbox_ds_lsm_free(tree.handle)
		tree.handle = nil
	}
	return nil
}

func (tree *Int64LsmTree) Set(key, value int64) error {
	handle, err := tree.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_lsm_put(handle, C.int64_t(key), C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to store LSM entry")
	}
	return nil
}

func (tree *Int64LsmTree) Get(key int64) (int64, bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return 0, false, err
	}
	var value C.int64_t
	var found C.int
	if C.coolbox_ds_lsm_get(handle, C.int64_t(key), &value, &found) == 0 {
		return 0, false, errors.New("coolbox: failed to read LSM entry")
	}
	return int64(value), found != 0, nil
}

func (tree *Int64LsmTree) Delete(key int64) (bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return false, err
	}
	var removed C.int
	if C.coolbox_ds_lsm_remove(handle, C.int64_t(key), &removed) == 0 {
		return false, errors.New("coolbox: failed to remove LSM entry")
	}
	return removed != 0, nil
}

func (tree *Int64LsmTree) Contains(key int64) (bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_lsm_contains(handle, C.int64_t(key)) != 0, nil
}

func (tree *Int64LsmTree) Flush() error {
	handle, err := tree.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_lsm_flush(handle) == 0 {
		return errors.New("coolbox: failed to flush LSM memtable")
	}
	return nil
}

func (tree *Int64LsmTree) Compact() error {
	handle, err := tree.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_lsm_compact(handle) == 0 {
		return errors.New("coolbox: failed to compact LSM tree")
	}
	return nil
}

func (tree *Int64LsmTree) Clear() error {
	handle, err := tree.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_lsm_clear(handle) == 0 {
		return errors.New("coolbox: failed to clear LSM tree")
	}
	return nil
}

func (tree *Int64LsmTree) Len() (int, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_lsm_size(handle)), nil
}

func (tree *Int64LsmTree) MemtableLen() (int, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_lsm_memtable_size(handle)), nil
}

func (tree *Int64LsmTree) SSTableCount() (int, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_lsm_sstable_count(handle)), nil
}

func (tree *Int64LsmTree) nativeHandle() (unsafe.Pointer, error) {
	if tree == nil || tree.handle == nil {
		return nil, errDataStructureClosed
	}
	return tree.handle, nil
}

// Int64MapleTree wraps the native order-4 Maple Tree implementation.
type Int64MapleTree struct {
	handle unsafe.Pointer
}

func NewInt64MapleTree() (*Int64MapleTree, error) {
	handle := C.coolbox_ds_maple_new()
	if handle == nil {
		return nil, errors.New("coolbox: failed to create Maple tree")
	}
	return &Int64MapleTree{handle: handle}, nil
}

func (tree *Int64MapleTree) Close() error {
	if tree != nil && tree.handle != nil {
		C.coolbox_ds_maple_free(tree.handle)
		tree.handle = nil
	}
	return nil
}

func (tree *Int64MapleTree) Set(key, value int64) error {
	handle, err := tree.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_maple_insert(handle, C.int64_t(key), C.int64_t(value)) == 0 {
		return errors.New("coolbox: failed to store Maple tree entry")
	}
	return nil
}

func (tree *Int64MapleTree) Get(key int64) (int64, bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return 0, false, err
	}
	var value C.int64_t
	var found C.int
	if C.coolbox_ds_maple_get(handle, C.int64_t(key), &value, &found) == 0 {
		return 0, false, errors.New("coolbox: failed to read Maple tree entry")
	}
	return int64(value), found != 0, nil
}

func (tree *Int64MapleTree) Delete(key int64) (bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return false, err
	}
	var removed C.int
	if C.coolbox_ds_maple_remove(handle, C.int64_t(key), &removed) == 0 {
		return false, errors.New("coolbox: failed to remove Maple tree entry")
	}
	return removed != 0, nil
}

func (tree *Int64MapleTree) nativeHandle() (unsafe.Pointer, error) {
	if tree == nil || tree.handle == nil {
		return nil, errDataStructureClosed
	}
	return tree.handle, nil
}

// MetricNeighbor is a result returned by a metric-tree nearest/radius query.
type MetricNeighbor struct {
	Index    int
	Item     []float64
	Distance float64
}

// CoverTreeKind selects the standard or farthest-first cover tree.
type CoverTreeKind int

const (
	StandardCoverTree CoverTreeKind = iota
	FasterCoverTree
)

// Float64CoverTree indexes equal-dimensional points using Euclidean distance.
type Float64CoverTree struct {
	handle     unsafe.Pointer
	dimensions int
}

func NewFloat64CoverTree(kind CoverTreeKind, dimensions int) (*Float64CoverTree, error) {
	if kind != StandardCoverTree && kind != FasterCoverTree {
		return nil, fmt.Errorf("coolbox: invalid cover-tree kind %d", kind)
	}
	if dimensions < 1 {
		return nil, errors.New("coolbox: cover-tree dimensions must be positive")
	}
	handle := C.coolbox_ds_cover_tree_new(C.int(concurrentFlag(kind == FasterCoverTree)), C.size_t(dimensions))
	if handle == nil {
		return nil, errors.New("coolbox: failed to create cover tree")
	}
	return &Float64CoverTree{handle: handle, dimensions: dimensions}, nil
}

func (tree *Float64CoverTree) Close() error {
	if tree != nil && tree.handle != nil {
		C.coolbox_ds_cover_tree_free(tree.handle)
		tree.handle = nil
	}
	return nil
}

func (tree *Float64CoverTree) Build(items [][]float64) error {
	handle, err := tree.nativeHandle()
	if err != nil {
		return err
	}
	if len(items) == 0 {
		if C.coolbox_ds_cover_tree_build(handle, nil, 0, C.size_t(tree.dimensions)) == 0 {
			return errors.New("coolbox: failed to build empty cover tree")
		}
		return nil
	}
	flat, rows, columns, err := flattenMetricItems(items)
	if err != nil {
		return err
	}
	if columns != tree.dimensions {
		return fmt.Errorf("coolbox: cover tree requires %d-dimensional items, got %d", tree.dimensions, columns)
	}
	if C.coolbox_ds_cover_tree_build(handle, &flat[0], C.size_t(rows), C.size_t(columns)) == 0 {
		return errors.New("coolbox: failed to build cover tree")
	}
	return nil
}

func (tree *Float64CoverTree) Insert(item []float64) error {
	handle, err := tree.nativeHandle()
	if err != nil {
		return err
	}
	if err := validateMetricItem(item, tree.dimensions); err != nil {
		return err
	}
	nativeItem := make([]C.double, len(item))
	for i, value := range item {
		nativeItem[i] = C.double(value)
	}
	if C.coolbox_ds_cover_tree_insert(handle, &nativeItem[0], C.size_t(len(item))) == 0 {
		return errors.New("coolbox: failed to insert into cover tree")
	}
	return nil
}

func (tree *Float64CoverTree) Clear() error {
	handle, err := tree.nativeHandle()
	if err != nil {
		return err
	}
	if C.coolbox_ds_cover_tree_clear(handle) == 0 {
		return errors.New("coolbox: failed to clear cover tree")
	}
	return nil
}

func (tree *Float64CoverTree) Len() (int, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return 0, err
	}
	return int(C.coolbox_ds_cover_tree_size(handle)), nil
}

func (tree *Float64CoverTree) ValidateInvariants() (bool, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return false, err
	}
	return C.coolbox_ds_cover_tree_validate(handle) != 0, nil
}

func (tree *Float64CoverTree) KNearest(query []float64, k int) ([]MetricNeighbor, error) {
	if k < 0 {
		return nil, errors.New("coolbox: nearest-neighbor count cannot be negative")
	}
	return tree.query(query, k, 0, false)
}

func (tree *Float64CoverTree) RadiusSearch(query []float64, radius float64) ([]MetricNeighbor, error) {
	if math.IsNaN(radius) || math.IsInf(radius, 0) || radius < 0 {
		return nil, errors.New("coolbox: cover-tree radius must be finite and non-negative")
	}
	return tree.query(query, 0, radius, true)
}

func (tree *Float64CoverTree) query(query []float64, k int, radius float64, useRadius bool) ([]MetricNeighbor, error) {
	handle, err := tree.nativeHandle()
	if err != nil {
		return nil, err
	}
	if err := validateMetricItem(query, tree.dimensions); err != nil {
		return nil, err
	}
	nativeQuery := make([]C.double, len(query))
	for i, value := range query {
		nativeQuery[i] = C.double(value)
	}
	var count C.size_t
	if C.coolbox_ds_cover_tree_query(handle, &nativeQuery[0], C.size_t(len(query)), C.size_t(k),
		C.double(radius), C.int(concurrentFlag(useRadius)), &count, nil, nil, nil, 0) == 0 {
		return nil, errors.New("coolbox: cover-tree query failed")
	}
	n := int(count)
	neighbors := make([]MetricNeighbor, n)
	if n == 0 {
		return neighbors, nil
	}
	indices := make([]C.size_t, n)
	distances := make([]C.double, n)
	items := make([]C.double, n*tree.dimensions)
	if C.coolbox_ds_cover_tree_query(handle, &nativeQuery[0], C.size_t(len(query)), C.size_t(k),
		C.double(radius), C.int(concurrentFlag(useRadius)), &count, &indices[0], &distances[0],
		&items[0], C.size_t(n)) == 0 {
		return nil, errors.New("coolbox: failed to read cover-tree query results")
	}
	for i := range neighbors {
		point := make([]float64, tree.dimensions)
		for d := range point {
			point[d] = float64(items[i*tree.dimensions+d])
		}
		neighbors[i] = MetricNeighbor{
			Index:    int(indices[i]),
			Item:     point,
			Distance: float64(distances[i]),
		}
	}
	return neighbors, nil
}

func (tree *Float64CoverTree) nativeHandle() (unsafe.Pointer, error) {
	if tree == nil || tree.handle == nil {
		return nil, errDataStructureClosed
	}
	return tree.handle, nil
}

func flattenMetricItems(items [][]float64) ([]C.double, int, int, error) {
	if len(items) == 0 {
		return nil, 0, 0, errors.New("coolbox: cannot infer cover-tree dimension from empty input")
	}
	columns := len(items[0])
	if columns == 0 {
		return nil, 0, 0, errors.New("coolbox: cover-tree items cannot be empty")
	}
	flat := make([]C.double, 0, len(items)*columns)
	for row, item := range items {
		if err := validateMetricItem(item, columns); err != nil {
			return nil, 0, 0, fmt.Errorf("coolbox: cover-tree item %d: %w", row, err)
		}
		for _, value := range item {
			flat = append(flat, C.double(value))
		}
	}
	return flat, len(items), columns, nil
}

func validateMetricItem(item []float64, dimensions int) error {
	if len(item) != dimensions {
		return fmt.Errorf("expected %d-dimensional item, got %d", dimensions, len(item))
	}
	for i, value := range item {
		if math.IsNaN(value) || math.IsInf(value, 0) {
			return fmt.Errorf("coordinate %d must be finite", i)
		}
	}
	return nil
}
