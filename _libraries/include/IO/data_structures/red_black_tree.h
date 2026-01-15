#ifndef DATA_STRUCTURES_RED_BLACK_TREE_H
#define DATA_STRUCTURES_RED_BLACK_TREE_H

#include <memory>
#include <functional>

namespace data_structures {

template<typename T>
class RedBlackTree {
public:
    enum Color { RED, BLACK };
    
    struct Node {
        T data;
        Color color;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
        std::shared_ptr<Node> parent;
        
        explicit Node(const T& value) 
            : data(value), color(RED), left(nullptr), right(nullptr), parent(nullptr) {}
    };
    
    RedBlackTree() : root_(nullptr), size_(0) {}
    
    void insert(const T& value);
    bool remove(const T& value);
    bool search(const T& value) const;
    
    void inorder_traversal(std::function<void(const T&)> callback) const;
    
    size_t size() const { return size_; }
    bool empty() const { return root_ == nullptr; }
    void clear();
    
    T min() const;
    T max() const;
    
private:
    std::shared_ptr<Node> root_;
    size_t size_;
    
    void insert_fixup(std::shared_ptr<Node> node);
    void remove_fixup(std::shared_ptr<Node> node, std::shared_ptr<Node> parent);
    
    void rotate_left(std::shared_ptr<Node> node);
    void rotate_right(std::shared_ptr<Node> node);
    
    std::shared_ptr<Node> find_min(std::shared_ptr<Node> node) const;
    std::shared_ptr<Node> find_max(std::shared_ptr<Node> node) const;
    std::shared_ptr<Node> search_helper(std::shared_ptr<Node> node, const T& value) const;
    
    void inorder_helper(std::shared_ptr<Node> node, std::function<void(const T&)> callback) const;
    void transplant(std::shared_ptr<Node> u, std::shared_ptr<Node> v);
};

} // namespace data_structures

#endif // DATA_STRUCTURES_RED_BLACK_TREE_H
