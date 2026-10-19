import pytest

try:
    from ml_toolbox import data_structures as ds
except ImportError as error:
    pytest.skip(f"CoolBox Python extension is not built: {error}", allow_module_level=True)


def test_lists_heaps_and_queue():
    with ds.Int64LinkedList() as linked:
        linked.push_back(8)
        linked.push_front(3)
        linked.insert(1, 5)
        assert linked.values == [3, 5, 8]
        assert linked.front == 3
        assert linked.back == 8
        assert linked.remove(5)
        assert linked.pop_back()
        assert linked.values == [3]

    with ds.Int64DoublyLinkedList() as linked:
        linked.push_back(4)
        linked.push_front(2)
        assert linked.values == [2, 4]
        assert linked.pop_front()
        assert linked.front == 4

    with ds.Int64Heap(ds.HeapKind.MIN) as heap:
        heap.push(4)
        heap.push(1)
        heap.push(3)
        assert heap.top == 1
        assert heap.pop() == 1

    with ds.LegacyInt64Heap(ds.HeapKind.MAX) as heap:
        heap.push(4)
        heap.push(9)
        assert heap.pop() == 9

    with ds.Int64FolioQueue() as queue:
        queue.enqueue_many([2, 4])
        assert queue.dequeue() == 2
        assert queue.dequeue() == 4
        assert queue.dequeue() is None


def test_maps_trees_and_filters():
    with ds.Int64Map() as mapping:
        mapping.insert(7, 70)
        assert mapping.get(7) == 70
        assert mapping.get(8) is None
        assert mapping.remove(7)

    with ds.Int64ConcurrentMap() as mapping:
        mapping.insert(-2, 22)
        assert mapping.contains(-2)
        assert mapping.get(-2) == 22

    with ds.Int64Tree(ds.TreeKind.RED_BLACK) as tree:
        for value in [5, 2, 8]:
            tree.insert(value)
        assert tree.values == [2, 5, 8]
        assert tree.minimum == 2
        assert tree.maximum == 8
        assert tree.remove(5)

    for filter_type in (ds.BloomFilter, ds.EGHFilter):
        with filter_type(100, 0.01) as filter_:
            filter_.insert(123)
            assert filter_.contains(123)
            assert filter_.insert_count == 1
            assert filter_.approximate_count >= 1
            filter_.clear()
            assert filter_.insert_count == 0


def test_veb_array_and_concurrent_list():
    with ds.VanEmdeBoasTree(16) as tree:
        tree.insert(3)
        tree.insert(11)
        assert tree.minimum == 3
        assert tree.maximum == 11
        assert tree.successor(3) == 11
        assert tree.predecessor(11) == 3
        tree.remove(3)
        assert tree.minimum == 11

    with ds.Float64Array([2, 3]) as array:
        assert array.shape == [2, 3]
        array.set([1, 2], 4.5)
        assert array.get([1, 2]) == pytest.approx(4.5)

    with ds.Int64ConcurrentLinkedList() as linked:
        linked.push_back(4)
        linked.push_front(1)
        assert linked.contains(4)
        assert linked.pop_front() == 1


def test_bplus_radix_lsm_and_maple_trees():
    with ds.Int64BPlusTree() as tree:
        tree.insert(4, 40)
        tree.insert(2, 20)
        assert tree.get(4) == 40
        assert tree.items() == [(2, 20), (4, 40)]
        assert tree.items(3, 5) == [(4, 40)]

    with ds.StringRadixTree() as tree:
        tree.insert("cool", 5)
        assert tree.get("cool") == 5
        assert tree.get("cold") is None
        with pytest.raises(ValueError):
            tree.insert("nul\0key", 1)

    with ds.Int64LsmTree(2) as tree:
        tree.set(8, 80)
        tree.set(3, 30)
        assert tree.get(8) == 80
        tree.flush()
        assert tree.contains(3)
        tree.compact()
        assert tree.size == 2

    with ds.Int64MapleTree() as tree:
        tree.insert(6, 60)
        assert tree.get(6) == 60
        assert tree.remove(6)


def test_cover_tree_query():
    for kind in (ds.CoverTreeKind.STANDARD, ds.CoverTreeKind.FASTER):
        with ds.Float64CoverTree(2, kind) as tree:
            tree.build([[0.0, 0.0], [1.0, 0.0], [3.0, 0.0]])
            assert tree.size == 3
            assert tree.validate()
            nearest = tree.query([0.25, 0.0], k=2)
            assert [item[0] for item in nearest] == [0, 1]
            assert [item[1] for item in nearest] == pytest.approx([0.25, 0.75])
            in_radius = tree.query([0.0, 0.0], radius=1.1)
            assert len(in_radius) == 2
