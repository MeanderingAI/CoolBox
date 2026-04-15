#include "../headers/red_black_tree.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace data_structures {

template<typename T>
void RedBlackTree<T>::insert(const T& value) {
    auto parent = std::shared_ptr<Node>(nullptr);
    auto current = root_;

    while (current) {
        parent = current;
        if (value < current->data) {
            current = current->left;
        } else if (value > current->data) {
            current = current->right;
        } else {
            return;
        }
    }

    auto node = std::make_shared<Node>(value);
    node->parent = parent;

    if (!parent) {
        root_ = node;
    } else if (value < parent->data) {
        parent->left = node;
    } else {
        parent->right = node;
    }

    ++size_;
    insert_fixup(node);
}

template<typename T>
bool RedBlackTree<T>::remove(const T& value) {
    auto node = search_helper(root_, value);
    if (!node) {
        return false;
    }

    auto replacement = node;
    auto replacement_original_color = replacement->color;
    auto fixup_node = std::shared_ptr<Node>(nullptr);
    auto fixup_parent = std::shared_ptr<Node>(nullptr);

    if (!node->left) {
        fixup_node = node->right;
        fixup_parent = node->parent;
        transplant(node, node->right);
    } else if (!node->right) {
        fixup_node = node->left;
        fixup_parent = node->parent;
        transplant(node, node->left);
    } else {
        replacement = find_min(node->right);
        replacement_original_color = replacement->color;
        fixup_node = replacement->right;

        if (replacement->parent == node) {
            fixup_parent = replacement;
            if (fixup_node) {
                fixup_node->parent = replacement;
            }
        } else {
            fixup_parent = replacement->parent;
            transplant(replacement, replacement->right);
            replacement->right = node->right;
            if (replacement->right) {
                replacement->right->parent = replacement;
            }
        }

        transplant(node, replacement);
        replacement->left = node->left;
        if (replacement->left) {
            replacement->left->parent = replacement;
        }
        replacement->color = node->color;
    }

    node->left.reset();
    node->right.reset();
    node->parent.reset();

    --size_;

    if (replacement_original_color == BLACK) {
        remove_fixup(fixup_node, fixup_parent);
    }

    return true;
}

template<typename T>
bool RedBlackTree<T>::search(const T& value) const {
    return static_cast<bool>(search_helper(root_, value));
}

template<typename T>
void RedBlackTree<T>::inorder_traversal(std::function<void(const T&)> callback) const {
    inorder_helper(root_, std::move(callback));
}

template<typename T>
void RedBlackTree<T>::clear() {
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
T RedBlackTree<T>::min() const {
    if (!root_) {
        throw std::runtime_error("Tree is empty");
    }
    return find_min(root_)->data;
}

template<typename T>
T RedBlackTree<T>::max() const {
    if (!root_) {
        throw std::runtime_error("Tree is empty");
    }
    return find_max(root_)->data;
}

template<typename T>
void RedBlackTree<T>::insert_fixup(std::shared_ptr<Node> node) {
    while (node != root_ && node->parent && node->parent->color == RED) {
        auto parent = node->parent;
        auto grandparent = parent->parent;
        if (!grandparent) {
            break;
        }

        if (parent == grandparent->left) {
            auto uncle = grandparent->right;
            if (uncle && uncle->color == RED) {
                parent->color = BLACK;
                uncle->color = BLACK;
                grandparent->color = RED;
                node = grandparent;
            } else {
                if (node == parent->right) {
                    node = parent;
                    rotate_left(node);
                    parent = node->parent;
                    grandparent = parent ? parent->parent : nullptr;
                }
                if (parent) {
                    parent->color = BLACK;
                }
                if (grandparent) {
                    grandparent->color = RED;
                    rotate_right(grandparent);
                }
            }
        } else {
            auto uncle = grandparent->left;
            if (uncle && uncle->color == RED) {
                parent->color = BLACK;
                uncle->color = BLACK;
                grandparent->color = RED;
                node = grandparent;
            } else {
                if (node == parent->left) {
                    node = parent;
                    rotate_right(node);
                    parent = node->parent;
                    grandparent = parent ? parent->parent : nullptr;
                }
                if (parent) {
                    parent->color = BLACK;
                }
                if (grandparent) {
                    grandparent->color = RED;
                    rotate_left(grandparent);
                }
            }
        }
    }

    if (root_) {
        root_->color = BLACK;
    }
}

template<typename T>
void RedBlackTree<T>::remove_fixup(std::shared_ptr<Node> node, std::shared_ptr<Node> parent) {
    auto color_of = [](const std::shared_ptr<Node>& current) {
        return current ? current->color : BLACK;
    };

    while (node != root_ && color_of(node) == BLACK) {
        if (!parent) {
            break;
        }

        if (node == parent->left) {
            auto sibling = parent->right;

            if (color_of(sibling) == RED) {
                sibling->color = BLACK;
                parent->color = RED;
                rotate_left(parent);
                sibling = parent->right;
            }

            if (color_of(sibling ? sibling->left : nullptr) == BLACK &&
                color_of(sibling ? sibling->right : nullptr) == BLACK) {
                if (sibling) {
                    sibling->color = RED;
                }
                node = parent;
                parent = node ? node->parent : nullptr;
            } else {
                if (color_of(sibling ? sibling->right : nullptr) == BLACK) {
                    if (sibling && sibling->left) {
                        sibling->left->color = BLACK;
                    }
                    if (sibling) {
                        sibling->color = RED;
                        rotate_right(sibling);
                    }
                    sibling = parent->right;
                }

                if (sibling) {
                    sibling->color = parent->color;
                }
                parent->color = BLACK;
                if (sibling && sibling->right) {
                    sibling->right->color = BLACK;
                }
                rotate_left(parent);
                node = root_;
                parent.reset();
            }
        } else {
            auto sibling = parent->left;

            if (color_of(sibling) == RED) {
                sibling->color = BLACK;
                parent->color = RED;
                rotate_right(parent);
                sibling = parent->left;
            }

            if (color_of(sibling ? sibling->left : nullptr) == BLACK &&
                color_of(sibling ? sibling->right : nullptr) == BLACK) {
                if (sibling) {
                    sibling->color = RED;
                }
                node = parent;
                parent = node ? node->parent : nullptr;
            } else {
                if (color_of(sibling ? sibling->left : nullptr) == BLACK) {
                    if (sibling && sibling->right) {
                        sibling->right->color = BLACK;
                    }
                    if (sibling) {
                        sibling->color = RED;
                        rotate_left(sibling);
                    }
                    sibling = parent->left;
                }

                if (sibling) {
                    sibling->color = parent->color;
                }
                parent->color = BLACK;
                if (sibling && sibling->left) {
                    sibling->left->color = BLACK;
                }
                rotate_right(parent);
                node = root_;
                parent.reset();
            }
        }
    }

    if (node) {
        node->color = BLACK;
    }
}

template<typename T>
void RedBlackTree<T>::rotate_left(std::shared_ptr<Node> node) {
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
void RedBlackTree<T>::rotate_right(std::shared_ptr<Node> node) {
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
std::shared_ptr<typename RedBlackTree<T>::Node> RedBlackTree<T>::find_min(std::shared_ptr<Node> node) const {
    while (node && node->left) {
        node = node->left;
    }
    return node;
}

template<typename T>
std::shared_ptr<typename RedBlackTree<T>::Node> RedBlackTree<T>::find_max(std::shared_ptr<Node> node) const {
    while (node && node->right) {
        node = node->right;
    }
    return node;
}

template<typename T>
std::shared_ptr<typename RedBlackTree<T>::Node> RedBlackTree<T>::search_helper(std::shared_ptr<Node> node, const T& value) const {
    while (node) {
        if (value < node->data) {
            node = node->left;
        } else if (value > node->data) {
            node = node->right;
        } else {
            return node;
        }
    }
    return nullptr;
}

template<typename T>
void RedBlackTree<T>::inorder_helper(std::shared_ptr<Node> node, std::function<void(const T&)> callback) const {
    if (!node) {
        return;
    }
    inorder_helper(node->left, callback);
    callback(node->data);
    inorder_helper(node->right, callback);
}

template<typename T>
void RedBlackTree<T>::transplant(std::shared_ptr<Node> u, std::shared_ptr<Node> v) {
    if (!u->parent) {
        root_ = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }

    if (v) {
        v->parent = u->parent;
    }
}

template class RedBlackTree<int>;
template class RedBlackTree<double>;
template class RedBlackTree<std::string>;

} // namespace data_structures
