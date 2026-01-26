#include "IO/data_structures/binary_search_tree.h"
#include <stdexcept>

namespace data_structures {

template<typename T>
void BinarySearchTree<T>::insert(const T& value) {
    root_ = insert_helper(root_, value);
}

template<typename T>
std::shared_ptr<typename BinarySearchTree<T>::Node> 
BinarySearchTree<T>::insert_helper(std::shared_ptr<Node> node, const T& value) {
    if (!node) {
        size_++;
        return std::make_shared<Node>(value);
    }
    
    if (value < node->data) {
        node->left = insert_helper(node->left, value);
    } else if (value > node->data) {
        node->right = insert_helper(node->right, value);
    }
    // If equal, don't insert duplicate
    
    return node;
}

template<typename T>
bool BinarySearchTree<T>::remove(const T& value) {
    size_t old_size = size_;
    root_ = remove_helper(root_, value);
    return size_ < old_size;
}

template<typename T>
std::shared_ptr<typename BinarySearchTree<T>::Node> 
BinarySearchTree<T>::remove_helper(std::shared_ptr<Node> node, const T& value) {
    if (!node) return nullptr;
    
    if (value < node->data) {
        node->left = remove_helper(node->left, value);
    } else if (value > node->data) {
        node->right = remove_helper(node->right, value);
    } else {
        // Node found
        if (!node->left && !node->right) {
            // Leaf node
            size_--;
            return nullptr;
        } else if (!node->left) {
            // Only right child
            size_--;
            return node->right;
        } else if (!node->right) {
            // Only left child
            size_--;
            return node->left;
        } else {
            // Two children: get inorder successor
            auto min_right = find_min(node->right);
            node->data = min_right->data;
            node->right = remove_helper(node->right, min_right->data);
        }
    }
    
    return node;
}

template<typename T>
bool BinarySearchTree<T>::search(const T& value) const {
    return search_helper(root_, value);
}

template<typename T>
bool BinarySearchTree<T>::search_helper(std::shared_ptr<Node> node, const T& value) const {
    if (!node) return false;
    
    if (value == node->data) return true;
    if (value < node->data) return search_helper(node->left, value);
    return search_helper(node->right, value);
}

template<typename T>
void BinarySearchTree<T>::inorder_traversal(std::function<void(const T&)> callback) const {
    inorder_helper(root_, callback);
}

template<typename T>
void BinarySearchTree<T>::inorder_helper(std::shared_ptr<Node> node, std::function<void(const T&)> callback) const {
    if (!node) return;
    inorder_helper(node->left, callback);
    callback(node->data);
    inorder_helper(node->right, callback);
}

template<typename T>
void BinarySearchTree<T>::preorder_traversal(std::function<void(const T&)> callback) const {
    preorder_helper(root_, callback);
}

template<typename T>
void BinarySearchTree<T>::preorder_helper(std::shared_ptr<Node> node, std::function<void(const T&)> callback) const {
    if (!node) return;
    callback(node->data);
    preorder_helper(node->left, callback);
    preorder_helper(node->right, callback);
}

template<typename T>
void BinarySearchTree<T>::postorder_traversal(std::function<void(const T&)> callback) const {
    postorder_helper(root_, callback);
}

template<typename T>
void BinarySearchTree<T>::postorder_helper(std::shared_ptr<Node> node, std::function<void(const T&)> callback) const {
    if (!node) return;
    postorder_helper(node->left, callback);
    postorder_helper(node->right, callback);
    callback(node->data);
}

template<typename T>
void BinarySearchTree<T>::clear() {
    root_ = nullptr;
    size_ = 0;
}

template<typename T>
T BinarySearchTree<T>::min() const {
    if (!root_) throw std::runtime_error("Tree is empty");
    return find_min(root_)->data;
}

template<typename T>
T BinarySearchTree<T>::max() const {
    if (!root_) throw std::runtime_error("Tree is empty");
    return find_max(root_)->data;
}

template<typename T>
std::shared_ptr<typename BinarySearchTree<T>::Node> 
BinarySearchTree<T>::find_min(std::shared_ptr<Node> node) const {
    while (node->left) node = node->left;
    return node;
}

template<typename T>
std::shared_ptr<typename BinarySearchTree<T>::Node> 
BinarySearchTree<T>::find_max(std::shared_ptr<Node> node) const {
    while (node->right) node = node->right;
    return node;
}

// Explicit template instantiations
template class BinarySearchTree<int>;
template class BinarySearchTree<double>;
template class BinarySearchTree<std::string>;

} // namespace data_structures
