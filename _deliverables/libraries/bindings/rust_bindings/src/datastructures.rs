use std::ffi::{c_char, c_double, c_int, c_void, CString};
use std::marker::PhantomData;
use std::ptr::{self, NonNull};

unsafe extern "C" {
    fn coolbox_ds_list_new() -> *mut c_void;
    fn coolbox_ds_list_free(handle: *mut c_void);
    fn coolbox_ds_list_push_front(handle: *mut c_void, value: i64) -> c_int;
    fn coolbox_ds_list_push_back(handle: *mut c_void, value: i64) -> c_int;
    fn coolbox_ds_list_insert(handle: *mut c_void, index: usize, value: i64) -> c_int;
    fn coolbox_ds_list_pop_front(handle: *mut c_void, removed: *mut c_int) -> c_int;
    fn coolbox_ds_list_pop_back(handle: *mut c_void, removed: *mut c_int) -> c_int;
    fn coolbox_ds_list_remove_at(handle: *mut c_void, index: usize, removed: *mut c_int) -> c_int;
    fn coolbox_ds_list_remove_value(handle: *mut c_void, value: i64, removed: *mut c_int) -> c_int;
    fn coolbox_ds_list_at(handle: *const c_void, index: usize, value: *mut i64) -> c_int;
    fn coolbox_ds_list_front(handle: *const c_void, value: *mut i64) -> c_int;
    fn coolbox_ds_list_back(handle: *const c_void, value: *mut i64) -> c_int;
    fn coolbox_ds_list_find(handle: *const c_void, value: i64) -> c_int;
    fn coolbox_ds_list_size(handle: *const c_void) -> usize;
    fn coolbox_ds_list_empty(handle: *const c_void) -> c_int;
    fn coolbox_ds_list_clear(handle: *mut c_void) -> c_int;
    fn coolbox_ds_list_reverse(handle: *mut c_void) -> c_int;
    fn coolbox_ds_list_values(handle: *const c_void, values: *mut i64, capacity: usize) -> usize;

    fn coolbox_ds_heap_new(min_heap: c_int) -> *mut c_void;
    fn coolbox_ds_heap_free(handle: *mut c_void);
    fn coolbox_ds_heap_push(handle: *mut c_void, value: i64) -> c_int;
    fn coolbox_ds_heap_pop(handle: *mut c_void) -> c_int;
    fn coolbox_ds_heap_top(handle: *const c_void, value: *mut i64) -> c_int;
    fn coolbox_ds_heap_size(handle: *const c_void) -> usize;
    fn coolbox_ds_heap_empty(handle: *const c_void) -> c_int;
    fn coolbox_ds_heap_clear(handle: *mut c_void) -> c_int;

    fn coolbox_ds_indexed_queue_new() -> *mut c_void;
    fn coolbox_ds_indexed_queue_free(handle: *mut c_void);
    fn coolbox_ds_indexed_queue_push(handle: *mut c_void, key: i64, priority: c_double) -> c_int;
    fn coolbox_ds_indexed_queue_update(handle: *mut c_void, key: i64, priority: c_double) -> c_int;
    fn coolbox_ds_indexed_queue_pop(handle: *mut c_void) -> c_int;
    fn coolbox_ds_indexed_queue_top(
        handle: *const c_void,
        key: *mut i64,
        priority: *mut c_double,
    ) -> c_int;
    fn coolbox_ds_indexed_queue_contains(handle: *const c_void, key: i64) -> c_int;
    fn coolbox_ds_indexed_queue_size(handle: *const c_void) -> usize;
    fn coolbox_ds_indexed_queue_empty(handle: *const c_void) -> c_int;
    fn coolbox_ds_indexed_queue_clear(handle: *mut c_void) -> c_int;

    fn coolbox_ds_map_new(concurrent: c_int, capacity: usize) -> *mut c_void;
    fn coolbox_ds_map_free(handle: *mut c_void);
    fn coolbox_ds_map_insert(handle: *mut c_void, key: i64, value: i64) -> c_int;
    fn coolbox_ds_map_remove(handle: *mut c_void, key: i64, removed: *mut c_int) -> c_int;
    fn coolbox_ds_map_get(
        handle: *const c_void,
        key: i64,
        value: *mut i64,
        found: *mut c_int,
    ) -> c_int;
    fn coolbox_ds_map_contains(handle: *const c_void, key: i64) -> c_int;
    fn coolbox_ds_map_size(handle: *const c_void) -> usize;
    fn coolbox_ds_map_clear(handle: *mut c_void) -> c_int;

    fn coolbox_ds_tree_new(kind: c_int) -> *mut c_void;
    fn coolbox_ds_tree_free(handle: *mut c_void);
    fn coolbox_ds_tree_insert(handle: *mut c_void, value: i64) -> c_int;
    fn coolbox_ds_tree_remove(handle: *mut c_void, value: i64, removed: *mut c_int) -> c_int;
    fn coolbox_ds_tree_contains(handle: *mut c_void, value: i64) -> c_int;
    fn coolbox_ds_tree_size(handle: *const c_void) -> usize;
    fn coolbox_ds_tree_empty(handle: *const c_void) -> c_int;
    fn coolbox_ds_tree_clear(handle: *mut c_void) -> c_int;
    fn coolbox_ds_tree_min(handle: *const c_void, value: *mut i64) -> c_int;
    fn coolbox_ds_tree_max(handle: *const c_void, value: *mut i64) -> c_int;
    fn coolbox_ds_tree_values(handle: *const c_void, values: *mut i64, capacity: usize) -> usize;

    fn coolbox_ds_filter_new(kind: c_int, expected: usize, rate: c_double) -> *mut c_void;
    fn coolbox_ds_filter_free(handle: *mut c_void);
    fn coolbox_ds_filter_insert(handle: *mut c_void, value: i64) -> c_int;
    fn coolbox_ds_filter_contains(handle: *const c_void, value: i64) -> c_int;
    fn coolbox_ds_filter_insert_count(handle: *const c_void) -> usize;
    fn coolbox_ds_filter_approximate_count(handle: *const c_void) -> usize;
    fn coolbox_ds_filter_estimated_rate(handle: *const c_void) -> c_double;
    fn coolbox_ds_filter_clear(handle: *mut c_void) -> c_int;

    fn coolbox_ds_veb_new(universe: u64) -> *mut c_void;
    fn coolbox_ds_veb_free(handle: *mut c_void);
    fn coolbox_ds_veb_insert(handle: *mut c_void, value: u64) -> c_int;
    fn coolbox_ds_veb_remove(handle: *mut c_void, value: u64) -> c_int;
    fn coolbox_ds_veb_contains(handle: *const c_void, value: u64) -> c_int;
    fn coolbox_ds_veb_min(handle: *const c_void) -> u64;
    fn coolbox_ds_veb_max(handle: *const c_void) -> u64;
    fn coolbox_ds_veb_successor(handle: *const c_void, value: u64) -> u64;
    fn coolbox_ds_veb_predecessor(handle: *const c_void, value: u64) -> u64;
    fn coolbox_ds_veb_universe_size(handle: *const c_void) -> u64;
    fn coolbox_ds_veb_empty(handle: *const c_void) -> c_int;

    fn coolbox_ds_array_new(rank: usize, shape: *const usize) -> *mut c_void;
    fn coolbox_ds_array_free(handle: *mut c_void);
    fn coolbox_ds_array_size(handle: *const c_void) -> usize;
    fn coolbox_ds_array_get(
        handle: *const c_void,
        indices: *const usize,
        rank: usize,
        value: *mut c_double,
    ) -> c_int;
    fn coolbox_ds_array_set(
        handle: *mut c_void,
        indices: *const usize,
        rank: usize,
        value: c_double,
    ) -> c_int;

    fn coolbox_ds_queue_new() -> *mut c_void;
    fn coolbox_ds_queue_free(handle: *mut c_void);
    fn coolbox_ds_queue_enqueue(handle: *mut c_void, value: i64) -> c_int;
    fn coolbox_ds_queue_enqueue_batch(
        handle: *mut c_void,
        values: *const i64,
        count: usize,
    ) -> c_int;
    fn coolbox_ds_queue_try_dequeue(
        handle: *mut c_void,
        value: *mut i64,
        found: *mut c_int,
    ) -> c_int;
    fn coolbox_ds_queue_size(handle: *const c_void) -> usize;
    fn coolbox_ds_queue_empty(handle: *const c_void) -> c_int;

    fn coolbox_ds_concurrent_list_new() -> *mut c_void;
    fn coolbox_ds_concurrent_list_free(handle: *mut c_void);
    fn coolbox_ds_concurrent_list_push_front(handle: *mut c_void, value: i64) -> c_int;
    fn coolbox_ds_concurrent_list_push_back(handle: *mut c_void, value: i64) -> c_int;
    fn coolbox_ds_concurrent_list_pop_front(
        handle: *mut c_void,
        value: *mut i64,
        found: *mut c_int,
    ) -> c_int;
    fn coolbox_ds_concurrent_list_remove(
        handle: *mut c_void,
        value: i64,
        removed: *mut c_int,
    ) -> c_int;
    fn coolbox_ds_concurrent_list_contains(handle: *const c_void, value: i64) -> c_int;
    fn coolbox_ds_concurrent_list_size(handle: *const c_void) -> usize;

    fn coolbox_ds_bplus_tree_new() -> *mut c_void;
    fn coolbox_ds_bplus_tree_free(handle: *mut c_void);
    fn coolbox_ds_bplus_tree_insert(handle: *mut c_void, key: i64, value: i64) -> c_int;
    fn coolbox_ds_bplus_tree_get(
        handle: *const c_void,
        key: i64,
        value: *mut i64,
        found: *mut c_int,
    ) -> c_int;
    fn coolbox_ds_bplus_tree_remove(handle: *mut c_void, key: i64, removed: *mut c_int) -> c_int;
    fn coolbox_ds_bplus_tree_size(handle: *const c_void) -> usize;
    fn coolbox_ds_bplus_tree_clear(handle: *mut c_void) -> c_int;
    fn coolbox_ds_bplus_tree_entries(
        handle: *const c_void,
        range: c_int,
        minimum: i64,
        maximum: i64,
        count: *mut usize,
        keys: *mut i64,
        values: *mut i64,
        capacity: usize,
    ) -> c_int;

    fn coolbox_ds_radix_tree_new() -> *mut c_void;
    fn coolbox_ds_radix_tree_free(handle: *mut c_void);
    fn coolbox_ds_radix_tree_insert(handle: *mut c_void, key: *const c_char, value: i64) -> c_int;
    fn coolbox_ds_radix_tree_get(
        handle: *const c_void,
        key: *const c_char,
        value: *mut i64,
        found: *mut c_int,
    ) -> c_int;
    fn coolbox_ds_radix_tree_remove(
        handle: *mut c_void,
        key: *const c_char,
        removed: *mut c_int,
    ) -> c_int;

    fn coolbox_ds_doubly_list_new() -> *mut c_void;
    fn coolbox_ds_doubly_list_free(handle: *mut c_void);
    fn coolbox_ds_doubly_list_push_front(handle: *mut c_void, value: i64) -> c_int;
    fn coolbox_ds_doubly_list_push_back(handle: *mut c_void, value: i64) -> c_int;
    fn coolbox_ds_doubly_list_insert(handle: *mut c_void, index: usize, value: i64) -> c_int;
    fn coolbox_ds_doubly_list_pop_front(handle: *mut c_void, removed: *mut c_int) -> c_int;
    fn coolbox_ds_doubly_list_pop_back(handle: *mut c_void, removed: *mut c_int) -> c_int;
    fn coolbox_ds_doubly_list_remove_at(
        handle: *mut c_void,
        index: usize,
        removed: *mut c_int,
    ) -> c_int;
    fn coolbox_ds_doubly_list_front(handle: *const c_void, value: *mut i64) -> c_int;
    fn coolbox_ds_doubly_list_back(handle: *const c_void, value: *mut i64) -> c_int;
    fn coolbox_ds_doubly_list_at(handle: *const c_void, index: usize, value: *mut i64) -> c_int;
    fn coolbox_ds_doubly_list_size(handle: *const c_void) -> usize;
    fn coolbox_ds_doubly_list_empty(handle: *const c_void) -> c_int;
    fn coolbox_ds_doubly_list_clear(handle: *mut c_void) -> c_int;

    fn coolbox_ds_legacy_heap_new(min_heap: c_int) -> *mut c_void;
    fn coolbox_ds_legacy_heap_free(handle: *mut c_void);
    fn coolbox_ds_legacy_heap_push(handle: *mut c_void, value: i64) -> c_int;
    fn coolbox_ds_legacy_heap_pop(handle: *mut c_void) -> c_int;
    fn coolbox_ds_legacy_heap_top(handle: *const c_void, value: *mut i64) -> c_int;
    fn coolbox_ds_legacy_heap_size(handle: *const c_void) -> usize;
    fn coolbox_ds_legacy_heap_empty(handle: *const c_void) -> c_int;
    fn coolbox_ds_legacy_heap_clear(handle: *mut c_void) -> c_int;

    fn coolbox_ds_lsm_new(threshold: usize) -> *mut c_void;
    fn coolbox_ds_lsm_free(handle: *mut c_void);
    fn coolbox_ds_lsm_put(handle: *mut c_void, key: i64, value: i64) -> c_int;
    fn coolbox_ds_lsm_get(
        handle: *const c_void,
        key: i64,
        value: *mut i64,
        found: *mut c_int,
    ) -> c_int;
    fn coolbox_ds_lsm_remove(handle: *mut c_void, key: i64, removed: *mut c_int) -> c_int;
    fn coolbox_ds_lsm_contains(handle: *const c_void, key: i64) -> c_int;
    fn coolbox_ds_lsm_flush(handle: *mut c_void) -> c_int;
    fn coolbox_ds_lsm_compact(handle: *mut c_void) -> c_int;
    fn coolbox_ds_lsm_clear(handle: *mut c_void) -> c_int;
    fn coolbox_ds_lsm_size(handle: *const c_void) -> usize;
    fn coolbox_ds_lsm_memtable_size(handle: *const c_void) -> usize;
    fn coolbox_ds_lsm_sstable_count(handle: *const c_void) -> usize;

    fn coolbox_ds_maple_new() -> *mut c_void;
    fn coolbox_ds_maple_free(handle: *mut c_void);
    fn coolbox_ds_maple_insert(handle: *mut c_void, key: i64, value: i64) -> c_int;
    fn coolbox_ds_maple_get(
        handle: *const c_void,
        key: i64,
        value: *mut i64,
        found: *mut c_int,
    ) -> c_int;
    fn coolbox_ds_maple_remove(handle: *mut c_void, key: i64, removed: *mut c_int) -> c_int;

    fn coolbox_ds_cover_tree_new(faster: c_int, dimensions: usize) -> *mut c_void;
    fn coolbox_ds_cover_tree_free(handle: *mut c_void);
    fn coolbox_ds_cover_tree_build(
        handle: *mut c_void,
        values: *const c_double,
        count: usize,
        dimensions: usize,
    ) -> c_int;
    fn coolbox_ds_cover_tree_insert(
        handle: *mut c_void,
        item: *const c_double,
        dimensions: usize,
    ) -> c_int;
    fn coolbox_ds_cover_tree_clear(handle: *mut c_void) -> c_int;
    fn coolbox_ds_cover_tree_size(handle: *const c_void) -> usize;
    fn coolbox_ds_cover_tree_validate(handle: *const c_void) -> c_int;
    fn coolbox_ds_cover_tree_query(
        handle: *const c_void,
        query: *const c_double,
        dimensions: usize,
        k: usize,
        radius: c_double,
        use_radius: c_int,
        count: *mut usize,
        indices: *mut usize,
        distances: *mut c_double,
        items: *mut c_double,
        capacity: usize,
    ) -> c_int;
}

struct Handle {
    ptr: NonNull<c_void>,
    free: unsafe extern "C" fn(*mut c_void),
    _not_sync: PhantomData<*mut ()>,
}

impl Handle {
    fn new(
        ptr: *mut c_void,
        free: unsafe extern "C" fn(*mut c_void),
        what: &str,
    ) -> Result<Self, String> {
        Ok(Self {
            ptr: NonNull::new(ptr).ok_or_else(|| format!("failed to create {what}"))?,
            free,
            _not_sync: PhantomData,
        })
    }

    fn as_ptr(&self) -> *mut c_void {
        self.ptr.as_ptr()
    }
}

impl Drop for Handle {
    fn drop(&mut self) {
        unsafe { (self.free)(self.ptr.as_ptr()) };
    }
}

fn status(ok: c_int, message: &str) -> Result<(), String> {
    if ok == 0 {
        Err(message.to_string())
    } else {
        Ok(())
    }
}

pub struct Int64LinkedList(Handle);
impl Int64LinkedList {
    pub fn new() -> Result<Self, String> {
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_list_new() },
            coolbox_ds_list_free,
            "linked list",
        )?))
    }
    pub fn push_front(&mut self, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_list_push_front(self.0.as_ptr(), value) },
            "failed to push list front",
        )
    }
    pub fn push_back(&mut self, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_list_push_back(self.0.as_ptr(), value) },
            "failed to push list back",
        )
    }
    pub fn insert(&mut self, index: usize, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_list_insert(self.0.as_ptr(), index, value) },
            "list index out of range",
        )
    }
    pub fn pop_front(&mut self) -> Result<bool, String> {
        let mut removed = 0;
        status(
            unsafe { coolbox_ds_list_pop_front(self.0.as_ptr(), &mut removed) },
            "failed to pop list front",
        )?;
        Ok(removed != 0)
    }
    pub fn pop_back(&mut self) -> Result<bool, String> {
        let mut removed = 0;
        status(
            unsafe { coolbox_ds_list_pop_back(self.0.as_ptr(), &mut removed) },
            "failed to pop list back",
        )?;
        Ok(removed != 0)
    }
    pub fn remove_at(&mut self, index: usize) -> Result<bool, String> {
        let mut removed = 0;
        status(
            unsafe { coolbox_ds_list_remove_at(self.0.as_ptr(), index, &mut removed) },
            "list index out of range",
        )?;
        Ok(removed != 0)
    }
    pub fn remove(&mut self, value: i64) -> Result<bool, String> {
        let mut removed = 0;
        status(
            unsafe { coolbox_ds_list_remove_value(self.0.as_ptr(), value, &mut removed) },
            "failed to remove list item",
        )?;
        Ok(removed != 0)
    }
    pub fn at(&self, index: usize) -> Result<i64, String> {
        let mut value = 0;
        status(
            unsafe { coolbox_ds_list_at(self.0.as_ptr(), index, &mut value) },
            "list index out of range",
        )?;
        Ok(value)
    }
    pub fn front(&self) -> Result<i64, String> {
        let mut value = 0;
        status(
            unsafe { coolbox_ds_list_front(self.0.as_ptr(), &mut value) },
            "list is empty",
        )?;
        Ok(value)
    }
    pub fn back(&self) -> Result<i64, String> {
        let mut value = 0;
        status(
            unsafe { coolbox_ds_list_back(self.0.as_ptr(), &mut value) },
            "list is empty",
        )?;
        Ok(value)
    }
    pub fn contains(&self, value: i64) -> bool {
        unsafe { coolbox_ds_list_find(self.0.as_ptr(), value) != 0 }
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_ds_list_size(self.0.as_ptr()) }
    }
    pub fn is_empty(&self) -> bool {
        unsafe { coolbox_ds_list_empty(self.0.as_ptr()) != 0 }
    }
    pub fn clear(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_list_clear(self.0.as_ptr()) },
            "failed to clear list",
        )
    }
    pub fn reverse(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_list_reverse(self.0.as_ptr()) },
            "failed to reverse list",
        )
    }
    pub fn values(&self) -> Vec<i64> {
        let mut values = vec![0; self.len()];
        if !values.is_empty() {
            unsafe {
                coolbox_ds_list_values(self.0.as_ptr(), values.as_mut_ptr(), values.len());
            }
        }
        values
    }
}

pub struct Int64DoublyLinkedList(Handle);
impl Int64DoublyLinkedList {
    pub fn new() -> Result<Self, String> {
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_doubly_list_new() },
            coolbox_ds_doubly_list_free,
            "doubly linked list",
        )?))
    }
    pub fn push_front(&mut self, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_doubly_list_push_front(self.0.as_ptr(), value) },
            "failed to push doubly-linked-list front",
        )
    }
    pub fn push_back(&mut self, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_doubly_list_push_back(self.0.as_ptr(), value) },
            "failed to push doubly-linked-list back",
        )
    }
    pub fn insert(&mut self, index: usize, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_doubly_list_insert(self.0.as_ptr(), index, value) },
            "doubly linked list index out of range",
        )
    }
    pub fn pop_front(&mut self) -> Result<bool, String> {
        let mut removed = 0;
        status(
            unsafe { coolbox_ds_doubly_list_pop_front(self.0.as_ptr(), &mut removed) },
            "failed to pop list front",
        )?;
        Ok(removed != 0)
    }
    pub fn pop_back(&mut self) -> Result<bool, String> {
        let mut removed = 0;
        status(
            unsafe { coolbox_ds_doubly_list_pop_back(self.0.as_ptr(), &mut removed) },
            "failed to pop list back",
        )?;
        Ok(removed != 0)
    }
    pub fn remove_at(&mut self, index: usize) -> Result<bool, String> {
        let mut removed = 0;
        status(
            unsafe { coolbox_ds_doubly_list_remove_at(self.0.as_ptr(), index, &mut removed) },
            "list index out of range",
        )?;
        Ok(removed != 0)
    }
    pub fn at(&self, index: usize) -> Result<i64, String> {
        let mut value = 0;
        status(
            unsafe { coolbox_ds_doubly_list_at(self.0.as_ptr(), index, &mut value) },
            "list index out of range",
        )?;
        Ok(value)
    }
    pub fn front(&self) -> Result<i64, String> {
        let mut value = 0;
        status(
            unsafe { coolbox_ds_doubly_list_front(self.0.as_ptr(), &mut value) },
            "list is empty",
        )?;
        Ok(value)
    }
    pub fn back(&self) -> Result<i64, String> {
        let mut value = 0;
        status(
            unsafe { coolbox_ds_doubly_list_back(self.0.as_ptr(), &mut value) },
            "list is empty",
        )?;
        Ok(value)
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_ds_doubly_list_size(self.0.as_ptr()) }
    }
    pub fn is_empty(&self) -> bool {
        unsafe { coolbox_ds_doubly_list_empty(self.0.as_ptr()) != 0 }
    }
    pub fn clear(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_doubly_list_clear(self.0.as_ptr()) },
            "failed to clear list",
        )
    }
    pub fn values(&self) -> Result<Vec<i64>, String> {
        (0..self.len()).map(|i| self.at(i)).collect()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum HeapKind {
    Max,
    Min,
}

pub struct Int64Heap(Handle);
impl Int64Heap {
    pub fn new(kind: HeapKind) -> Result<Self, String> {
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_heap_new((kind == HeapKind::Min) as c_int) },
            coolbox_ds_heap_free,
            "binary heap",
        )?))
    }
    pub fn push(&mut self, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_heap_push(self.0.as_ptr(), value) },
            "failed to push heap value",
        )
    }
    pub fn pop(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_heap_pop(self.0.as_ptr()) },
            "heap is empty",
        )
    }
    pub fn top(&self) -> Result<i64, String> {
        let mut value = 0;
        status(
            unsafe { coolbox_ds_heap_top(self.0.as_ptr(), &mut value) },
            "heap is empty",
        )?;
        Ok(value)
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_ds_heap_size(self.0.as_ptr()) }
    }
    pub fn is_empty(&self) -> bool {
        unsafe { coolbox_ds_heap_empty(self.0.as_ptr()) != 0 }
    }
    pub fn clear(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_heap_clear(self.0.as_ptr()) },
            "failed to clear heap",
        )
    }
}

pub struct LegacyInt64Heap(Handle);
impl LegacyInt64Heap {
    pub fn new(kind: HeapKind) -> Result<Self, String> {
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_legacy_heap_new((kind == HeapKind::Min) as c_int) },
            coolbox_ds_legacy_heap_free,
            "legacy heap",
        )?))
    }
    pub fn push(&mut self, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_legacy_heap_push(self.0.as_ptr(), value) },
            "failed to push heap value",
        )
    }
    pub fn pop(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_legacy_heap_pop(self.0.as_ptr()) },
            "heap is empty",
        )
    }
    pub fn top(&self) -> Result<i64, String> {
        let mut value = 0;
        status(
            unsafe { coolbox_ds_legacy_heap_top(self.0.as_ptr(), &mut value) },
            "heap is empty",
        )?;
        Ok(value)
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_ds_legacy_heap_size(self.0.as_ptr()) }
    }
    pub fn is_empty(&self) -> bool {
        unsafe { coolbox_ds_legacy_heap_empty(self.0.as_ptr()) != 0 }
    }
    pub fn clear(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_legacy_heap_clear(self.0.as_ptr()) },
            "failed to clear heap",
        )
    }
}

pub struct IndexedPriorityQueue(Handle);
impl IndexedPriorityQueue {
    pub fn new() -> Result<Self, String> {
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_indexed_queue_new() },
            coolbox_ds_indexed_queue_free,
            "indexed priority queue",
        )?))
    }
    pub fn push(&mut self, key: i64, priority: f64) -> Result<(), String> {
        finite(priority)?;
        status(
            unsafe { coolbox_ds_indexed_queue_push(self.0.as_ptr(), key, priority) },
            "failed to insert priority",
        )
    }
    pub fn update(&mut self, key: i64, priority: f64) -> Result<(), String> {
        finite(priority)?;
        status(
            unsafe { coolbox_ds_indexed_queue_update(self.0.as_ptr(), key, priority) },
            "failed to update priority",
        )
    }
    pub fn pop(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_indexed_queue_pop(self.0.as_ptr()) },
            "priority queue is empty",
        )
    }
    pub fn top(&self) -> Result<(i64, f64), String> {
        let (mut key, mut priority) = (0, 0.0);
        status(
            unsafe { coolbox_ds_indexed_queue_top(self.0.as_ptr(), &mut key, &mut priority) },
            "priority queue is empty",
        )?;
        Ok((key, priority))
    }
    pub fn contains(&self, key: i64) -> bool {
        unsafe { coolbox_ds_indexed_queue_contains(self.0.as_ptr(), key) != 0 }
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_ds_indexed_queue_size(self.0.as_ptr()) }
    }
    pub fn is_empty(&self) -> bool {
        unsafe { coolbox_ds_indexed_queue_empty(self.0.as_ptr()) != 0 }
    }
    pub fn clear(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_indexed_queue_clear(self.0.as_ptr()) },
            "failed to clear priority queue",
        )
    }
}

pub struct Int64Map(Handle);
impl Int64Map {
    pub fn new(capacity: usize, concurrent: bool) -> Result<Self, String> {
        if capacity == 0 {
            return Err("map capacity must be positive".into());
        }
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_map_new(concurrent as c_int, capacity) },
            coolbox_ds_map_free,
            "hash map",
        )?))
    }
    pub fn insert(&mut self, key: i64, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_map_insert(self.0.as_ptr(), key, value) },
            "failed to insert map entry",
        )
    }
    pub fn get(&self, key: i64) -> Result<Option<i64>, String> {
        let (mut value, mut found) = (0, 0);
        status(
            unsafe { coolbox_ds_map_get(self.0.as_ptr(), key, &mut value, &mut found) },
            "failed to read map entry",
        )?;
        Ok((found != 0).then_some(value))
    }
    pub fn contains(&self, key: i64) -> bool {
        unsafe { coolbox_ds_map_contains(self.0.as_ptr(), key) != 0 }
    }
    pub fn remove(&mut self, key: i64) -> Result<bool, String> {
        let mut removed = 0;
        status(
            unsafe { coolbox_ds_map_remove(self.0.as_ptr(), key, &mut removed) },
            "failed to remove map entry",
        )?;
        Ok(removed != 0)
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_ds_map_size(self.0.as_ptr()) }
    }
    pub fn clear(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_map_clear(self.0.as_ptr()) },
            "failed to clear map",
        )
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum OrderedTreeKind {
    BinarySearch,
    RedBlack,
    Splay,
}
pub struct Int64OrderedTree(Handle);
impl Int64OrderedTree {
    pub fn new(kind: OrderedTreeKind) -> Result<Self, String> {
        let code = match kind {
            OrderedTreeKind::BinarySearch => 0,
            OrderedTreeKind::RedBlack => 1,
            OrderedTreeKind::Splay => 2,
        };
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_tree_new(code) },
            coolbox_ds_tree_free,
            "ordered tree",
        )?))
    }
    pub fn insert(&mut self, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_tree_insert(self.0.as_ptr(), value) },
            "failed to insert tree value",
        )
    }
    pub fn remove(&mut self, value: i64) -> Result<bool, String> {
        let mut removed = 0;
        status(
            unsafe { coolbox_ds_tree_remove(self.0.as_ptr(), value, &mut removed) },
            "failed to remove tree value",
        )?;
        Ok(removed != 0)
    }
    pub fn contains(&mut self, value: i64) -> bool {
        unsafe { coolbox_ds_tree_contains(self.0.as_ptr(), value) != 0 }
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_ds_tree_size(self.0.as_ptr()) }
    }
    pub fn is_empty(&self) -> bool {
        unsafe { coolbox_ds_tree_empty(self.0.as_ptr()) != 0 }
    }
    pub fn clear(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_tree_clear(self.0.as_ptr()) },
            "failed to clear tree",
        )
    }
    pub fn min(&self) -> Result<i64, String> {
        let mut value = 0;
        status(
            unsafe { coolbox_ds_tree_min(self.0.as_ptr(), &mut value) },
            "tree is empty",
        )?;
        Ok(value)
    }
    pub fn max(&self) -> Result<i64, String> {
        let mut value = 0;
        status(
            unsafe { coolbox_ds_tree_max(self.0.as_ptr(), &mut value) },
            "tree is empty",
        )?;
        Ok(value)
    }
    pub fn values(&self) -> Vec<i64> {
        let mut values = vec![0; self.len()];
        if !values.is_empty() {
            unsafe {
                coolbox_ds_tree_values(self.0.as_ptr(), values.as_mut_ptr(), values.len());
            }
        }
        values
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ProbabilisticFilterKind {
    Bloom,
    Egh,
}
pub struct Int64ProbabilisticFilter(Handle);
impl Int64ProbabilisticFilter {
    pub fn new(
        kind: ProbabilisticFilterKind,
        expected: usize,
        false_positive_rate: f64,
    ) -> Result<Self, String> {
        if expected == 0 {
            return Err("expected insertions must be positive".into());
        }
        if !(false_positive_rate > 0.0 && false_positive_rate < 1.0) {
            return Err("false-positive rate must be between 0 and 1".into());
        }
        let code = if kind == ProbabilisticFilterKind::Egh {
            1
        } else {
            0
        };
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_filter_new(code, expected, false_positive_rate) },
            coolbox_ds_filter_free,
            "probabilistic filter",
        )?))
    }
    pub fn insert(&mut self, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_filter_insert(self.0.as_ptr(), value) },
            "failed to insert filter value",
        )
    }
    pub fn contains(&self, value: i64) -> bool {
        unsafe { coolbox_ds_filter_contains(self.0.as_ptr(), value) != 0 }
    }
    pub fn insert_count(&self) -> usize {
        unsafe { coolbox_ds_filter_insert_count(self.0.as_ptr()) }
    }
    pub fn approximate_count(&self) -> usize {
        unsafe { coolbox_ds_filter_approximate_count(self.0.as_ptr()) }
    }
    pub fn estimated_false_positive_rate(&self) -> f64 {
        unsafe { coolbox_ds_filter_estimated_rate(self.0.as_ptr()) }
    }
    pub fn clear(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_filter_clear(self.0.as_ptr()) },
            "failed to clear filter",
        )
    }
}

pub const VEB_NONE: u64 = u64::MAX;
pub const MAX_VEB_UNIVERSE: u64 = 1 << 20;
pub struct VanEmdeBoasTree(Handle);
impl VanEmdeBoasTree {
    pub fn new(universe: u64) -> Result<Self, String> {
        if universe == 0 || universe > MAX_VEB_UNIVERSE {
            return Err(format!("universe must be in [1, {MAX_VEB_UNIVERSE}]"));
        }
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_veb_new(universe) },
            coolbox_ds_veb_free,
            "Van Emde Boas tree",
        )?))
    }
    pub fn insert(&mut self, value: u64) -> Result<(), String> {
        if value >= self.universe_size() {
            return Err("key outside universe".into());
        }
        status(
            unsafe { coolbox_ds_veb_insert(self.0.as_ptr(), value) },
            "failed to insert key",
        )
    }
    pub fn remove(&mut self, value: u64) -> Result<(), String> {
        if value >= self.universe_size() {
            return Err("key outside universe".into());
        }
        status(
            unsafe { coolbox_ds_veb_remove(self.0.as_ptr(), value) },
            "failed to remove key",
        )
    }
    pub fn contains(&self, value: u64) -> bool {
        unsafe { coolbox_ds_veb_contains(self.0.as_ptr(), value) != 0 }
    }
    pub fn min(&self) -> Option<u64> {
        let value = unsafe { coolbox_ds_veb_min(self.0.as_ptr()) };
        (value != VEB_NONE).then_some(value)
    }
    pub fn max(&self) -> Option<u64> {
        let value = unsafe { coolbox_ds_veb_max(self.0.as_ptr()) };
        (value != VEB_NONE).then_some(value)
    }
    pub fn successor(&self, value: u64) -> Option<u64> {
        let result = unsafe { coolbox_ds_veb_successor(self.0.as_ptr(), value) };
        (result != VEB_NONE).then_some(result)
    }
    pub fn predecessor(&self, value: u64) -> Option<u64> {
        let result = unsafe { coolbox_ds_veb_predecessor(self.0.as_ptr(), value) };
        (result != VEB_NONE).then_some(result)
    }
    pub fn universe_size(&self) -> u64 {
        unsafe { coolbox_ds_veb_universe_size(self.0.as_ptr()) }
    }
    pub fn is_empty(&self) -> bool {
        unsafe { coolbox_ds_veb_empty(self.0.as_ptr()) != 0 }
    }
}

pub struct Float64Array {
    handle: Handle,
    shape: Vec<usize>,
}
impl Float64Array {
    pub fn new(shape: &[usize]) -> Result<Self, String> {
        if shape.is_empty() || shape.len() > 4 || shape.iter().any(|&d| d == 0) {
            return Err("array rank must be 1..=4 and dimensions positive".into());
        }
        let size = shape
            .iter()
            .try_fold(1usize, |a, &b| a.checked_mul(b))
            .ok_or("array size overflow")?;
        let _ = size;
        Ok(Self {
            handle: Handle::new(
                unsafe { coolbox_ds_array_new(shape.len(), shape.as_ptr()) },
                coolbox_ds_array_free,
                "xarray",
            )?,
            shape: shape.to_vec(),
        })
    }
    pub fn shape(&self) -> &[usize] {
        &self.shape
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_ds_array_size(self.handle.as_ptr()) }
    }
    pub fn get(&self, indices: &[usize]) -> Result<f64, String> {
        self.validate_indices(indices)?;
        let mut value = 0.0;
        status(
            unsafe {
                coolbox_ds_array_get(
                    self.handle.as_ptr(),
                    indices.as_ptr(),
                    indices.len(),
                    &mut value,
                )
            },
            "xarray index out of range",
        )?;
        Ok(value)
    }
    pub fn set(&mut self, indices: &[usize], value: f64) -> Result<(), String> {
        self.validate_indices(indices)?;
        status(
            unsafe {
                coolbox_ds_array_set(self.handle.as_ptr(), indices.as_ptr(), indices.len(), value)
            },
            "xarray index out of range",
        )
    }
    fn validate_indices(&self, indices: &[usize]) -> Result<(), String> {
        if indices.len() != self.shape.len()
            || indices.iter().zip(&self.shape).any(|(&i, &d)| i >= d)
        {
            return Err("xarray index out of range".into());
        }
        Ok(())
    }
}

pub struct Int64FolioQueue(Handle);
impl Int64FolioQueue {
    pub fn new() -> Result<Self, String> {
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_queue_new() },
            coolbox_ds_queue_free,
            "folio queue",
        )?))
    }
    pub fn enqueue(&mut self, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_queue_enqueue(self.0.as_ptr(), value) },
            "failed to enqueue",
        )
    }
    pub fn enqueue_batch(&mut self, values: &[i64]) -> Result<(), String> {
        status(
            unsafe {
                coolbox_ds_queue_enqueue_batch(self.0.as_ptr(), values.as_ptr(), values.len())
            },
            "failed to enqueue batch",
        )
    }
    pub fn try_dequeue(&mut self) -> Result<Option<i64>, String> {
        let (mut value, mut found) = (0, 0);
        status(
            unsafe { coolbox_ds_queue_try_dequeue(self.0.as_ptr(), &mut value, &mut found) },
            "failed to dequeue",
        )?;
        Ok((found != 0).then_some(value))
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_ds_queue_size(self.0.as_ptr()) }
    }
    pub fn is_empty(&self) -> bool {
        unsafe { coolbox_ds_queue_empty(self.0.as_ptr()) != 0 }
    }
}

pub struct Int64ConcurrentLinkedList(Handle);
impl Int64ConcurrentLinkedList {
    pub fn new() -> Result<Self, String> {
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_concurrent_list_new() },
            coolbox_ds_concurrent_list_free,
            "concurrent linked list",
        )?))
    }
    pub fn push_front(&mut self, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_concurrent_list_push_front(self.0.as_ptr(), value) },
            "failed to push list front",
        )
    }
    pub fn push_back(&mut self, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_concurrent_list_push_back(self.0.as_ptr(), value) },
            "failed to push list back",
        )
    }
    pub fn pop_front(&mut self) -> Result<Option<i64>, String> {
        let (mut value, mut found) = (0, 0);
        status(
            unsafe {
                coolbox_ds_concurrent_list_pop_front(self.0.as_ptr(), &mut value, &mut found)
            },
            "failed to pop list front",
        )?;
        Ok((found != 0).then_some(value))
    }
    pub fn remove(&mut self, value: i64) -> Result<bool, String> {
        let mut removed = 0;
        status(
            unsafe { coolbox_ds_concurrent_list_remove(self.0.as_ptr(), value, &mut removed) },
            "failed to remove list value",
        )?;
        Ok(removed != 0)
    }
    pub fn contains(&self, value: i64) -> bool {
        unsafe { coolbox_ds_concurrent_list_contains(self.0.as_ptr(), value) != 0 }
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_ds_concurrent_list_size(self.0.as_ptr()) }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Int64Entry {
    pub key: i64,
    pub value: i64,
}
pub struct Int64BPlusTree(Handle);
impl Int64BPlusTree {
    pub fn new() -> Result<Self, String> {
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_bplus_tree_new() },
            coolbox_ds_bplus_tree_free,
            "B+ tree",
        )?))
    }
    pub fn insert(&mut self, key: i64, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_bplus_tree_insert(self.0.as_ptr(), key, value) },
            "failed to insert B+ tree entry",
        )
    }
    pub fn get(&self, key: i64) -> Result<Option<i64>, String> {
        let (mut value, mut found) = (0, 0);
        status(
            unsafe { coolbox_ds_bplus_tree_get(self.0.as_ptr(), key, &mut value, &mut found) },
            "failed to search B+ tree",
        )?;
        Ok((found != 0).then_some(value))
    }
    pub fn remove(&mut self, key: i64) -> Result<bool, String> {
        let mut removed = 0;
        status(
            unsafe { coolbox_ds_bplus_tree_remove(self.0.as_ptr(), key, &mut removed) },
            "failed to remove B+ tree entry",
        )?;
        Ok(removed != 0)
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_ds_bplus_tree_size(self.0.as_ptr()) }
    }
    pub fn clear(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_bplus_tree_clear(self.0.as_ptr()) },
            "failed to clear B+ tree",
        )
    }
    pub fn entries(&self) -> Result<Vec<Int64Entry>, String> {
        self.range_inner(false, 0, 0)
    }
    pub fn range(&self, min: i64, max: i64) -> Result<Vec<Int64Entry>, String> {
        if min > max {
            return Err("range minimum exceeds maximum".into());
        }
        self.range_inner(true, min, max)
    }
    fn range_inner(&self, range: bool, min: i64, max: i64) -> Result<Vec<Int64Entry>, String> {
        let (mut count, flag) = (0, range as c_int);
        status(
            unsafe {
                coolbox_ds_bplus_tree_entries(
                    self.0.as_ptr(),
                    flag,
                    min,
                    max,
                    &mut count,
                    ptr::null_mut(),
                    ptr::null_mut(),
                    0,
                )
            },
            "failed to enumerate B+ tree",
        )?;
        let (mut keys, mut values) = (vec![0; count], vec![0; count]);
        if count > 0 {
            status(
                unsafe {
                    coolbox_ds_bplus_tree_entries(
                        self.0.as_ptr(),
                        flag,
                        min,
                        max,
                        &mut count,
                        keys.as_mut_ptr(),
                        values.as_mut_ptr(),
                        count,
                    )
                },
                "failed to read B+ tree entries",
            )?;
        }
        Ok(keys
            .into_iter()
            .zip(values)
            .map(|(key, value)| Int64Entry { key, value })
            .collect())
    }
}

pub struct StringRadixTree(Handle);
impl StringRadixTree {
    pub fn new() -> Result<Self, String> {
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_radix_tree_new() },
            coolbox_ds_radix_tree_free,
            "radix tree",
        )?))
    }
    pub fn insert(&mut self, key: &str, value: i64) -> Result<(), String> {
        let key = CString::new(key).map_err(|_| "radix keys cannot contain NUL bytes")?;
        status(
            unsafe { coolbox_ds_radix_tree_insert(self.0.as_ptr(), key.as_ptr(), value) },
            "failed to insert radix entry",
        )
    }
    pub fn get(&self, key: &str) -> Result<Option<i64>, String> {
        let key = CString::new(key).map_err(|_| "radix keys cannot contain NUL bytes")?;
        let (mut value, mut found) = (0, 0);
        status(
            unsafe {
                coolbox_ds_radix_tree_get(self.0.as_ptr(), key.as_ptr(), &mut value, &mut found)
            },
            "failed to search radix tree",
        )?;
        Ok((found != 0).then_some(value))
    }
    pub fn remove(&mut self, key: &str) -> Result<bool, String> {
        let key = CString::new(key).map_err(|_| "radix keys cannot contain NUL bytes")?;
        let mut removed = 0;
        status(
            unsafe { coolbox_ds_radix_tree_remove(self.0.as_ptr(), key.as_ptr(), &mut removed) },
            "failed to remove radix entry",
        )?;
        Ok(removed != 0)
    }
}

pub struct Int64LsmTree(Handle);
impl Int64LsmTree {
    pub fn new(threshold: usize) -> Result<Self, String> {
        if threshold == 0 {
            return Err("memtable threshold must be positive".into());
        }
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_lsm_new(threshold) },
            coolbox_ds_lsm_free,
            "LSM tree",
        )?))
    }
    pub fn put(&mut self, key: i64, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_lsm_put(self.0.as_ptr(), key, value) },
            "failed to write LSM entry",
        )
    }
    pub fn get(&self, key: i64) -> Result<Option<i64>, String> {
        let (mut value, mut found) = (0, 0);
        status(
            unsafe { coolbox_ds_lsm_get(self.0.as_ptr(), key, &mut value, &mut found) },
            "failed to read LSM entry",
        )?;
        Ok((found != 0).then_some(value))
    }
    pub fn remove(&mut self, key: i64) -> Result<bool, String> {
        let mut removed = 0;
        status(
            unsafe { coolbox_ds_lsm_remove(self.0.as_ptr(), key, &mut removed) },
            "failed to remove LSM entry",
        )?;
        Ok(removed != 0)
    }
    pub fn contains(&self, key: i64) -> bool {
        unsafe { coolbox_ds_lsm_contains(self.0.as_ptr(), key) != 0 }
    }
    pub fn flush(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_lsm_flush(self.0.as_ptr()) },
            "failed to flush LSM tree",
        )
    }
    pub fn compact(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_lsm_compact(self.0.as_ptr()) },
            "failed to compact LSM tree",
        )
    }
    pub fn clear(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_lsm_clear(self.0.as_ptr()) },
            "failed to clear LSM tree",
        )
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_ds_lsm_size(self.0.as_ptr()) }
    }
    pub fn memtable_len(&self) -> usize {
        unsafe { coolbox_ds_lsm_memtable_size(self.0.as_ptr()) }
    }
    pub fn sstable_count(&self) -> usize {
        unsafe { coolbox_ds_lsm_sstable_count(self.0.as_ptr()) }
    }
}

pub struct Int64MapleTree(Handle);
impl Int64MapleTree {
    pub fn new() -> Result<Self, String> {
        Ok(Self(Handle::new(
            unsafe { coolbox_ds_maple_new() },
            coolbox_ds_maple_free,
            "Maple tree",
        )?))
    }
    pub fn insert(&mut self, key: i64, value: i64) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_maple_insert(self.0.as_ptr(), key, value) },
            "failed to insert Maple entry",
        )
    }
    pub fn get(&self, key: i64) -> Result<Option<i64>, String> {
        let (mut value, mut found) = (0, 0);
        status(
            unsafe { coolbox_ds_maple_get(self.0.as_ptr(), key, &mut value, &mut found) },
            "failed to read Maple entry",
        )?;
        Ok((found != 0).then_some(value))
    }
    pub fn remove(&mut self, key: i64) -> Result<bool, String> {
        let mut removed = 0;
        status(
            unsafe { coolbox_ds_maple_remove(self.0.as_ptr(), key, &mut removed) },
            "failed to remove Maple entry",
        )?;
        Ok(removed != 0)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CoverTreeKind {
    Standard,
    Faster,
}
#[derive(Debug, Clone, PartialEq)]
pub struct MetricNeighbor {
    pub index: usize,
    pub item: Vec<f64>,
    pub distance: f64,
}
pub struct Float64CoverTree {
    handle: Handle,
    dimensions: usize,
}
impl Float64CoverTree {
    pub fn new(kind: CoverTreeKind, dimensions: usize) -> Result<Self, String> {
        if dimensions == 0 {
            return Err("dimensions must be positive".into());
        }
        Ok(Self {
            handle: Handle::new(
                unsafe {
                    coolbox_ds_cover_tree_new((kind == CoverTreeKind::Faster) as c_int, dimensions)
                },
                coolbox_ds_cover_tree_free,
                "cover tree",
            )?,
            dimensions,
        })
    }
    pub fn build(&mut self, items: &[Vec<f64>]) -> Result<(), String> {
        let flat = self.flatten(items)?;
        status(
            unsafe {
                coolbox_ds_cover_tree_build(
                    self.handle.as_ptr(),
                    flat.as_ptr(),
                    items.len(),
                    self.dimensions,
                )
            },
            "failed to build cover tree",
        )
    }
    pub fn insert(&mut self, item: &[f64]) -> Result<(), String> {
        self.validate_item(item)?;
        status(
            unsafe {
                coolbox_ds_cover_tree_insert(self.handle.as_ptr(), item.as_ptr(), self.dimensions)
            },
            "failed to insert cover-tree item",
        )
    }
    pub fn clear(&mut self) -> Result<(), String> {
        status(
            unsafe { coolbox_ds_cover_tree_clear(self.handle.as_ptr()) },
            "failed to clear cover tree",
        )
    }
    pub fn len(&self) -> usize {
        unsafe { coolbox_ds_cover_tree_size(self.handle.as_ptr()) }
    }
    pub fn validate_invariants(&self) -> bool {
        unsafe { coolbox_ds_cover_tree_validate(self.handle.as_ptr()) != 0 }
    }
    pub fn k_nearest(&self, query: &[f64], k: usize) -> Result<Vec<MetricNeighbor>, String> {
        self.query(query, k, 0.0, false)
    }
    pub fn radius_search(&self, query: &[f64], radius: f64) -> Result<Vec<MetricNeighbor>, String> {
        if !radius.is_finite() || radius < 0.0 {
            return Err("radius must be finite and non-negative".into());
        }
        self.query(query, 0, radius, true)
    }
    fn validate_item(&self, item: &[f64]) -> Result<(), String> {
        if item.len() != self.dimensions {
            return Err(format!("expected {} dimensions", self.dimensions));
        }
        if item.iter().any(|v| !v.is_finite()) {
            return Err("coordinates must be finite".into());
        }
        Ok(())
    }
    fn flatten(&self, items: &[Vec<f64>]) -> Result<Vec<f64>, String> {
        let mut flat = Vec::with_capacity(items.len() * self.dimensions);
        for item in items {
            self.validate_item(item)?;
            flat.extend_from_slice(item)
        }
        Ok(flat)
    }
    fn query(
        &self,
        query: &[f64],
        k: usize,
        radius: f64,
        use_radius: bool,
    ) -> Result<Vec<MetricNeighbor>, String> {
        self.validate_item(query)?;
        let flag = use_radius as c_int;
        let (mut count, mut ids, mut distances, mut points) =
            (0, Vec::<usize>::new(), Vec::<f64>::new(), Vec::<f64>::new());
        status(
            unsafe {
                coolbox_ds_cover_tree_query(
                    self.handle.as_ptr(),
                    query.as_ptr(),
                    self.dimensions,
                    k,
                    radius,
                    flag,
                    &mut count,
                    ptr::null_mut(),
                    ptr::null_mut(),
                    ptr::null_mut(),
                    0,
                )
            },
            "cover-tree query failed",
        )?;
        ids.resize(count, 0);
        distances.resize(count, 0.0);
        points.resize(count * self.dimensions, 0.0);
        if count > 0 {
            status(
                unsafe {
                    coolbox_ds_cover_tree_query(
                        self.handle.as_ptr(),
                        query.as_ptr(),
                        self.dimensions,
                        k,
                        radius,
                        flag,
                        &mut count,
                        ids.as_mut_ptr(),
                        distances.as_mut_ptr(),
                        points.as_mut_ptr(),
                        count,
                    )
                },
                "failed to read cover-tree results",
            )?;
        }
        Ok((0..count)
            .map(|i| MetricNeighbor {
                index: ids[i],
                item: points[i * self.dimensions..(i + 1) * self.dimensions].to_vec(),
                distance: distances[i],
            })
            .collect())
    }
}

fn finite(value: f64) -> Result<(), String> {
    if value.is_finite() {
        Ok(())
    } else {
        Err("value must be finite".into())
    }
}
