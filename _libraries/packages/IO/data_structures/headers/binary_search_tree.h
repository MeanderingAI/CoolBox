#ifndef DATA_STRUCTURES_BINARY_SEARCH_TREE_H
#define DATA_STRUCTURES_BINARY_SEARCH_TREE_H

#include <memory>
#include <functional>
#include <vector>

namespace data_structures {

template<typename T>
class BinarySearchTree {
public:
    struct Node {
        T data;
        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;
        
        explicit Node(const T& value) : data(value), left(nullptr), right(nullptr) {}
    };
    
    BinarySearchTree() : root_(nullptr), size_(0) {}
    
    void insert(const T& value);
    bool remove(const T& value);
    bool search(const T& value) const;
    
    void inorder_traversal(std::function<void(const T&)> callback) const;
    void preorder_traversal(std::function<void(const T&)> callback) const;
    void postorder_traversal(std::function<void(const T&)> callback) const;
    
    size_t size() const { return size_; }
    bool empty() const { return root_ == nullptr; }
    void clear();
    
    T min() const;
    T max() const;
    
private:
    std::shared_ptr<Node> root_;
    size_t size_;
    
    std::shared_ptr<Node> insert_helper(std::shared_ptr<Node> node, const T& value);
    std::shared_ptr<Node> remove_helper(std::shared_ptr<Node> node, const T& value);
    bool search_helper(std::shared_ptr<Node> node, const T& value) const;
    
    void inorder_helper(std::shared_ptr<Node> node, std::function<void(const T&)> callback) const;
    void preorder_helper(std::shared_ptr<Node> node, std::function<void(const T&)> callback) const;
    void postorder_helper(std::shared_ptr<Node> node, std::function<void(const T&)> callback) const;
    
    std::shared_ptr<Node> find_min(std::shared_ptr<Node> node) const;
    std::shared_ptr<Node> find_max(std::shared_ptr<Node> node) const;
};

} // namespace data_structures

#endif // DATA_STRUCTURES_BINARY_SEARCH_TREE_H
