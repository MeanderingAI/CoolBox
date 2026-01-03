#ifndef DATA_STRUCTURES_SPLAY_TREE_H
#define DATA_STRUCTURES_SPLAY_TREE_H

#include <memory>
#include <functional>

namespace data_structures {

// Splay Tree - self-adjusting binary search tree
template<typename T>
class SplayTree {
public:
    struct Node {
        T data;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
        std::shared_ptr<Node> parent;
        
        explicit Node(const T& value) 
            : data(value), left(nullptr), right(nullptr), parent(nullptr) {}
    };
    
    SplayTree() : root_(nullptr), size_(0) {}
    
    void insert(const T& value);
    bool remove(const T& value);
    bool search(const T& value);
    
    void inorder_traversal(std::function<void(const T&)> callback) const;
    
    size_t size() const { return size_; }
    bool empty() const { return root_ == nullptr; }
    void clear();
    
private:
    std::shared_ptr<Node> root_;
    size_t size_;
    
    void splay(std::shared_ptr<Node> node);
    void rotate_left(std::shared_ptr<Node> node);
    void rotate_right(std::shared_ptr<Node> node);
    
    std::shared_ptr<Node> find_min(std::shared_ptr<Node> node) const;
    std::shared_ptr<Node> search_helper(std::shared_ptr<Node> node, const T& value);
    void inorder_helper(std::shared_ptr<Node> node, std::function<void(const T&)> callback) const;
};

} // namespace data_structures

#endif // DATA_STRUCTURES_SPLAY_TREE_H
