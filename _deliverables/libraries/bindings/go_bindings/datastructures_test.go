package coolbox

import (
	"reflect"
	"testing"
)

func TestNativeLinkedList(t *testing.T) {
	list, err := NewInt64LinkedList()
	if err != nil {
		t.Fatal(err)
	}
	defer list.Close()

	for _, value := range []int64{2, 4} {
		if err := list.PushBack(value); err != nil {
			t.Fatal(err)
		}
	}
	if err := list.InsertAt(1, 3); err != nil {
		t.Fatal(err)
	}
	if err := list.PushFront(1); err != nil {
		t.Fatal(err)
	}
	values, err := list.Values()
	if err != nil || !reflect.DeepEqual(values, []int64{1, 2, 3, 4}) {
		t.Fatalf("Values() = %v, %v; want [1 2 3 4]", values, err)
	}
	if err := list.Reverse(); err != nil {
		t.Fatal(err)
	}
	last, err := list.Front()
	if err != nil || last != 4 {
		t.Fatalf("Front() after reverse = %d, %v; want 4", last, err)
	}
	removed, err := list.Remove(2)
	if err != nil || !removed {
		t.Fatalf("Remove(2) = %v, %v; want true", removed, err)
	}
}

func TestNativeHeapsAndIndexedPriorityQueue(t *testing.T) {
	maxHeap, err := NewInt64Heap(MaxHeap)
	if err != nil {
		t.Fatal(err)
	}
	defer maxHeap.Close()
	minHeap, err := NewInt64Heap(MinHeap)
	if err != nil {
		t.Fatal(err)
	}
	defer minHeap.Close()
	for _, value := range []int64{3, 1, 5} {
		if err := maxHeap.Push(value); err != nil {
			t.Fatal(err)
		}
		if err := minHeap.Push(value); err != nil {
			t.Fatal(err)
		}
	}
	if top, err := maxHeap.Top(); err != nil || top != 5 {
		t.Fatalf("max-heap Top() = %d, %v; want 5", top, err)
	}
	if top, err := minHeap.Top(); err != nil || top != 1 {
		t.Fatalf("min-heap Top() = %d, %v; want 1", top, err)
	}

	queue, err := NewIndexedPriorityQueue()
	if err != nil {
		t.Fatal(err)
	}
	defer queue.Close()
	if err := queue.Push(11, 4); err != nil {
		t.Fatal(err)
	}
	if err := queue.Push(22, 2); err != nil {
		t.Fatal(err)
	}
	if err := queue.Update(11, 1); err != nil {
		t.Fatal(err)
	}
	key, priority, err := queue.Top()
	if err != nil || key != 11 || priority != 1 {
		t.Fatalf("priority queue Top() = (%d, %v), %v; want (11, 1)", key, priority, err)
	}
}

func TestNativeHashMaps(t *testing.T) {
	for _, concurrent := range []bool{false, true} {
		m, err := NewInt64Map(8, concurrent)
		if err != nil {
			t.Fatal(err)
		}
		if err := m.Set(10, 90); err != nil {
			t.Fatal(err)
		}
		if value, found, err := m.Get(10); err != nil || !found || value != 90 {
			t.Fatalf("Get(10) = %d, %v, %v; want 90, true", value, found, err)
		}
		if removed, err := m.Delete(10); err != nil || !removed {
			t.Fatalf("Delete(10) = %v, %v; want true", removed, err)
		}
		if err := m.Close(); err != nil {
			t.Fatal(err)
		}
	}
}

func TestNativeOrderedTrees(t *testing.T) {
	for _, kind := range []Int64TreeKind{BinarySearchTreeKind, RedBlackTreeKind, SplayTreeKind} {
		tree, err := NewInt64Tree(kind)
		if err != nil {
			t.Fatal(err)
		}
		for _, value := range []int64{7, 2, 9, 1, 5, 8} {
			if err := tree.Insert(value); err != nil {
				t.Fatal(err)
			}
		}
		values, err := tree.Values()
		want := []int64{1, 2, 5, 7, 8, 9}
		if err != nil || !reflect.DeepEqual(values, want) {
			t.Fatalf("tree kind %d Values() = %v, %v; want %v", kind, values, err, want)
		}
		if value, err := tree.Min(); err != nil || value != 1 {
			t.Fatalf("tree kind %d Min() = %d, %v; want 1", kind, value, err)
		}
		if value, err := tree.Max(); err != nil || value != 9 {
			t.Fatalf("tree kind %d Max() = %d, %v; want 9", kind, value, err)
		}
		if ok, err := tree.Contains(5); err != nil || !ok {
			t.Fatalf("tree kind %d Contains(5) = %v, %v; want true", kind, ok, err)
		}
		if removed, err := tree.Remove(7); err != nil || !removed {
			t.Fatalf("tree kind %d Remove(7) = %v, %v; want true", kind, removed, err)
		}
		tree.Close()
	}
}

func TestNativeProbabilisticFilters(t *testing.T) {
	for _, kind := range []ProbabilisticFilterKind{BloomFilterKind, EGHFilterKind} {
		filter, err := NewInt64Filter(kind, 100, 0.01)
		if err != nil {
			t.Fatal(err)
		}
		for _, value := range []int64{12, 24, 36} {
			if err := filter.Insert(value); err != nil {
				t.Fatal(err)
			}
		}
		for _, value := range []int64{12, 24, 36} {
			if ok, err := filter.Contains(value); err != nil || !ok {
				t.Fatalf("filter kind %d Contains(%d) = %v, %v; want true", kind, value, ok, err)
			}
		}
		count, err := filter.InsertCount()
		if err != nil || count != 3 {
			t.Fatalf("filter kind %d InsertCount() = %d, %v; want 3", kind, count, err)
		}
		filter.Close()
	}
}

func TestNativeVanEmdeBoasTree(t *testing.T) {
	tree, err := NewVanEmdeBoasTree(100)
	if err != nil {
		t.Fatal(err)
	}
	defer tree.Close()
	for _, value := range []uint64{4, 10, 90} {
		if err := tree.Insert(value); err != nil {
			t.Fatal(err)
		}
	}
	if value, found, err := tree.Successor(10); err != nil || !found || value != 90 {
		t.Fatalf("Successor(10) = %d, %v, %v; want 90, true", value, found, err)
	}
	if value, found, err := tree.Predecessor(10); err != nil || !found || value != 4 {
		t.Fatalf("Predecessor(10) = %d, %v, %v; want 4, true", value, found, err)
	}
	if _, err := NewVanEmdeBoasTree(MaxVanEmdeBoasUniverse + 1); err == nil {
		t.Fatal("constructor accepted a vEB universe above the allocation cap")
	}
}

func TestNativeFloat64Array(t *testing.T) {
	array, err := NewFloat64Array([]int{2, 3})
	if err != nil {
		t.Fatal(err)
	}
	defer array.Close()
	if err := array.Set(3.5, 1, 2); err != nil {
		t.Fatal(err)
	}
	if value, err := array.Get(1, 2); err != nil || value != 3.5 {
		t.Fatalf("Get(1, 2) = %v, %v; want 3.5", value, err)
	}
	if _, err := array.Get(2, 0); err == nil {
		t.Fatal("Get accepted an out-of-range index")
	}
}

func TestNativeQueueAndConcurrentList(t *testing.T) {
	queue, err := NewInt64FolioQueue()
	if err != nil {
		t.Fatal(err)
	}
	defer queue.Close()
	if err := queue.EnqueueBatch([]int64{4, 5, 6}); err != nil {
		t.Fatal(err)
	}
	for _, want := range []int64{4, 5, 6} {
		value, found, err := queue.TryDequeue()
		if err != nil || !found || value != want {
			t.Fatalf("TryDequeue() = %d, %v, %v; want %d, true", value, found, err, want)
		}
	}
	if _, found, err := queue.TryDequeue(); err != nil || found {
		t.Fatalf("TryDequeue() on empty queue found item: %v, %v", found, err)
	}

	list, err := NewInt64ConcurrentLinkedList()
	if err != nil {
		t.Fatal(err)
	}
	defer list.Close()
	if err := list.PushBack(8); err != nil {
		t.Fatal(err)
	}
	if err := list.PushFront(7); err != nil {
		t.Fatal(err)
	}
	value, found, err := list.PopFront()
	if err != nil || !found || value != 7 {
		t.Fatalf("PopFront() = %d, %v, %v; want 7, true", value, found, err)
	}
}

func TestNativeBPlusTreeAndRadixTree(t *testing.T) {
	bplus, err := NewInt64BPlusTree()
	if err != nil {
		t.Fatal(err)
	}
	defer bplus.Close()
	for key := int64(0); key < 12; key++ {
		if err := bplus.Set(key, key*10); err != nil {
			t.Fatal(err)
		}
	}
	if value, found, err := bplus.Get(7); err != nil || !found || value != 70 {
		t.Fatalf("B+ tree Get(7) = %d, %v, %v; want 70, true", value, found, err)
	}
	entries, err := bplus.Range(3, 6)
	if err != nil || len(entries) != 4 || entries[0] != (Int64Entry{Key: 3, Value: 30}) ||
		entries[3] != (Int64Entry{Key: 6, Value: 60}) {
		t.Fatalf("B+ tree Range(3, 6) = %v, %v", entries, err)
	}

	radix, err := NewStringRadixTree()
	if err != nil {
		t.Fatal(err)
	}
	defer radix.Close()
	for _, entry := range []struct {
		key   string
		value int64
	}{{"car", 1}, {"cart", 2}, {"cat", 3}} {
		if err := radix.Set(entry.key, entry.value); err != nil {
			t.Fatal(err)
		}
	}
	for _, entry := range []struct {
		key   string
		value int64
	}{{"car", 1}, {"cart", 2}, {"cat", 3}} {
		if value, found, err := radix.Get(entry.key); err != nil || !found || value != entry.value {
			t.Fatalf("radix Get(%q) = %d, %v, %v; want %d, true", entry.key, value, found, err, entry.value)
		}
	}
}

func TestRemainingNativeStructures(t *testing.T) {
	list, err := NewInt64DoublyLinkedList()
	if err != nil {
		t.Fatal(err)
	}
	defer list.Close()
	for _, value := range []int64{2, 4} {
		if err := list.PushBack(value); err != nil {
			t.Fatal(err)
		}
	}
	if err := list.InsertAt(1, 3); err != nil {
		t.Fatal(err)
	}
	if values, err := list.Values(); err != nil || !reflect.DeepEqual(values, []int64{2, 3, 4}) {
		t.Fatalf("doubly linked list Values() = %v, %v", values, err)
	}
	if removed, err := list.PopBack(); err != nil || !removed {
		t.Fatalf("doubly linked list PopBack() = %v, %v", removed, err)
	}

	heap, err := NewLegacyInt64Heap(MaxHeap)
	if err != nil {
		t.Fatal(err)
	}
	defer heap.Close()
	for _, value := range []int64{2, 8, 5} {
		if err := heap.Push(value); err != nil {
			t.Fatal(err)
		}
	}
	if top, err := heap.Top(); err != nil || top != 8 {
		t.Fatalf("legacy heap Top() = %d, %v; want 8", top, err)
	}

	lsm, err := NewInt64LsmTree(2)
	if err != nil {
		t.Fatal(err)
	}
	defer lsm.Close()
	for key := int64(1); key <= 4; key++ {
		if err := lsm.Set(key, key*10); err != nil {
			t.Fatal(err)
		}
	}
	if value, found, err := lsm.Get(3); err != nil || !found || value != 30 {
		t.Fatalf("LSM Get(3) = %d, %v, %v; want 30, true", value, found, err)
	}
	if _, err := lsm.Delete(3); err != nil {
		t.Fatal(err)
	}
	if err := lsm.Compact(); err != nil {
		t.Fatal(err)
	}
	if _, found, err := lsm.Get(3); err != nil || found {
		t.Fatalf("LSM Get(3) after delete = found %v, err %v; want false", found, err)
	}

	maple, err := NewInt64MapleTree()
	if err != nil {
		t.Fatal(err)
	}
	defer maple.Close()
	for key := int64(0); key < 80; key++ {
		if err := maple.Set(key, key+100); err != nil {
			t.Fatal(err)
		}
	}
	if value, found, err := maple.Get(63); err != nil || !found || value != 163 {
		t.Fatalf("Maple Get(63) = %d, %v, %v; want 163, true", value, found, err)
	}
	if removed, err := maple.Delete(63); err != nil || !removed {
		t.Fatalf("Maple Delete(63) = %v, %v; want true", removed, err)
	}
	if _, found, err := maple.Get(63); err != nil || found {
		t.Fatalf("Maple Get(63) after delete found %v, err %v", found, err)
	}
}

func TestNativeCoverTrees(t *testing.T) {
	points := [][]float64{{0, 0}, {1, 0}, {0, 2}, {5, 5}}
	for _, kind := range []CoverTreeKind{StandardCoverTree, FasterCoverTree} {
		tree, err := NewFloat64CoverTree(kind, 2)
		if err != nil {
			t.Fatal(err)
		}
		if err := tree.Build(points); err != nil {
			t.Fatal(err)
		}
		if size, err := tree.Len(); err != nil || size != len(points) {
			t.Fatalf("cover tree kind %d Len() = %d, %v", kind, size, err)
		}
		nearest, err := tree.KNearest([]float64{0.1, 0.1}, 2)
		if err != nil || len(nearest) != 2 || nearest[0].Index != 0 {
			t.Fatalf("cover tree kind %d KNearest() = %v, %v", kind, nearest, err)
		}
		withinRadius, err := tree.RadiusSearch([]float64{0, 0}, 1.1)
		if err != nil || len(withinRadius) != 2 {
			t.Fatalf("cover tree kind %d RadiusSearch() = %v, %v; want 2 results", kind, withinRadius, err)
		}
		if err := tree.Insert([]float64{2, 2}); err != nil {
			t.Fatal(err)
		}
		tree.Close()
	}
}
