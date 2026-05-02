#include "../headers/splay_tree.h"

#include <string>
#include <utility>

namespace data_structures {

template<typename T>
void SplayTree<T>::insert(const T& value) {
    if (!root_) {
        root_ = std::make_shared<Node>(value);
        ++size_;
        return;
    }

    auto current = root_;
    std::shared_ptr<Node> parent;
    while (current) {
        parent = current;
        if (value < current->data) {
            current = current->left;
        } else if (value > current->data) {
            current = current->right;
        } else {
            splay(current);
            return;
        }
    }

    auto node = std::make_shared<Node>(value);
    node->parent = parent;
    if (value < parent->data) {
        parent->left = node;
    } else {
        parent->right = node;
    }

    ++size_;
    splay(node);
}

template<typename T>
bool SplayTree<T>::remove(const T& value) {
    if (!search(value)) {
        return false;
    }

    auto target = root_;
    auto left_subtree = target->left;
    auto right_subtree = target->right;

    if (left_subtree) {
        left_subtree->parent.reset();
    }
    if (right_subtree) {
        right_subtree->parent.reset();
    }

    target->left.reset();
    target->right.reset();
    target->parent.reset();

    if (!left_subtree) {
        root_ = right_subtree;
    } else {
        root_ = left_subtree;
        auto max_left = left_subtree;
        while (max_left->right) {
            max_left = max_left->right;
        }
        splay(max_left);
        root_->right = right_subtree;
        if (right_subtree) {
            right_subtree->parent = root_;
        }
    }

    --size_;
    return true;
}

template<typename T>
bool SplayTree<T>::search(const T& value) {
    auto node = search_helper(root_, value);
    if (node) {
        splay(node);
        return true;
    }
    return false;
}

template<typename T>
void SplayTree<T>::inorder_traversal(std::function<void(const T&)> callback) const {
    inorder_helper(root_, std::move(callback));
}

template<typename T>
void SplayTree<T>::clear() {
    std::function<void(std::shared_ptr<Node>)> clear_node = [&](std::shared_ptr<Node> node) {
        if (!node) {
            return;
        }
        auto left = node->left;
        auto right = node->right;
        node->left.reset();
        node->right.reset();
        node->parent.reset();
        clear_node(left);
        clear_node(right);
    };

    clear_node(root_);
    root_.reset();
    size_ = 0;
}

template<typename T>
void SplayTree<T>::splay(std::shared_ptr<Node> node) {
    while (node && node->parent) {
        auto parent = node->parent;
        auto grandparent = parent->parent;

        if (!grandparent) {
            if (node == parent->left) {
                rotate_right(parent);
            } else {
                rotate_left(parent);
            }
        } else if (node == parent->left && parent == grandparent->left) {
            rotate_right(grandparent);
            rotate_right(parent);
        } else if (node == parent->right && parent == grandparent->right) {
            rotate_left(grandparent);
            rotate_left(parent);
        } else if (node == parent->right && parent == grandparent->left) {
            rotate_left(parent);
            rotate_right(grandparent);
        } else {
            rotate_right(parent);
            rotate_left(grandparent);
        }
    }
}

template<typename T>
void SplayTree<T>::rotate_left(std::shared_ptr<Node> node) {
    if (!node || !node->right) {
        return;
    }

    auto pivot = node->right;
    node->right = pivot->left;
    if (pivot->left) {
        pivot->left->parent = node;
    }

    pivot->parent = node->parent;
    if (!node->parent) {
        root_ = pivot;
    } else if (node == node->parent->left) {
        node->parent->left = pivot;
    } else {
        node->parent->right = pivot;
    }

    pivot->left = node;
    node->parent = pivot;
}

template<typename T>
void SplayTree<T>::rotate_right(std::shared_ptr<Node> node) {
    if (!node || !node->left) {
        return;
    }

    auto pivot = node->left;
    node->left = pivot->right;
    if (pivot->right) {
        pivot->right->parent = node;
    }

    pivot->parent = node->parent;
    if (!node->parent) {
        root_ = pivot;
    } else if (node == node->parent->right) {
        node->parent->right = pivot;
    } else {
        node->parent->left = pivot;
    }

    pivot->right = node;
    node->parent = pivot;
}

template<typename T>
std::shared_ptr<typename SplayTree<T>::Node> SplayTree<T>::find_min(std::shared_ptr<Node> node) const {
    while (node && node->left) {
        node = node->left;
    }
    return node;
}

template<typename T>
std::shared_ptr<typename SplayTree<T>::Node> SplayTree<T>::search_helper(std::shared_ptr<Node> node, const T& value) {
    std::shared_ptr<Node> last;
    while (node) {
        last = node;
        if (value < node->data) {
            node = node->left;
        } else if (value > node->data) {
            node = node->right;
        } else {
            return node;
        }
    }

    if (last) {
        splay(last);
    }
    return nullptr;
}

template<typename T>
void SplayTree<T>::inorder_helper(std::shared_ptr<Node> node, std::function<void(const T&)> callback) const {
    if (!node) {
        return;
    }
    inorder_helper(node->left, callback);
    callback(node->data);
    inorder_helper(node->right, callback);
}

template class SplayTree<int>;
template class SplayTree<double>;
template class SplayTree<std::string>;

} // namespace data_structures
