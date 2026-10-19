#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "ds_bridge.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace py = pybind11;

namespace {

enum class HeapKind {
    Max,
    Min,
};

enum class TreeKind {
    BinarySearch,
    RedBlack,
    Splay,
};

enum class CoverTreeKind {
    Standard,
    Faster,
};

void require_status(int status, const char* message) {
    if (!status) {
        throw std::runtime_error(message);
    }
}

using Deleter = void (*)(void*);

class NativeHandle {
public:
    NativeHandle(void* value, Deleter deleter, const char* name)
        : value_(value), deleter_(deleter), name_(name) {
        if (!value_) {
            throw std::invalid_argument(std::string("failed to create ") + name_);
        }
    }

    NativeHandle(const NativeHandle&) = delete;
    NativeHandle& operator=(const NativeHandle&) = delete;

    virtual ~NativeHandle() {
        close();
    }

    void close() {
        if (value_) {
            deleter_(value_);
            value_ = nullptr;
        }
    }

    bool closed() const {
        return value_ == nullptr;
    }

    void* get() const {
        if (!value_) {
            throw std::runtime_error(name_ + " is closed");
        }
        return value_;
    }

private:
    void* value_;
    Deleter deleter_;
    std::string name_;
};

struct Int64LinkedList : NativeHandle {
    Int64LinkedList() : NativeHandle(coolbox_ds_list_new(), coolbox_ds_list_free, "linked list") {}
};

struct Int64DoublyLinkedList : NativeHandle {
    Int64DoublyLinkedList()
        : NativeHandle(coolbox_ds_doubly_list_new(), coolbox_ds_doubly_list_free, "doubly linked list") {}
};

struct Int64Heap : NativeHandle {
    explicit Int64Heap(HeapKind kind)
        : NativeHandle(coolbox_ds_heap_new(kind == HeapKind::Min), coolbox_ds_heap_free, "binary heap") {}
};

struct LegacyInt64Heap : NativeHandle {
    explicit LegacyInt64Heap(HeapKind kind)
        : NativeHandle(coolbox_ds_legacy_heap_new(kind == HeapKind::Min), coolbox_ds_legacy_heap_free, "legacy heap") {}
};

struct IndexedPriorityQueue : NativeHandle {
    IndexedPriorityQueue()
        : NativeHandle(coolbox_ds_indexed_queue_new(), coolbox_ds_indexed_queue_free, "indexed priority queue") {}
};

struct Int64Map : NativeHandle {
    explicit Int64Map(size_t capacity)
        : NativeHandle(coolbox_ds_map_new(0, capacity), coolbox_ds_map_free, "hash map") {}
};

struct Int64ConcurrentMap : NativeHandle {
    explicit Int64ConcurrentMap(size_t capacity)
        : NativeHandle(coolbox_ds_map_new(1, capacity), coolbox_ds_map_free, "concurrent hash map") {}
};

struct Int64Tree : NativeHandle {
    explicit Int64Tree(TreeKind kind)
        : NativeHandle(coolbox_ds_tree_new(static_cast<int>(kind)), coolbox_ds_tree_free, "ordered tree") {}
};

struct BloomFilter : NativeHandle {
    BloomFilter(size_t expected, double rate)
        : NativeHandle(coolbox_ds_filter_new(0, expected, rate), coolbox_ds_filter_free, "Bloom filter") {}
};

struct EGHFilter : NativeHandle {
    EGHFilter(size_t expected, double rate)
        : NativeHandle(coolbox_ds_filter_new(1, expected, rate), coolbox_ds_filter_free, "EGH filter") {}
};

struct VanEmdeBoasTree : NativeHandle {
    explicit VanEmdeBoasTree(uint64_t universe)
        : NativeHandle(coolbox_ds_veb_new(universe), coolbox_ds_veb_free, "Van Emde Boas tree") {}
};

struct Float64Array : NativeHandle {
    explicit Float64Array(const std::vector<size_t>& shape)
        : NativeHandle(coolbox_ds_array_new(shape.size(), shape.data()), coolbox_ds_array_free, "float64 array") {}
};

struct Int64FolioQueue : NativeHandle {
    Int64FolioQueue()
        : NativeHandle(coolbox_ds_queue_new(), coolbox_ds_queue_free, "folio queue") {}
};

struct Int64ConcurrentLinkedList : NativeHandle {
    Int64ConcurrentLinkedList()
        : NativeHandle(coolbox_ds_concurrent_list_new(), coolbox_ds_concurrent_list_free,
                       "concurrent linked list") {}
};

struct Int64BPlusTree : NativeHandle {
    Int64BPlusTree()
        : NativeHandle(coolbox_ds_bplus_tree_new(), coolbox_ds_bplus_tree_free, "B+ tree") {}
};

struct StringRadixTree : NativeHandle {
    StringRadixTree()
        : NativeHandle(coolbox_ds_radix_tree_new(), coolbox_ds_radix_tree_free, "radix tree") {}
};

struct Int64LsmTree : NativeHandle {
    explicit Int64LsmTree(size_t threshold)
        : NativeHandle(coolbox_ds_lsm_new(threshold), coolbox_ds_lsm_free, "LSM tree") {}
};

struct Int64MapleTree : NativeHandle {
    Int64MapleTree()
        : NativeHandle(coolbox_ds_maple_new(), coolbox_ds_maple_free, "Maple tree") {}
};

struct Float64CoverTree : NativeHandle {
    Float64CoverTree(size_t dimensions, CoverTreeKind kind)
        : NativeHandle(coolbox_ds_cover_tree_new(kind == CoverTreeKind::Faster, dimensions), coolbox_ds_cover_tree_free,
                       "cover tree"),
          dimensions_(dimensions) {}

    size_t dimensions() const {
        return dimensions_;
    }

private:
    size_t dimensions_;
};

template <typename ValuesFunction>
std::vector<int64_t> read_values(const NativeHandle& handle, ValuesFunction values) {
    const size_t count = values(handle.get(), nullptr, 0);
    std::vector<int64_t> result(count);
    if (count != 0) {
        const size_t copied = values(handle.get(), result.data(), result.size());
        if (copied != count) {
            throw std::runtime_error("failed to read native values");
        }
    }
    return result;
}

template <typename GetFunction>
std::optional<int64_t> read_map_value(const NativeHandle& handle, int64_t key, GetFunction get) {
    int64_t value = 0;
    int found = 0;
    require_status(get(handle.get(), key, &value, &found), "failed to read native map entry");
    return found ? std::optional<int64_t>(value) : std::nullopt;
}

template <typename ReadFunction>
std::optional<int64_t> read_front_back(const NativeHandle& handle, ReadFunction read) {
    int64_t value = 0;
    if (!read(handle.get(), &value)) {
        return std::nullopt;
    }
    return value;
}

void validate_string_key(const std::string& key) {
    if (key.find('\0') != std::string::npos) {
        throw std::invalid_argument("radix-tree keys cannot contain NUL characters");
    }
}

void bind_data_structures_impl(py::module_& module) {
    py::enum_<HeapKind>(module, "HeapKind")
        .value("MAX", HeapKind::Max)
        .value("MIN", HeapKind::Min);

    py::enum_<TreeKind>(module, "TreeKind")
        .value("BINARY_SEARCH", TreeKind::BinarySearch)
        .value("RED_BLACK", TreeKind::RedBlack)
        .value("SPLAY", TreeKind::Splay);

    py::class_<NativeHandle>(module, "_NativeHandle")
        .def("close", &NativeHandle::close)
        .def_property_readonly("closed", &NativeHandle::closed)
        .def("__enter__", [](py::object self) { return self; })
        .def("__exit__", [](NativeHandle& self, py::object, py::object, py::object) {
            self.close();
        });

    py::class_<Int64LinkedList, NativeHandle>(module, "Int64LinkedList")
        .def(py::init<>())
        .def("push_front", [](Int64LinkedList& self, int64_t value) {
            require_status(coolbox_ds_list_push_front(self.get(), value), "failed to push onto linked list");
        })
        .def("push_back", [](Int64LinkedList& self, int64_t value) {
            require_status(coolbox_ds_list_push_back(self.get(), value), "failed to push onto linked list");
        })
        .def("insert", [](Int64LinkedList& self, size_t index, int64_t value) {
            require_status(coolbox_ds_list_insert(self.get(), index, value), "failed to insert into linked list");
        })
        .def("pop_front", [](Int64LinkedList& self) {
            int removed = 0;
            require_status(coolbox_ds_list_pop_front(self.get(), &removed), "failed to pop linked list");
            return removed != 0;
        })
        .def("pop_back", [](Int64LinkedList& self) {
            int removed = 0;
            require_status(coolbox_ds_list_pop_back(self.get(), &removed), "failed to pop linked list");
            return removed != 0;
        })
        .def("remove_at", [](Int64LinkedList& self, size_t index) {
            int removed = 0;
            require_status(coolbox_ds_list_remove_at(self.get(), index, &removed), "failed to remove list item");
            return removed != 0;
        })
        .def("remove", [](Int64LinkedList& self, int64_t value) {
            int removed = 0;
            require_status(coolbox_ds_list_remove_value(self.get(), value, &removed), "failed to remove list item");
            return removed != 0;
        })
        .def("at", [](Int64LinkedList& self, size_t index) {
            int64_t value = 0;
            require_status(coolbox_ds_list_at(self.get(), index, &value), "linked-list index is out of range");
            return value;
        })
        .def_property_readonly("front", [](Int64LinkedList& self) {
            return read_front_back(self, coolbox_ds_list_front);
        })
        .def_property_readonly("back", [](Int64LinkedList& self) {
            return read_front_back(self, coolbox_ds_list_back);
        })
        .def("contains", [](Int64LinkedList& self, int64_t value) {
            return coolbox_ds_list_find(self.get(), value) != 0;
        })
        .def_property_readonly("values", [](Int64LinkedList& self) {
            return read_values(self, coolbox_ds_list_values);
        })
        .def_property_readonly("size", [](Int64LinkedList& self) {
            return coolbox_ds_list_size(self.get());
        })
        .def_property_readonly("empty", [](Int64LinkedList& self) {
            return coolbox_ds_list_empty(self.get()) != 0;
        })
        .def("clear", [](Int64LinkedList& self) {
            require_status(coolbox_ds_list_clear(self.get()), "failed to clear linked list");
        })
        .def("reverse", [](Int64LinkedList& self) {
            require_status(coolbox_ds_list_reverse(self.get()), "failed to reverse linked list");
        });

    py::class_<Int64DoublyLinkedList, NativeHandle>(module, "Int64DoublyLinkedList")
        .def(py::init<>())
        .def("push_front", [](Int64DoublyLinkedList& self, int64_t value) {
            require_status(coolbox_ds_doubly_list_push_front(self.get(), value), "failed to push onto list");
        })
        .def("push_back", [](Int64DoublyLinkedList& self, int64_t value) {
            require_status(coolbox_ds_doubly_list_push_back(self.get(), value), "failed to push onto list");
        })
        .def("insert", [](Int64DoublyLinkedList& self, size_t index, int64_t value) {
            require_status(coolbox_ds_doubly_list_insert(self.get(), index, value), "failed to insert into list");
        })
        .def("pop_front", [](Int64DoublyLinkedList& self) {
            int removed = 0;
            require_status(coolbox_ds_doubly_list_pop_front(self.get(), &removed), "failed to pop list");
            return removed != 0;
        })
        .def("pop_back", [](Int64DoublyLinkedList& self) {
            int removed = 0;
            require_status(coolbox_ds_doubly_list_pop_back(self.get(), &removed), "failed to pop list");
            return removed != 0;
        })
        .def("remove_at", [](Int64DoublyLinkedList& self, size_t index) {
            int removed = 0;
            require_status(coolbox_ds_doubly_list_remove_at(self.get(), index, &removed), "failed to remove item");
            return removed != 0;
        })
        .def("at", [](Int64DoublyLinkedList& self, size_t index) {
            int64_t value = 0;
            require_status(coolbox_ds_doubly_list_at(self.get(), index, &value), "list index is out of range");
            return value;
        })
        .def_property_readonly("front", [](Int64DoublyLinkedList& self) {
            return read_front_back(self, coolbox_ds_doubly_list_front);
        })
        .def_property_readonly("back", [](Int64DoublyLinkedList& self) {
            return read_front_back(self, coolbox_ds_doubly_list_back);
        })
        .def_property_readonly("values", [](Int64DoublyLinkedList& self) {
            const size_t count = coolbox_ds_doubly_list_size(self.get());
            std::vector<int64_t> values(count);
            for (size_t i = 0; i < count; ++i) {
                require_status(coolbox_ds_doubly_list_at(self.get(), i, &values[i]),
                               "failed to read doubly linked list");
            }
            return values;
        })
        .def_property_readonly("size", [](Int64DoublyLinkedList& self) {
            return coolbox_ds_doubly_list_size(self.get());
        })
        .def_property_readonly("empty", [](Int64DoublyLinkedList& self) {
            return coolbox_ds_doubly_list_empty(self.get()) != 0;
        })
        .def("clear", [](Int64DoublyLinkedList& self) {
            require_status(coolbox_ds_doubly_list_clear(self.get()), "failed to clear doubly linked list");
        });

    py::class_<Int64Heap, NativeHandle>(module, "Int64Heap")
        .def(py::init<HeapKind>(), py::arg("kind") = HeapKind::Max)
        .def("push", [](Int64Heap& self, int64_t value) {
            require_status(coolbox_ds_heap_push(self.get(), value), "failed to push onto heap");
        })
        .def("pop", [](Int64Heap& self) {
            int64_t value = 0;
            require_status(coolbox_ds_heap_top(self.get(), &value), "cannot pop an empty heap");
            require_status(coolbox_ds_heap_pop(self.get()), "failed to pop heap");
            return value;
        })
        .def_property_readonly("top", [](Int64Heap& self) {
            int64_t value = 0;
            require_status(coolbox_ds_heap_top(self.get(), &value), "heap is empty");
            return value;
        })
        .def_property_readonly("size", [](Int64Heap& self) { return coolbox_ds_heap_size(self.get()); })
        .def_property_readonly("empty", [](Int64Heap& self) { return coolbox_ds_heap_empty(self.get()) != 0; })
        .def("clear", [](Int64Heap& self) {
            require_status(coolbox_ds_heap_clear(self.get()), "failed to clear heap");
        });

    py::class_<LegacyInt64Heap, NativeHandle>(module, "LegacyInt64Heap")
        .def(py::init<HeapKind>(), py::arg("kind") = HeapKind::Max)
        .def("push", [](LegacyInt64Heap& self, int64_t value) {
            require_status(coolbox_ds_legacy_heap_push(self.get(), value), "failed to push onto legacy heap");
        })
        .def("pop", [](LegacyInt64Heap& self) {
            int64_t value = 0;
            require_status(coolbox_ds_legacy_heap_top(self.get(), &value), "legacy heap is empty");
            require_status(coolbox_ds_legacy_heap_pop(self.get()), "failed to pop legacy heap");
            return value;
        })
        .def_property_readonly("top", [](LegacyInt64Heap& self) {
            int64_t value = 0;
            require_status(coolbox_ds_legacy_heap_top(self.get(), &value), "legacy heap is empty");
            return value;
        })
        .def_property_readonly("size", [](LegacyInt64Heap& self) {
            return coolbox_ds_legacy_heap_size(self.get());
        })
        .def_property_readonly("empty", [](LegacyInt64Heap& self) {
            return coolbox_ds_legacy_heap_empty(self.get()) != 0;
        })
        .def("clear", [](LegacyInt64Heap& self) {
            require_status(coolbox_ds_legacy_heap_clear(self.get()), "failed to clear legacy heap");
        });

    py::class_<IndexedPriorityQueue, NativeHandle>(module, "IndexedPriorityQueue")
        .def(py::init<>())
        .def("push", [](IndexedPriorityQueue& self, int64_t key, double priority) {
            require_status(coolbox_ds_indexed_queue_push(self.get(), key, priority),
                           "failed to push indexed priority");
        })
        .def("update", [](IndexedPriorityQueue& self, int64_t key, double priority) {
            require_status(coolbox_ds_indexed_queue_update(self.get(), key, priority),
                           "failed to update indexed priority");
        })
        .def("pop", [](IndexedPriorityQueue& self) {
            int64_t key = 0;
            double priority = 0.0;
            require_status(coolbox_ds_indexed_queue_top(self.get(), &key, &priority),
                           "priority queue is empty");
            require_status(coolbox_ds_indexed_queue_pop(self.get()), "failed to pop priority queue");
            return std::make_pair(key, priority);
        })
        .def_property_readonly("top", [](IndexedPriorityQueue& self) {
            int64_t key = 0;
            double priority = 0.0;
            require_status(coolbox_ds_indexed_queue_top(self.get(), &key, &priority),
                           "priority queue is empty");
            return std::make_pair(key, priority);
        })
        .def("contains", [](IndexedPriorityQueue& self, int64_t key) {
            return coolbox_ds_indexed_queue_contains(self.get(), key) != 0;
        })
        .def_property_readonly("size", [](IndexedPriorityQueue& self) {
            return coolbox_ds_indexed_queue_size(self.get());
        })
        .def_property_readonly("empty", [](IndexedPriorityQueue& self) {
            return coolbox_ds_indexed_queue_empty(self.get()) != 0;
        })
        .def("clear", [](IndexedPriorityQueue& self) {
            require_status(coolbox_ds_indexed_queue_clear(self.get()), "failed to clear priority queue");
        });

    auto bind_map = [&module](const char* name, bool concurrent) {
        if (concurrent) {
            py::class_<Int64ConcurrentMap, NativeHandle>(module, name)
                .def(py::init<size_t>(), py::arg("initial_capacity") = 16)
                .def("insert", [](Int64ConcurrentMap& self, int64_t key, int64_t value) {
                    require_status(coolbox_ds_map_insert(self.get(), key, value), "failed to insert map entry");
                })
                .def("get", [](Int64ConcurrentMap& self, int64_t key) {
                    return read_map_value(self, key, coolbox_ds_map_get);
                })
                .def("remove", [](Int64ConcurrentMap& self, int64_t key) {
                    int removed = 0;
                    require_status(coolbox_ds_map_remove(self.get(), key, &removed), "failed to remove map entry");
                    return removed != 0;
                })
                .def("contains", [](Int64ConcurrentMap& self, int64_t key) {
                    return coolbox_ds_map_contains(self.get(), key) != 0;
                })
                .def_property_readonly("size", [](Int64ConcurrentMap& self) {
                    return coolbox_ds_map_size(self.get());
                })
                .def("clear", [](Int64ConcurrentMap& self) {
                    require_status(coolbox_ds_map_clear(self.get()), "failed to clear map");
                });
        } else {
            py::class_<Int64Map, NativeHandle>(module, name)
                .def(py::init<size_t>(), py::arg("initial_capacity") = 16)
                .def("insert", [](Int64Map& self, int64_t key, int64_t value) {
                    require_status(coolbox_ds_map_insert(self.get(), key, value), "failed to insert map entry");
                })
                .def("get", [](Int64Map& self, int64_t key) {
                    return read_map_value(self, key, coolbox_ds_map_get);
                })
                .def("remove", [](Int64Map& self, int64_t key) {
                    int removed = 0;
                    require_status(coolbox_ds_map_remove(self.get(), key, &removed), "failed to remove map entry");
                    return removed != 0;
                })
                .def("contains", [](Int64Map& self, int64_t key) {
                    return coolbox_ds_map_contains(self.get(), key) != 0;
                })
                .def_property_readonly("size", [](Int64Map& self) { return coolbox_ds_map_size(self.get()); })
                .def("clear", [](Int64Map& self) {
                    require_status(coolbox_ds_map_clear(self.get()), "failed to clear map");
                });
        }
    };
    bind_map("Int64Map", false);
    bind_map("Int64ConcurrentMap", true);

    py::class_<Int64Tree, NativeHandle>(module, "Int64Tree")
        .def(py::init<TreeKind>(), py::arg("kind") = TreeKind::BinarySearch)
        .def("insert", [](Int64Tree& self, int64_t value) {
            require_status(coolbox_ds_tree_insert(self.get(), value), "failed to insert into ordered tree");
        })
        .def("remove", [](Int64Tree& self, int64_t value) {
            int removed = 0;
            require_status(coolbox_ds_tree_remove(self.get(), value, &removed), "failed to remove from ordered tree");
            return removed != 0;
        })
        .def("contains", [](Int64Tree& self, int64_t value) {
            return coolbox_ds_tree_contains(self.get(), value) != 0;
        })
        .def_property_readonly("size", [](Int64Tree& self) { return coolbox_ds_tree_size(self.get()); })
        .def_property_readonly("empty", [](Int64Tree& self) { return coolbox_ds_tree_empty(self.get()) != 0; })
        .def_property_readonly("values", [](Int64Tree& self) {
            return read_values(self, coolbox_ds_tree_values);
        })
        .def_property_readonly("minimum", [](Int64Tree& self) -> std::optional<int64_t> {
            int64_t value = 0;
            if (!coolbox_ds_tree_min(self.get(), &value)) return std::nullopt;
            return value;
        })
        .def_property_readonly("maximum", [](Int64Tree& self) -> std::optional<int64_t> {
            int64_t value = 0;
            if (!coolbox_ds_tree_max(self.get(), &value)) return std::nullopt;
            return value;
        })
        .def("clear", [](Int64Tree& self) {
            require_status(coolbox_ds_tree_clear(self.get()), "failed to clear ordered tree");
        });

    auto bind_filter = [&module](const char* name, bool egh) {
        if (egh) {
            py::class_<EGHFilter, NativeHandle>(module, name)
                .def(py::init<size_t, double>(), py::arg("expected_insertions"),
                     py::arg("false_positive_rate") = 0.01)
                .def("insert", [](EGHFilter& self, int64_t value) {
                    require_status(coolbox_ds_filter_insert(self.get(), value), "failed to insert into EGH filter");
                })
                .def("contains", [](EGHFilter& self, int64_t value) {
                    return coolbox_ds_filter_contains(self.get(), value) != 0;
                })
                .def_property_readonly("insert_count", [](EGHFilter& self) {
                    return coolbox_ds_filter_insert_count(self.get());
                })
                .def_property_readonly("approximate_count", [](EGHFilter& self) {
                    return coolbox_ds_filter_approximate_count(self.get());
                })
                .def_property_readonly("estimated_false_positive_rate", [](EGHFilter& self) {
                    return coolbox_ds_filter_estimated_rate(self.get());
                })
                .def("clear", [](EGHFilter& self) {
                    require_status(coolbox_ds_filter_clear(self.get()), "failed to clear EGH filter");
                });
        } else {
            py::class_<BloomFilter, NativeHandle>(module, name)
                .def(py::init<size_t, double>(), py::arg("expected_insertions"),
                     py::arg("false_positive_rate") = 0.01)
                .def("insert", [](BloomFilter& self, int64_t value) {
                    require_status(coolbox_ds_filter_insert(self.get(), value), "failed to insert into Bloom filter");
                })
                .def("contains", [](BloomFilter& self, int64_t value) {
                    return coolbox_ds_filter_contains(self.get(), value) != 0;
                })
                .def_property_readonly("insert_count", [](BloomFilter& self) {
                    return coolbox_ds_filter_insert_count(self.get());
                })
                .def_property_readonly("approximate_count", [](BloomFilter& self) {
                    return coolbox_ds_filter_approximate_count(self.get());
                })
                .def_property_readonly("estimated_false_positive_rate", [](BloomFilter& self) {
                    return coolbox_ds_filter_estimated_rate(self.get());
                })
                .def("clear", [](BloomFilter& self) {
                    require_status(coolbox_ds_filter_clear(self.get()), "failed to clear Bloom filter");
                });
        }
    };
    bind_filter("BloomFilter", false);
    bind_filter("EGHFilter", true);

    py::class_<VanEmdeBoasTree, NativeHandle>(module, "VanEmdeBoasTree")
        .def(py::init<uint64_t>(), py::arg("universe_size"))
        .def("insert", [](VanEmdeBoasTree& self, uint64_t value) {
            require_status(coolbox_ds_veb_insert(self.get(), value), "failed to insert into vEB tree");
        })
        .def("remove", [](VanEmdeBoasTree& self, uint64_t value) {
            require_status(coolbox_ds_veb_remove(self.get(), value), "failed to remove from vEB tree");
        })
        .def("contains", [](VanEmdeBoasTree& self, uint64_t value) {
            return coolbox_ds_veb_contains(self.get(), value) != 0;
        })
        .def_property_readonly("minimum", [](VanEmdeBoasTree& self) -> std::optional<uint64_t> {
            const uint64_t value = coolbox_ds_veb_min(self.get());
            return value == std::numeric_limits<uint64_t>::max()
                ? std::nullopt : std::optional<uint64_t>(value);
        })
        .def_property_readonly("maximum", [](VanEmdeBoasTree& self) -> std::optional<uint64_t> {
            const uint64_t value = coolbox_ds_veb_max(self.get());
            return value == std::numeric_limits<uint64_t>::max()
                ? std::nullopt : std::optional<uint64_t>(value);
        })
        .def("successor", [](VanEmdeBoasTree& self, uint64_t value) -> std::optional<uint64_t> {
            const uint64_t result = coolbox_ds_veb_successor(self.get(), value);
            return result == std::numeric_limits<uint64_t>::max()
                ? std::nullopt : std::optional<uint64_t>(result);
        })
        .def("predecessor", [](VanEmdeBoasTree& self, uint64_t value) -> std::optional<uint64_t> {
            const uint64_t result = coolbox_ds_veb_predecessor(self.get(), value);
            return result == std::numeric_limits<uint64_t>::max()
                ? std::nullopt : std::optional<uint64_t>(result);
        })
        .def_property_readonly("universe_size", [](VanEmdeBoasTree& self) {
            return coolbox_ds_veb_universe_size(self.get());
        })
        .def_property_readonly("empty", [](VanEmdeBoasTree& self) {
            return coolbox_ds_veb_empty(self.get()) != 0;
        });

    py::class_<Float64Array, NativeHandle>(module, "Float64Array")
        .def(py::init<const std::vector<size_t>&>(), py::arg("shape"))
        .def_property_readonly("shape", [](Float64Array& self) {
            const size_t rank = coolbox_ds_array_rank(self.get());
            std::vector<size_t> shape(rank);
            require_status(coolbox_ds_array_shape(self.get(), shape.data(), shape.size()),
                           "failed to read array shape");
            return shape;
        })
        .def_property_readonly("size", [](Float64Array& self) {
            return coolbox_ds_array_size(self.get());
        })
        .def("get", [](Float64Array& self, const std::vector<size_t>& indices) {
            double value = 0.0;
            require_status(coolbox_ds_array_get(self.get(), indices.data(), indices.size(), &value),
                           "invalid array coordinates");
            return value;
        })
        .def("set", [](Float64Array& self, const std::vector<size_t>& indices, double value) {
            require_status(coolbox_ds_array_set(self.get(), indices.data(), indices.size(), value),
                           "invalid array coordinates");
        });

    py::class_<Int64FolioQueue, NativeHandle>(module, "Int64FolioQueue")
        .def(py::init<>())
        .def("enqueue", [](Int64FolioQueue& self, int64_t value) {
            require_status(coolbox_ds_queue_enqueue(self.get(), value), "failed to enqueue value");
        })
        .def("enqueue_many", [](Int64FolioQueue& self, const std::vector<int64_t>& values) {
            require_status(coolbox_ds_queue_enqueue_batch(self.get(), values.data(), values.size()),
                           "failed to enqueue values");
        })
        .def("dequeue", [](Int64FolioQueue& self) -> std::optional<int64_t> {
            int64_t value = 0;
            int found = 0;
            require_status(coolbox_ds_queue_try_dequeue(self.get(), &value, &found), "failed to dequeue value");
            return found ? std::optional<int64_t>(value) : std::nullopt;
        })
        .def_property_readonly("size", [](Int64FolioQueue& self) {
            return coolbox_ds_queue_size(self.get());
        })
        .def_property_readonly("empty", [](Int64FolioQueue& self) {
            return coolbox_ds_queue_empty(self.get()) != 0;
        });

    py::class_<Int64ConcurrentLinkedList, NativeHandle>(module, "Int64ConcurrentLinkedList")
        .def(py::init<>())
        .def("push_front", [](Int64ConcurrentLinkedList& self, int64_t value) {
            require_status(coolbox_ds_concurrent_list_push_front(self.get(), value), "failed to push onto list");
        })
        .def("push_back", [](Int64ConcurrentLinkedList& self, int64_t value) {
            require_status(coolbox_ds_concurrent_list_push_back(self.get(), value), "failed to push onto list");
        })
        .def("pop_front", [](Int64ConcurrentLinkedList& self) -> std::optional<int64_t> {
            int64_t value = 0;
            int found = 0;
            require_status(coolbox_ds_concurrent_list_pop_front(self.get(), &value, &found),
                           "failed to pop concurrent list");
            return found ? std::optional<int64_t>(value) : std::nullopt;
        })
        .def("remove", [](Int64ConcurrentLinkedList& self, int64_t value) {
            int removed = 0;
            require_status(coolbox_ds_concurrent_list_remove(self.get(), value, &removed),
                           "failed to remove from concurrent list");
            return removed != 0;
        })
        .def("contains", [](Int64ConcurrentLinkedList& self, int64_t value) {
            return coolbox_ds_concurrent_list_contains(self.get(), value) != 0;
        })
        .def_property_readonly("size", [](Int64ConcurrentLinkedList& self) {
            return coolbox_ds_concurrent_list_size(self.get());
        })
        .def_property_readonly("empty", [](Int64ConcurrentLinkedList& self) {
            return coolbox_ds_concurrent_list_empty(self.get()) != 0;
        });

    py::class_<Int64BPlusTree, NativeHandle>(module, "Int64BPlusTree")
        .def(py::init<>())
        .def("insert", [](Int64BPlusTree& self, int64_t key, int64_t value) {
            require_status(coolbox_ds_bplus_tree_insert(self.get(), key, value), "failed to insert B+ tree entry");
        })
        .def("get", [](Int64BPlusTree& self, int64_t key) {
            return read_map_value(self, key, coolbox_ds_bplus_tree_get);
        })
        .def("remove", [](Int64BPlusTree& self, int64_t key) {
            int removed = 0;
            require_status(coolbox_ds_bplus_tree_remove(self.get(), key, &removed), "failed to remove B+ tree entry");
            return removed != 0;
        })
        .def_property_readonly("size", [](Int64BPlusTree& self) {
            return coolbox_ds_bplus_tree_size(self.get());
        })
        .def("items", [](Int64BPlusTree& self, std::optional<int64_t> minimum,
                         std::optional<int64_t> maximum) {
            if (minimum.has_value() != maximum.has_value()) {
                throw std::invalid_argument("minimum and maximum must be supplied together");
            }
            const int range = minimum.has_value();
            const int64_t lower = minimum.value_or(0);
            const int64_t upper = maximum.value_or(0);
            size_t count = 0;
            require_status(coolbox_ds_bplus_tree_entries(self.get(), range, lower, upper,
                                                          &count, nullptr, nullptr, 0),
                           "failed to enumerate B+ tree entries");
            std::vector<int64_t> keys(count);
            std::vector<int64_t> values(count);
            if (count != 0) {
                require_status(coolbox_ds_bplus_tree_entries(self.get(), range, lower, upper,
                                                              &count, keys.data(), values.data(), count),
                               "failed to read B+ tree entries");
            }
            std::vector<std::pair<int64_t, int64_t>> entries;
            entries.reserve(count);
            for (size_t i = 0; i < count; ++i) entries.emplace_back(keys[i], values[i]);
            return entries;
        }, py::arg("minimum") = std::nullopt, py::arg("maximum") = std::nullopt)
        .def("clear", [](Int64BPlusTree& self) {
            require_status(coolbox_ds_bplus_tree_clear(self.get()), "failed to clear B+ tree");
        });

    py::class_<StringRadixTree, NativeHandle>(module, "StringRadixTree")
        .def(py::init<>())
        .def("insert", [](StringRadixTree& self, const std::string& key, int64_t value) {
            validate_string_key(key);
            require_status(coolbox_ds_radix_tree_insert(self.get(), key.c_str(), value),
                           "failed to insert radix-tree entry");
        })
        .def("get", [](StringRadixTree& self, const std::string& key) -> std::optional<int64_t> {
            validate_string_key(key);
            int64_t value = 0;
            int found = 0;
            require_status(coolbox_ds_radix_tree_get(self.get(), key.c_str(), &value, &found),
                           "failed to read radix-tree entry");
            return found ? std::optional<int64_t>(value) : std::nullopt;
        })
        .def("remove", [](StringRadixTree& self, const std::string& key) {
            validate_string_key(key);
            int removed = 0;
            require_status(coolbox_ds_radix_tree_remove(self.get(), key.c_str(), &removed),
                           "failed to remove radix-tree entry");
            return removed != 0;
        });

    py::class_<Int64LsmTree, NativeHandle>(module, "Int64LsmTree")
        .def(py::init<size_t>(), py::arg("memtable_threshold") = 64)
        .def("set", [](Int64LsmTree& self, int64_t key, int64_t value) {
            require_status(coolbox_ds_lsm_put(self.get(), key, value), "failed to store LSM entry");
        })
        .def("get", [](Int64LsmTree& self, int64_t key) {
            return read_map_value(self, key, coolbox_ds_lsm_get);
        })
        .def("remove", [](Int64LsmTree& self, int64_t key) {
            int removed = 0;
            require_status(coolbox_ds_lsm_remove(self.get(), key, &removed), "failed to remove LSM entry");
            return removed != 0;
        })
        .def("contains", [](Int64LsmTree& self, int64_t key) {
            return coolbox_ds_lsm_contains(self.get(), key) != 0;
        })
        .def("flush", [](Int64LsmTree& self) {
            require_status(coolbox_ds_lsm_flush(self.get()), "failed to flush LSM tree");
        })
        .def("compact", [](Int64LsmTree& self) {
            require_status(coolbox_ds_lsm_compact(self.get()), "failed to compact LSM tree");
        })
        .def("clear", [](Int64LsmTree& self) {
            require_status(coolbox_ds_lsm_clear(self.get()), "failed to clear LSM tree");
        })
        .def_property_readonly("size", [](Int64LsmTree& self) {
            return coolbox_ds_lsm_size(self.get());
        })
        .def_property_readonly("memtable_size", [](Int64LsmTree& self) {
            return coolbox_ds_lsm_memtable_size(self.get());
        })
        .def_property_readonly("sstable_count", [](Int64LsmTree& self) {
            return coolbox_ds_lsm_sstable_count(self.get());
        });

    py::class_<Int64MapleTree, NativeHandle>(module, "Int64MapleTree")
        .def(py::init<>())
        .def("insert", [](Int64MapleTree& self, int64_t key, int64_t value) {
            require_status(coolbox_ds_maple_insert(self.get(), key, value), "failed to insert Maple-tree entry");
        })
        .def("get", [](Int64MapleTree& self, int64_t key) {
            return read_map_value(self, key, coolbox_ds_maple_get);
        })
        .def("remove", [](Int64MapleTree& self, int64_t key) {
            int removed = 0;
            require_status(coolbox_ds_maple_remove(self.get(), key, &removed), "failed to remove Maple-tree entry");
            return removed != 0;
        });

    py::enum_<CoverTreeKind>(module, "CoverTreeKind")
        .value("STANDARD", CoverTreeKind::Standard)
        .value("FASTER", CoverTreeKind::Faster);

    py::class_<Float64CoverTree, NativeHandle>(module, "Float64CoverTree")
        .def(py::init<size_t, CoverTreeKind>(), py::arg("dimensions"),
             py::arg("kind") = CoverTreeKind::Standard)
        .def_property_readonly("dimensions", &Float64CoverTree::dimensions)
        .def("build", [](Float64CoverTree& self, const std::vector<std::vector<double>>& rows) {
            std::vector<double> flat;
            flat.reserve(rows.size() * self.dimensions());
            for (const auto& row : rows) {
                if (row.size() != self.dimensions()) {
                    throw std::invalid_argument("cover-tree vectors must match configured dimensions");
                }
                flat.insert(flat.end(), row.begin(), row.end());
            }
            require_status(coolbox_ds_cover_tree_build(self.get(), flat.data(), rows.size(),
                                                        self.dimensions()), "failed to build cover tree");
        })
        .def("insert", [](Float64CoverTree& self, const std::vector<double>& item) {
            if (item.size() != self.dimensions()) {
                throw std::invalid_argument("cover-tree vector must match configured dimensions");
            }
            require_status(coolbox_ds_cover_tree_insert(self.get(), item.data(), item.size()),
                           "failed to insert cover-tree item");
        })
        .def("clear", [](Float64CoverTree& self) {
            require_status(coolbox_ds_cover_tree_clear(self.get()), "failed to clear cover tree");
        })
        .def_property_readonly("size", [](Float64CoverTree& self) {
            return coolbox_ds_cover_tree_size(self.get());
        })
        .def("validate", [](Float64CoverTree& self) {
            return coolbox_ds_cover_tree_validate(self.get()) != 0;
        })
        .def("query", [](Float64CoverTree& self, const std::vector<double>& query, size_t k,
                         std::optional<double> radius) {
            if (query.size() != self.dimensions()) {
                throw std::invalid_argument("cover-tree query must match configured dimensions");
            }
            size_t count = 0;
            require_status(coolbox_ds_cover_tree_query(self.get(), query.data(), query.size(), k,
                                                        radius.value_or(0.0), radius.has_value(),
                                                        &count, nullptr, nullptr, nullptr, 0),
                           "failed to query cover tree");
            std::vector<size_t> indices(count);
            std::vector<double> distances(count);
            std::vector<double> items(count * self.dimensions());
            if (count != 0) {
                require_status(coolbox_ds_cover_tree_query(self.get(), query.data(), query.size(), k,
                                                            radius.value_or(0.0), radius.has_value(),
                                                            &count, indices.data(), distances.data(),
                                                            items.data(), count),
                               "failed to read cover-tree results");
            }
            std::vector<std::tuple<size_t, double, std::vector<double>>> results;
            results.reserve(count);
            for (size_t i = 0; i < count; ++i) {
                std::vector<double> item(items.begin() + i * self.dimensions(),
                                         items.begin() + (i + 1) * self.dimensions());
                results.emplace_back(indices[i], distances[i], std::move(item));
            }
            return results;
        }, py::arg("query"), py::arg("k") = 1, py::arg("radius") = std::nullopt);
}

} // namespace

void bind_data_structures(py::module_& module) {
    bind_data_structures_impl(module);
}
