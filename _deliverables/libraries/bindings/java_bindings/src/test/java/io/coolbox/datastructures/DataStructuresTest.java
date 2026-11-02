package io.coolbox.datastructures;
import static org.junit.jupiter.api.Assertions.*;
import java.util.OptionalLong;
import org.junit.jupiter.api.Test;
class DataStructuresTest {
 @Test void collectionsAndTreesWork(){
  try(Int64LinkedList l=new Int64LinkedList()){l.pushBack(2);l.pushFront(1);l.insert(2,3);assertArrayEquals(new long[]{1,2,3},l.values());assertEquals(OptionalLong.of(1),l.front());assertTrue(l.contains(2));assertTrue(l.removeAt(1));l.reverse();assertEquals(3,l.at(0));assertTrue(l.popBack());}
  try(Int64DoublyLinkedList l=new Int64DoublyLinkedList()){l.pushBack(4);l.pushFront(3);assertArrayEquals(new long[]{3,4},l.values());assertEquals(OptionalLong.of(4),l.back());}
  try(Int64Heap h=new Int64Heap()){h.push(3);h.push(8);assertEquals(8,h.pop());assertEquals(3,h.top());}
  try(LegacyInt64Heap h=new LegacyInt64Heap(HeapKind.MIN)){h.push(5);h.push(1);assertEquals(1,h.pop());}
  try(IndexedPriorityQueue q=new IndexedPriorityQueue()){q.push(9,4);q.push(2,1);assertEquals(2,q.top().key());assertTrue(q.contains(9));assertEquals(2,q.pop().key());}
  try(Int64Map m=new Int64Map()){m.put(1,8);assertEquals(OptionalLong.of(8),m.get(1));assertTrue(m.remove(1));}
  try(Int64ConcurrentMap m=new Int64ConcurrentMap()){m.put(2,7);assertTrue(m.contains(2));}
  try(Int64Tree t=new Int64Tree(TreeKind.RED_BLACK)){t.insert(4);t.insert(2);assertEquals(OptionalLong.of(2),t.min());assertArrayEquals(new long[]{2,4},t.values());}
  try(BloomFilter f=new BloomFilter()){f.insert(12);assertTrue(f.mightContain(12));assertEquals(1,f.insertCount());}
  try(EGHFilter f=new EGHFilter()){f.insert(12);assertTrue(f.mightContain(12));}
  try(VanEmdeBoasTree t=new VanEmdeBoasTree(64)){t.insert(7);t.insert(9);assertEquals(OptionalLong.of(9),t.successor(7));}
  try(Float64Array a=new Float64Array(2,3)){a.set(2.5,1,2);assertEquals(2.5,a.get(1,2));}
  try(Int64FolioQueue q=new Int64FolioQueue()){q.enqueueBatch(2,3);assertEquals(OptionalLong.of(2),q.tryDequeue());}
  try(Int64ConcurrentLinkedList l=new Int64ConcurrentLinkedList()){l.pushBack(7);assertEquals(OptionalLong.of(7),l.popFront());}
  try(Int64BPlusTree t=new Int64BPlusTree()){t.insert(3,6);assertEquals(OptionalLong.of(6),t.get(3));assertEquals(1,t.items().size());}
  try(StringRadixTree t=new StringRadixTree()){t.insert("abc",4);assertEquals(OptionalLong.of(4),t.get("abc"));assertThrows(IllegalArgumentException.class,()->t.get("a\0b"));}
  try(Int64LsmTree t=new Int64LsmTree(2)){t.set(1,5);assertEquals(OptionalLong.of(5),t.get(1));t.flush();}
  try(Int64MapleTree t=new Int64MapleTree()){t.insert(8,4);assertEquals(OptionalLong.of(4),t.get(8));}
  try(Float64CoverTree t=new Float64CoverTree(2)){t.build(new double[][]{{0,0},{3,4}});assertTrue(t.validate());assertEquals(0,t.query(new double[]{0,0}).get(0).index());}
 }
}
