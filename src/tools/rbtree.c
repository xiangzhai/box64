#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#ifdef RBTREE_TEST
#define rbtreeMalloc malloc
#define rbtreeFree free
#else
#include "custommem.h"
#include "debug.h"
#include "rbtree.h"
#if 0
#define rbtreeMalloc box_malloc
#define rbtreeFree box_free
#else
#define rbtreeMalloc customMalloc
#define rbtreeFree customFree
#endif
#endif

typedef struct rbnode {
    struct rbnode *left, *right, *parent;
    uintptr_t start, end;
    uint64_t data;
    uint8_t meta;
} rbnode;

struct rbtree {
    rbnode *root;
    const char* name;
    bool is_unstable;
    // Cache 
    rbnode *rightmost; 
    rbnode *leftmost;
    // TODO: Refine the naming scheme
};

rbtree_t* rbtree_init(const char* name) {
    rbtree_t* tree = rbtreeMalloc(sizeof(rbtree_t));
    tree->root = NULL;
    tree->is_unstable = false;
    tree->name = name?name:"(rbtree)";
    tree->rightmost = NULL;
    tree->leftmost = NULL;
    return tree;
}

static inline void delete_rbnode(rbnode *root) {
    if (!root) return;
    delete_rbnode(root->left);
    delete_rbnode(root->right);
    rbtreeFree(root);
}

void rbtree_delete(rbtree_t *tree) {
    delete_rbnode(tree->root);
    rbtreeFree(tree);
}

#define IS_LEFT  0x1
#define IS_BLACK 0x2

// Make sure prev is either the rightmost node before start or the leftmost range after start
static int add_range_next_to(rbtree_t *tree, rbnode *prev, uintptr_t start, uintptr_t end, uint64_t data) {
// printf("Adding %lx-%lx:%hhx next to %p\n", start, end, data, prev);
    rbnode *node = rbtreeMalloc(sizeof(*node));
    if (!node) return -1;
    node->start = start;
    node->end = end;
    node->data = data;
    node->left = NULL;
    node->right = NULL;

    if (tree->is_unstable) {
        printf_log(LOG_NONE, "Warning, unstable Red-Black tree \"%s\"; trying to add a node anyways\n", tree->name);
    }
    tree->is_unstable = true;

    if (!tree->root) {
        node->parent = NULL;
        node->meta = IS_BLACK;
        tree->root = node;
        tree->is_unstable = false;
        tree->leftmost = node;
        tree->rightmost = node;
        return 0;
    }
    
    // Update cache
    if (start < tree->leftmost->start) // new left most
        tree->leftmost = node;
    else if (start > tree->rightmost->start) // new right most
        tree->rightmost = node;

    node->parent = prev;
    if (prev->start < start) {
        prev->right = node;
        node->meta = 0;
    } else {
        prev->left = node;
        node->meta = IS_LEFT;
    }
    
    while (!(node->meta & IS_BLACK)) {
        if (!node->parent) {
            node->meta = IS_BLACK;
            tree->root = node;
            tree->is_unstable = false;
            return 0;
        }
        if (node->parent->meta & IS_BLACK) {
            tree->is_unstable = false;
            return 0;
        }
        if (!node->parent->parent) {
            tree->is_unstable = false;
            return 0; // Cannot happen as the root is black, unless the tree is unstable
        }
        if (node->parent->meta & IS_LEFT) {
            if (node->parent->parent->right && !(node->parent->parent->right->meta & IS_BLACK)) {
                node->parent->meta |= IS_BLACK;
                node = node->parent->parent;
                node->meta &= ~IS_BLACK;
                node->right->meta |= IS_BLACK;
            } else {
                if (!(node->meta & IS_LEFT)) {
                    rbnode *y, *z;
                    y = node;
                    z = y->parent;
                    // ((Bz->left), Rz, ((By->left), Ry, (By->right)))
                    // y = RED, rchild of z
                    // z = RED, child of z->parent
                    // target = (((Bz->left), Rz, (By->left)), Ry, (By->right))
                    if (z->meta & IS_LEFT) {
                        z->parent->left = y;
                    } else {
                        z->parent->right = y;
                    }
                    y->meta = z->meta; // red + same side as z
                    z->meta = IS_LEFT; // red + left
                    y->parent = z->parent;
                    z->parent = y;
                    z->right = y->left;
                    y->left = z;
                    if (z->right) {
                        z->right->meta &= ~IS_LEFT;
                        z->right->parent = z;
                    }
                    node = z;
                }
                rbnode *y, *z;
                y = node->parent;
                z = y->parent;
                // (((Rnode), Ry, (By->right)), Bz, (Bz->right))
                // node = RED, lchild of y
                // y = RED, lchild of z
                // z = BLACK, child of z->parent OR ROOT
                // target = ((Rnode), By, ((By->right), Rz, (Bz->right)))
                if (z->parent) {
                    if (z->meta & IS_LEFT) {
                        z->parent->left = y;
                    } else {
                        z->parent->right = y;
                    }
                }
                y->meta = z->meta; // black + same side as z
                z->meta = 0; // red + right
                y->parent = z->parent;
                z->parent = y;
                z->left = y->right;
                y->right = z;
                if (z->left) {
                    z->left->meta |= IS_LEFT;
                    z->left->parent = z;
                }
                if (!y->parent) tree->root = y;
                tree->is_unstable = false;
                return 0;
            }
        } else {
            if (node->parent->parent->left && !(node->parent->parent->left->meta & IS_BLACK)) {
                node->parent->meta |= IS_BLACK;
                node = node->parent->parent;
                node->meta &= ~IS_BLACK;
                node->left->meta |= IS_BLACK;
            } else {
                if (node->meta & IS_LEFT) {
                    rbnode *y, *z;
                    y = node;
                    z = y->parent;
                    // (((By->left), Ry, (By->right)), Rz, (Bz->right))
                    // y = RED, lchild of z
                    // z = RED, child of z->parent
                    // target = ((By->left), Ry, ((By->right), Rz, (Bz->right)))
                    if (z->meta & IS_LEFT) {
                        z->parent->left = y;
                    } else {
                        z->parent->right = y;
                    }
                    y->meta = z->meta; // red + same side as z
                    z->meta = 0; // red + right
                    y->parent = z->parent;
                    z->parent = y;
                    z->left = y->right;
                    y->right = z;
                    if (z->left) {
                        z->left->meta |= IS_LEFT;
                        z->left->parent = z;
                    }
                    node = z;
                }
                rbnode *y, *z;
                y = node->parent;
                z = y->parent;
                // ((Bz->left), Bz, ((By->left), Ry, (Rnode)))
                // node = RED, rchild of y
                // y = RED, rchild of z
                // z = BLACK, child of z->parent OR ROOT
                // target = (((Bz->left), Rz, (By->left)), By, (Rnode))
                if (z->parent) {
                    if (z->meta & IS_LEFT) {
                        z->parent->left = y;
                    } else {
                        z->parent->right = y;
                    }
                }
                y->meta = z->meta; // black + same side as z
                z->meta = IS_LEFT; // red + left
                y->parent = z->parent;
                z->parent = y;
                z->right = y->left;
                y->left = z;
                if (z->right) {
                    z->right->meta &= ~IS_LEFT;
                    z->right->parent = z;
                }
                if (!y->parent) tree->root = y;
                tree->is_unstable = false;
                return 0;
            }
        }
    }
    tree->is_unstable = false;
    return -1; // unreachable
}

static rbnode *find_addr(rbtree_t *tree, uintptr_t addr) {
    rbnode *node = tree->root;
    while (node) {
        if ((node->start <= addr) && (node->end > addr)) return node;
        if (addr < node->start) node = node->left;
        else node = node->right;
    }
    return NULL;
}

static rbnode *succ_node(rbnode *node);
static rbnode *pred_node(rbnode *node);
// node must be a valid node in the tree
static int remove_node(rbtree_t *tree, rbnode *node) {
// printf("Removing %p\n", node); rbtree_print(tree); fflush(stdout);
    if (tree->is_unstable) {
        printf_log(LOG_NONE, "Warning, unstable Red-Black tree; trying to add a node anyways\n");
    }
    tree->is_unstable = true;
    // Update cache
    if (node == tree->leftmost)
        tree->leftmost = succ_node(node);
    else if (node == tree->rightmost)
        tree->rightmost = pred_node(node);
    
    if (node->left && node->right) {
        // Swap node and its successor
        // Do NOT free the successor as a reference to it can exist
        rbnode *cur = node->right, *prev;
        while (cur) {
            prev = cur;
            cur = cur->left;
        }
        // Swap the position of node and prev != node
        uint8_t tmp8 = node->meta;
        node->meta = prev->meta;
        prev->meta = tmp8;
        prev->left = node->left;
        node->left = NULL;
        if (prev->left) prev->left->parent = prev;
        if (node->meta & IS_LEFT) {
            cur = node->parent;
            node->parent = prev->parent;
            prev->parent = cur;
            cur = node->right;
            node->right = prev->right;
            prev->right = cur;
            if (cur) cur->parent = prev;
        } else {
            node->right = prev->right;
            prev->right = node;
            prev->parent = node->parent;
            node->parent = prev;
        }
        if (node->right) node->right->parent = node; // Should be overriden later
        if (!prev->parent) {
            tree->root = prev; // prev is already black
        } else if (prev->meta & IS_LEFT) {
            prev->parent->left = prev;
        } else {
            prev->parent->right = prev;
        }
    }
    rbnode *child = node->left ? node->left : node->right, *parent = node->parent;
    if (child) {
        child->parent = parent;
        if (!parent) {
            tree->root = child;
            child->meta |= IS_BLACK; // Needs to be an or
            tree->is_unstable = false;
            return 0;
        } else if (node->meta & IS_LEFT) {
            child->meta |= IS_LEFT;
            parent->left = child;
        } else {
            child->meta &= ~IS_LEFT;
            parent->right = child;
        }
    } else {
        if (!parent) {
            tree->root = NULL;
            rbtreeFree(node);
            tree->is_unstable = false;
            return 0;
        } else if (node->meta & IS_LEFT) {
            parent->left = NULL;
        } else {
            parent->right = NULL;
        }
    }
    // Node has been removed, now to fix the tree
    if (!(node->meta & IS_BLACK)) {
        rbtreeFree(node);
        tree->is_unstable = false;
        return 0;
    }
    rbtreeFree(node);

    // Add a black node before child
    // Notice that the sibling cannot be NULL.
    while (parent && (!child || (child->meta & IS_BLACK))) {
        if ((child && child->meta & IS_LEFT) || (!child && !parent->left)) {
            node = parent->right;
            if (!(node->meta & IS_BLACK)) {
                // rotate ===
                rbnode *y, *z;
                y = node;
                z = parent;
                // ((Bchild), Bz, ((y->left), Ry, (y->right)))
                // y = RED, rchild of z
                // z = BLACK, child of z->parent OR ROOT
                // target = (((Bchild), Rz, (y->left)), By, (y->right))
                if (z->parent) {
                    if (z->meta & IS_LEFT) {
                        z->parent->left = y;
                    } else {
                        z->parent->right = y;
                    }
                } else {
                    tree->root = y;
                }
                y->meta = z->meta; // black + same side as z
                z->meta = IS_LEFT; // red + left
                y->parent = z->parent;
                z->parent = y;
                z->right = y->left;
                y->left = z;
                if (z->right) {
                    z->right->meta &= ~IS_LEFT;
                    z->right->parent = z;
                }
                // ===
                node = parent->right;
            }
            if (node->right && !(node->right->meta & IS_BLACK)) {
                case4_l: {
                rbnode *y, *z;
                y = node;
                z = parent;
                // ((Bchild), ?z, ((?y->left), By, (Ry->right)))
                // y = BLACK, rchild of z
                // z = ?, child of z->parent OR ROOT
                // target = (((Bchild), Bz, (?y->left)), ?y, (By->right))
                if (z->parent) {
                    if (z->meta & IS_LEFT) {
                        z->parent->left = y;
                    } else {
                        z->parent->right = y;
                    }
                }
                y->meta = z->meta; // same color as z + same side as z
                z->meta = IS_BLACK | IS_LEFT; // black + left
                y->parent = z->parent;
                z->parent = y;
                z->right = y->left;
                y->left = z;
                if (z->right) {
                    z->right->meta &= ~IS_LEFT;
                    z->right->parent = z;
                }
                if (!y->parent) tree->root = y;
                node->right->meta |= IS_BLACK;
                tree->is_unstable = false;
                return 0; }
            } else if (!node->left || (node->left->meta & IS_BLACK)) {
                // case2_l:
                child = parent; // Remember that child can be NULL
                parent = child->parent;
                node->meta &= ~IS_BLACK;
            } else {
                // case3_l:
                rbnode *y, *z;
                y = node->left;
                z = node;
                // (((y->left), Ry, (y->right)), Bz, (Bz->right))
                // y = RED, rchild of z
                // z = BLACK, child of z->parent
                // target = ((y->left), By, ((y->right), Rz, (z->right)))
                if (z->meta & IS_LEFT) {
                    z->parent->left = y;
                } else {
                    z->parent->right = y;
                }
                y->meta = z->meta; // black + same side as z
                z->meta = 0; // red + right
                y->parent = z->parent;
                z->parent = y;
                z->left = y->right;
                y->right = z;
                if (z->left) {
                    z->left->meta |= IS_LEFT;
                    z->left->parent = z;
                }
                node = y;
                goto case4_l;
            }
        } else {
            node = parent->left;
            if (!(node->meta & IS_BLACK)) {
                // rotate ===
                rbnode *y, *z;
                y = node;
                z = parent;
                // (((y->left), Ry, (y->right)), Bz, (Bchild))
                // y = RED, lchild of z
                // z = BLACK, child of z->parent OR ROOT
                // target = ((y->left), By, ((y->right), Rz, (Bchild)))
                if (z->parent) {
                    if (z->meta & IS_LEFT) {
                        z->parent->left = y;
                    } else {
                        z->parent->right = y;
                    }
                }
                y->meta = z->meta; // black + same side as z
                z->meta = 0; // red + right
                y->parent = z->parent;
                z->parent = y;
                z->left = y->right;
                y->right = z;
                if (z->left) {
                    z->left->meta |= IS_LEFT;
                    z->left->parent = z;
                }
                if (!y->parent) tree->root = y;
                // ===
                node = parent->left;
            }
            if (node->left && !(node->left->meta & IS_BLACK)) {
                case4_r: {
                rbnode *y, *z;
                y = node;
                z = y->parent;
                // (((?y->left), By, (Ry->right)), ?z, (Bchild))
                // y = BLACK, rchild of z
                // z = ?, child of z->parent OR ROOT
                // target = ((?y->left), ?y, ((Ry->right), Bz, (Bchild)))
                if (z->parent) {
                    if (z->meta & IS_LEFT) {
                        z->parent->left = y;
                    } else {
                        z->parent->right = y;
                    }
                }
                y->meta = z->meta; // same color as z + same side as z
                z->meta = IS_BLACK; // black + right
                y->parent = z->parent;
                z->parent = y;
                z->left = y->right;
                y->right = z;
                if (z->left) {
                    z->left->meta |= IS_LEFT;
                    z->left->parent = z;
                }
                if (!y->parent) tree->root = y;
                node->left->meta |= IS_BLACK;
                tree->is_unstable = false;
                return 0; }
            } else if (!node->right || (node->right->meta & IS_BLACK)) {
                // case2_r:
                child = parent;
                parent = child->parent;
                node->meta &= ~IS_BLACK;
            } else {
                // case3_r:
                rbnode *y, *z;
                y = node->right;
                z = node;
                // ((Bz->left), Bz, ((y->left), Ry, (y->right)))
                // y = RED, rchild of z
                // z = BLACK, child of z->parent
                // target = (((Bz->left), Rz, (y->left)), By, (y->right))
                if (z->meta & IS_LEFT) {
                    z->parent->left = y;
                } else {
                    z->parent->right = y;
                }
                y->meta = z->meta; // black + same side as z
                z->meta = IS_LEFT; // red + left
                y->parent = z->parent;
                z->parent = y;
                z->right = y->left;
                y->left = z;
                if (z->right) {
                    z->right->meta &= ~IS_LEFT;
                    z->right->parent = z;
                }
                node = y;
                goto case4_r;
            }
        }
    }
    if (child)
        child->meta |= IS_BLACK;
    tree->is_unstable = false;
    return 0;
}

static rbnode *pred_node(rbnode *node) {
    if (!node) return NULL;
    if (node->left) {
        node = node->left;
        while (node->right) node = node->right;
        return node;
    } else {
        while (node->parent && node->meta & IS_LEFT) node = node->parent;
        return node->parent;
    }
}

static rbnode *succ_node(rbnode *node) {
    if (!node) return NULL;
    if (node->right) {
        node = node->right;
        while (node->left) node = node->left;
        return node;
    } else {
        while (node->parent && !(node->meta & IS_LEFT)) node = node->parent;
        return node->parent;
    }
}

uint32_t rb_get(rbtree_t *tree, uintptr_t addr) {
    if (tree->leftmost && addr < tree->leftmost->start) return 0;
    if (tree->rightmost && addr > tree->rightmost->end) return 0;
    rbnode *node = find_addr(tree, addr);
    if (node) return node->data;
    return 0;
}

uint64_t rb_get_64(rbtree_t *tree, uintptr_t addr) {
    if (tree->leftmost && addr < tree->leftmost->start) return 0;
    if (tree->rightmost && addr > tree->rightmost->end) return 0;
    rbnode *node = find_addr(tree, addr);
    if (node) return node->data;
    return 0;
}

int rb_get_end(rbtree_t* tree, uintptr_t addr, uint32_t* val, uintptr_t* end) {
    rbnode *node = tree->root, *next = NULL;
    while (node) {
        if ((node->start <= addr) && (node->end > addr)) {
            *val = node->data;
            *end = node->end;
            return 1;
        }
        if (node->end <= addr) {
            node = node->right;
        } else {
            next = node;
            node = node->left;
        }
    }
    *val = 0;
    if (next) {
        *end = next->start;
    } else {
        *end = (uintptr_t)-1;
    }
    return 0;
}

int rb_get_end_64(rbtree_t* tree, uintptr_t addr, uint64_t* val, uintptr_t* end) {
    rbnode *node = tree->root, *next = NULL;
    while (node) {
        if ((node->start <= addr) && (node->end > addr)) {
            *val = node->data;
            *end = node->end;
            return 1;
        }
        if (node->end <= addr) {
            node = node->right;
        } else {
            next = node;
            node = node->left;
        }
    }
    *val = 0;
    if (next) {
        *end = next->start;
    } else {
        *end = (uintptr_t)-1;
    }
    return 0;
}

int rb_set_64(rbtree_t *tree, uintptr_t start, uintptr_t end, uint64_t data) {
// printf("rb_set( "); rbtree_print(tree); printf(" , 0x%lx, 0x%lx, %hhu);\n", start, end, data); fflush(stdout);
dynarec_log(LOG_DEBUG, "set %s: 0x%lx, 0x%lx, 0x%x\n", tree->name, start, end, data);
    if (!tree->root) {
        return add_range_next_to(tree, NULL, start, end, data);
    }
    
    rbnode *node = tree->root, *prev = NULL, *last = NULL;
    while (node) {
        if (node->start < start) {
            prev = node;
            node = node->right;
        } else if (node->start == start) {
            if (node->left) {
                prev = node->left;
                while (prev->right) prev = prev->right;
            }
            if (node->right) {
                last = node->right;
                while (last->left) last = last->left;
            }
            break;
        } else {
            last = node;
            node = node->left;
        }
    }

    // prev is the largest node starting strictly before start, or NULL if there is none
    // node is the node starting exactly at start, or NULL if there is none
    // last is the smallest node starting strictly after start, or NULL if there is none
    // Note that prev may contain start

    if (prev && (prev->end >= start) && (prev->data == data)) {
        // Merge with prev
        if (end <= prev->end) return 0; // Nothing to do!
        
        if (node && (node->end > end)) {
            node->start = end;
            prev->end = end;
            return 0;
        } else if (node && (node->end == end)) {
            remove_node(tree, node);
            prev->end = end;
            return 0;
        } else if (node) {
            remove_node(tree, node);
        }
        while (last && (last->start < end) && (last->end <= end)) {
            // Remove the entire node
            node = last;
            last = succ_node(last);
            remove_node(tree, node);
        }
        if (last && (last->start <= end) && (last->data == data)) {
            // Merge node and last
            prev->end = last->end;
            remove_node(tree, last);
            return 0;
        }
        if (last && (last->start < end)) last->start = end;
        prev->end = end;
        return 0;
    } else if (prev && (prev->end > start)) {
        if (prev->end > end) {
            // Split in three
            // Note that here, succ(prev) = last and node = NULL
            int ret;
            ret = add_range_next_to(tree, prev->right ? last : prev, end, prev->end, prev->data);
            ret = ret ? ret : add_range_next_to(tree, prev->right ? succ_node(prev) : prev, start, end, data);
            prev->end = start;
            return ret;
        }
        // Cut prev and continue
        prev->end = start;
    }

    if (node) {
        // Change node
        if (node->end >= end) {
            if (node->data == data) return 0; // Nothing to do!
            // Cut node
            if (node->end > end) {
                int ret = add_range_next_to(tree, node->right ? last : node, end, node->end, node->data);
                node->end = end;
                node->data = data;
                return ret;
            }
            // Fallthrough
        }
        
        // Overwrite and extend node
        while (last && (last->start < end) && (last->end <= end)) {
            // Remove the entire node
            prev = last;
            last = succ_node(last);
            remove_node(tree, prev);
        }
        if (last && (last->start <= end) && (last->data == data)) {
            // Merge node and last
            remove_node(tree, node);
            last->start = start;
            return 0;
        }
        if (last && (last->start < end)) last->start = end;
        if (node->end < end) node->end = end;
        node->data = data;
        return 0;
    }

    while (last && (last->start < end) && (last->end <= end)) {
        // Remove the entire node
        node = last;
        last = succ_node(last);
        remove_node(tree, node);
    }
    if (!last) {
        // Add a new node next to prev, the largest node of the tree
        // It exists since the tree is nonempty
        return add_range_next_to(tree, prev, start, end, data);
    }
    if ((last->start <= end) && (last->data == data)) {
        // Extend
        last->start = start;
        return 0;
    } else if (last->start < end) {
        // Cut
        last->start = end;
    }
    // Probably 'last->left ? prev : last' is enough
    return add_range_next_to(tree, last->left ? pred_node(last) : last, start, end, data);
}

int rb_set(rbtree_t *tree, uintptr_t start, uintptr_t end, uint32_t data) {
    return rb_set_64(tree, start, end, data);
}

int rb_unset(rbtree_t *tree, uintptr_t start, uintptr_t end) {
// printf("rb_unset( "); rbtree_print(tree); printf(" , 0x%lx, 0x%lx);\n", start, end); fflush(stdout);
dynarec_log(LOG_DEBUG, "unset: %s 0x%lx, 0x%lx);\n", tree->name, start, end);
    if (!tree->root) return 0;

    rbnode *node = tree->root, *prev = NULL, *next = NULL;
    while (node) {
        if (node->start < start) {
            prev = node;
            node = node->right;
        } else if (node->start == start) {
            if (node->left) {
                prev = node->left;
                while (prev->right) prev = prev->right;
            }
            if (node->right) {
                next = node->right;
                while (next->left) next = next->left;
            }
            break;
        } else {
            next = node;
            node = node->left;
        }
    }

    if (node) {
        if (node->end > end) {
            node->start = end;
            return 0;
        } else if (node->end == end) {
            remove_node(tree, node);
            return 0;
        } else {
            remove_node(tree, node);
        }
    } else if (prev && (prev->end > start)) {
        if (prev->end > end) {
            // Split prev
            int ret = add_range_next_to(tree, prev->right ? next : prev, end, prev->end, prev->data);
            prev->end = start;
            return ret;
        } else if (prev->end == end) {
            prev->end = start;
            return 0;
        } else {
            prev->end = start;
        }
    }
    while (next && (next->start < end) && (next->end <= end)) {
        // Remove the entire node
        node = next;
        next = succ_node(next);
        remove_node(tree, node);
    }
    if (next && (next->start < end)) {
        // next->end > end: cut the node
        next->start = end;
    }
    return 0;
}

uint64_t rb_inc(rbtree_t *tree, uintptr_t start, uintptr_t end) {
    // printf("rb_inc( "); rbtree_print(tree); printf(" , 0x%lx, 0x%lx);\n", start, end); fflush(stdout);
    dynarec_log(LOG_DEBUG, "inc %s: 0x%lx, 0x%lx\n", tree->name, start, end);
    if (!tree->root) {
        add_range_next_to(tree, NULL, start, end, 1);
        return 1;
    }
    
    rbnode *node = tree->root, *prev = NULL, *last = NULL;
    while (node) {
        if (node->start < start) {
            prev = node;
            node = node->right;
        } else if (node->start == start) {
            if (node->left) {
                prev = node->left;
                while (prev->right) prev = prev->right;
            }
            if (node->right) {
                last = node->right;
                while (last->left) last = last->left;
            }
            break;
        } else {
            last = node;
            node = node->left;
        }
    }

    // prev is the largest node starting strictly before start, or NULL if there is none
    // node is the node starting exactly at start, or NULL if there is none
    // last is the smallest node starting strictly after start, or NULL if there is none
    // Note that prev may contain start

    uint64_t data = (node?node->data:0)+1;

    if (prev && (prev->end >= start) && (prev->data == data)) {
        // Merge with prev
        if (end <= prev->end) return data; // Nothing to do!
        
        if (node && (node->end > end)) {
            node->start = end;
            prev->end = end;
            return data;
        } else if (node && (node->end == end)) {
            remove_node(tree, node);
            prev->end = end;
            return data;
        } else if (node) {
            remove_node(tree, node);
        }
        while (last && (last->start < end) && (last->end <= end)) {
            // Remove the entire node
            node = last;
            last = succ_node(last);
            remove_node(tree, node);
        }
        if (last && (last->start <= end) && (last->data == data)) {
            // Merge node and last
            prev->end = last->end;
            remove_node(tree, last);
            return data;
        }
        if (last && (last->start < end)) last->start = end;
        prev->end = end;
        return data;
    } else if (prev && (prev->end > start)) {
        if (prev->end > end) {
            // Split in three
            // Note that here, succ(prev) = last and node = NULL
            int ret;
            ret = add_range_next_to(tree, prev->right ? last : prev, end, prev->end, prev->data);
            ret = ret ? ret : add_range_next_to(tree, prev->right ? succ_node(prev) : prev, start, end, data);
            prev->end = start;
            return ret;
        }
        // Cut prev and continue
        prev->end = start;
    }

    if (node) {
        // Change node
        if (node->end >= end) {
            if (node->data == data) return 0; // Nothing to do!
            // Cut node
            if (node->end > end) {
                int ret = add_range_next_to(tree, node->right ? last : node, end, node->end, node->data);
                node->end = end;
                node->data = data;
                return ret;
            }
            // Fallthrough
        }
        
        // Overwrite and extend node
        while (last && (last->start < end) && (last->end <= end)) {
            // Remove the entire node
            prev = last;
            last = succ_node(last);
            remove_node(tree, prev);
        }
        if (last && (last->start <= end) && (last->data == data)) {
            // Merge node and last
            remove_node(tree, node);
            last->start = start;
            return data;
        }
        if (last && (last->start < end)) last->start = end;
        if (node->end < end) node->end = end;
        node->data = data;
        return data;
    }

    while (last && (last->start < end) && (last->end <= end)) {
        // Remove the entire node
        node = last;
        last = succ_node(last);
        remove_node(tree, node);
    }
    if (!last) {
        // Add a new node next to prev, the largest node of the tree
        // It exists since the tree is nonempty
        return add_range_next_to(tree, prev, start, end, data);
    }
    if ((last->start <= end) && (last->data == data)) {
        // Extend
        last->start = start;
        return data;
    } else if (last->start < end) {
        // Cut
        last->start = end;
    }
    // Probably 'last->left ? prev : last' is enough
    add_range_next_to(tree, last->left ? pred_node(last) : last, start, end, data);
    return data;
}

uint64_t rb_dec(rbtree_t *tree, uintptr_t start, uintptr_t end) {
    // printf("rb_dec( "); rbtree_print(tree); printf(" , 0x%lx, 0x%lx);\n", start, end); fflush(stdout);
    dynarec_log(LOG_DEBUG, "dec %s: 0x%lx, 0x%lx\n", tree->name, start, end);
    if (!tree->root) {
        return 0;
    }
    
    rbnode *node = tree->root, *prev = NULL, *last = NULL;
    while (node) {
        if (node->start < start) {
            prev = node;
            node = node->right;
        } else if (node->start == start) {
            if (node->left) {
                prev = node->left;
                while (prev->right) prev = prev->right;
            }
            if (node->right) {
                last = node->right;
                while (last->left) last = last->left;
            }
            break;
        } else {
            last = node;
            node = node->left;
        }
    }

    // prev is the largest node starting strictly before start, or NULL if there is none
    // node is the node starting exactly at start, or NULL if there is none
    // last is the smallest node starting strictly after start, or NULL if there is none
    // Note that prev may contain start

    uint64_t data = (node?node->data:0);
    if(!data) return data;
    --data;
    if(!data) {
        // delete the node...
        if (node) {
            if (node->end > end) {
                node->start = end;
                return 0;
            } else if (node->end == end) {
                remove_node(tree, node);
                return 0;
            } else {
                remove_node(tree, node);
            }
        } else if (prev && (prev->end > start)) {
            if (prev->end > end) {
                // Split prev
                int ret = add_range_next_to(tree, prev->right ? last : prev, end, prev->end, prev->data);
                prev->end = start;
                return ret;
            } else if (prev->end == end) {
                prev->end = start;
                return 0;
            } // else fallthrough
        }
        while (last && (last->start < end) && (last->end <= end)) {
            // Remove the entire node
            node = last;
            last = succ_node(last);
            remove_node(tree, node);
        }
        if (last && (last->start < end)) {
            // last->end > end: cut the node
            last->start = end;
        }
        return 0;
    }

    if (prev && (prev->end >= start) && (prev->data == data)) {
        // Merge with prev
        if (end <= prev->end) return data; // Nothing to do!
        
        if (node && (node->end > end)) {
            node->start = end;
            prev->end = end;
            return data;
        } else if (node && (node->end == end)) {
            remove_node(tree, node);
            prev->end = end;
            return data;
        } else if (node) {
            remove_node(tree, node);
        }
        while (last && (last->start < end) && (last->end <= end)) {
            // Remove the entire node
            node = last;
            last = succ_node(last);
            remove_node(tree, node);
        }
        if (last && (last->start <= end) && (last->data == data)) {
            // Merge node and last
            prev->end = last->end;
            remove_node(tree, last);
            return data;
        }
        if (last && (last->start < end)) last->start = end;
        prev->end = end;
        return data;
    } else if (prev && (prev->end > start)) {
        if (prev->end > end) {
            // Split in three
            // Note that here, succ(prev) = last and node = NULL
            int ret;
            ret = add_range_next_to(tree, prev->right ? last : prev, end, prev->end, prev->data);
            ret = ret ? ret : add_range_next_to(tree, prev->right ? succ_node(prev) : prev, start, end, data);
            prev->end = start;
            return ret;
        }
        // Cut prev and continue
        prev->end = start;
    }

    if (node) {
        // Change node
        if (node->end >= end) {
            if (node->data == data) return 0; // Nothing to do!
            // Cut node
            if (node->end > end) {
                int ret = add_range_next_to(tree, node->right ? last : node, end, node->end, node->data);
                node->end = end;
                node->data = data;
                return ret;
            }
            // Fallthrough
        }
        
        // Overwrite and extend node
        while (last && (last->start < end) && (last->end <= end)) {
            // Remove the entire node
            prev = last;
            last = succ_node(last);
            remove_node(tree, prev);
        }
        if (last && (last->start <= end) && (last->data == data)) {
            // Merge node and last
            remove_node(tree, node);
            last->start = start;
            return data;
        }
        if (last && (last->start < end)) last->start = end;
        if (node->end < end) node->end = end;
        node->data = data;
        return data;
    }

    while (last && (last->start < end) && (last->end <= end)) {
        // Remove the entire node
        node = last;
        last = succ_node(last);
        remove_node(tree, node);
    }
    if (!last) {
        // Add a new node next to prev, the largest node of the tree
        // It exists since the tree is nonempty
        return add_range_next_to(tree, prev, start, end, data);
    }
    if ((last->start <= end) && (last->data == data)) {
        // Extend
        last->start = start;
        return data;
    } else if (last->start < end) {
        // Cut
        last->start = end;
    }
    // Probably 'last->left ? prev : last' is enough
    add_range_next_to(tree, last->left ? pred_node(last) : last, start, end, data);
    return data;
}

uintptr_t rb_get_rightmost(rbtree_t* tree)
{
dynarec_log(LOG_DEBUG, "rb_get_rightmost(%s);\n", tree->name);
    if (!tree->root) return 0;
    return tree->rightmost->start;
}

uintptr_t rb_get_leftmost(rbtree_t* tree)
{
dynarec_log(LOG_DEBUG, "rb_get_leftmost(%s);\n", tree->name);
    if (!tree->root) return 0;
    return tree->leftmost->start;
}

#include <stdio.h>
#if 0
#define printf_log(L, ...) printf(__VA_ARGS__)
#define LOG_NONE    0
#endif
static void print_rbnode(const rbnode *node, unsigned depth, uintptr_t minstart, uintptr_t maxend, unsigned *bdepth) {
    if (!node) {
        if (!*bdepth || *bdepth == depth + 1) {
            *bdepth = depth + 1;
            printf_log(LOG_NONE, "[%u]", depth);
        } else
            printf_log(LOG_NONE, "<invalid black depth %u>", depth);
        return;
    }
    if (node->start < minstart) {
        printf_log(LOG_NONE, "<invalid start>");
        return;
    }
    if (node->end > maxend) {
        printf_log(LOG_NONE, "<invalid end>");
        return;
    }
    printf_log(LOG_NONE, "(");
    if (node->left && !(node->left->meta & IS_LEFT)) {
        printf_log(LOG_NONE, "<invalid meta>");
    } else if (node->left && (node->left->parent != node)) {
        printf_log(LOG_NONE, "<invalid parent %p instead of %p>", node->left->parent, node);
    } else if (node->left && !(node->meta & IS_BLACK) && !(node->left->meta & IS_BLACK)) {
        printf_log(LOG_NONE, "<invalid red-red node> ");
        print_rbnode(node->left, depth + ((node->meta & IS_BLACK) ? 1 : 0), minstart, node->start, bdepth);
    } else {
        print_rbnode(node->left, depth + ((node->meta & IS_BLACK) ? 1 : 0), minstart, node->start, bdepth);
    }
    printf_log(LOG_NONE, ", (%c/%p) %lx-%lx: %" PRIu64 ", ", node->meta & IS_BLACK ? 'B' : 'R', node, node->start, node->end, node->data);
    if (node->right && (node->right->meta & IS_LEFT)) {
        printf_log(LOG_NONE, "<invalid meta>");
    } else if (node->right && (node->right->parent != node)) {
        printf_log(LOG_NONE, "<invalid parent %p instead of %p>", node->right->parent, node);
    } else if (node->right && !(node->meta & IS_BLACK) && !(node->right->meta & IS_BLACK)) {
        printf_log(LOG_NONE, "<invalid red-red node> ");
        print_rbnode(node->right, depth + ((node->meta & IS_BLACK) ? 1 : 0), node->end, maxend, bdepth);
    } else {
        print_rbnode(node->right, depth + ((node->meta & IS_BLACK) ? 1 : 0), node->end, maxend, bdepth);
    }
    printf_log(LOG_NONE, ")");
}

static void cache_check(const rbtree_t *tree) {
    if (!tree || !tree->root)
        return;
    // find right most
    rbnode *right_node = tree->root;
    while (right_node->right)
        right_node = right_node->right;

    if (tree->rightmost != right_node){
        printf_log(LOG_NONE, "<invalid rightmost node>\n");
        return;
    }

    // find left most
    rbnode *left_node = tree->root;
    while (left_node->left)
        left_node = left_node->left;

    if (tree->leftmost != left_node){
        printf_log(LOG_NONE, "<invalid leftmost node>\n");
        return;
    }

    printf_log(LOG_NONE, "<valid cached node> \n");
}

void rbtree_walk(const rbtree_t *tree, void (*cb)(uintptr_t start, uintptr_t end, uint64_t data, void* userdata), void* userdata) {
    if (!tree || !tree->root) return;
    rbnode *node = tree->leftmost;
    while (node) {
        cb(node->start, node->end, node->data, userdata);
        node = succ_node(node);
    }
}

void rbtree_print(const rbtree_t *tree) {
    if (!tree) {
        printf_log(LOG_NONE, "<NULL>\n");
        return;
    }
    if (tree->name)
        printf_log(LOG_NONE, "tree name: %s\n", tree->name);
    if (tree->root && tree->root->parent) {
        printf_log(LOG_NONE, "Root has parent\n");
        return;
    }
    if (tree->root && !(tree->root->meta & IS_BLACK)) {
        printf_log(LOG_NONE, "Root is red\n");
        return;
    }
    cache_check(tree);
    unsigned bdepth = 0;
    print_rbnode(tree->root, 0, 0, (uintptr_t)-1, &bdepth);
    printf_log(LOG_NONE, "\n");
}

#ifdef RBTREE_TEST
int main() {
    rbtree_t* tree = rbtree_init("test");
    rbtree_print(tree); fflush(stdout);
    /*int ret;
    ret = rb_set(tree, 0x43, 0x44, 0x01);
    printf("%d; ", ret); rbtree_print(tree); fflush(stdout);
    ret = rb_set(tree, 0x42, 0x43, 0x01);
    printf("%d; ", ret); rbtree_print(tree); fflush(stdout);
    ret = rb_set(tree, 0x41, 0x42, 0x01);
    printf("%d; ", ret); rbtree_print(tree); fflush(stdout);
    ret = rb_set(tree, 0x40, 0x41, 0x01);
    printf("%d; ", ret); rbtree_print(tree); fflush(stdout);
    ret = rb_set(tree, 0x20, 0x40, 0x03);
    printf("%d; ", ret); rbtree_print(tree); fflush(stdout);
    ret = rb_set(tree, 0x10, 0x20, 0x01);
    printf("%d; ", ret); rbtree_print(tree); fflush(stdout);

    uint32_t val = rb_get(tree, 0x33);
    printf("0x33 has attribute %hhu\n", val); fflush(stdout);*/
    /* rbnode *node = find_addr(tree, 0x33);
    printf("0x33 is at %p: ", node); print_rbnode(node, 0); printf("\n"); fflush(stdout);
    ret = remove_node(tree, node);
    printf("%d; ", ret); rbtree_print(tree); fflush(stdout);
    node = find_addr(tree, 0x20);
    printf("0x20 is at %p\n", node);
    node = find_addr(tree, 0x1F);
    printf("0x1F is at %p: ", node); print_rbnode(node, 0); printf("\n"); fflush(stdout);
    ret = remove_node(tree, node);
    printf("%d; ", ret); rbtree_print(tree); fflush(stdout); */
    /* ret = rb_set(tree, 0x15, 0x42, 0x00);
    printf("%d; ", ret); rbtree_print(tree); fflush(stdout); */
    /*rb_unset(tree, 0x15, 0x42);
    rbtree_print(tree); fflush(stdout);*/
    
    // tree->root = node27; rbtree_print(tree); fflush(stdout);
    // rb_set(tree, 2, 3, 1); rbtree_print(tree); fflush(stdout);
    // add_range_next_to(tree, node24, 0x0E7000, 0x0E8000, 69); rbtree_print(tree); fflush(stdout);
    // rbtree_print(tree); fflush(stdout);
    // uint32_t val = rb_get(tree, 0x11003000);
    // printf("0x11003000 has attribute %hhu\n", val); fflush(stdout);
    // remove_node(tree, node0); rbtree_print(tree); fflush(stdout);
    // add_range_next_to(tree, node1, 0x0E7000, 0x0E8000, 69); rbtree_print(tree); fflush(stdout);
rb_set(tree, 0x130000, 0x140000, 7);
    rbtree_print(tree); fflush(stdout);
rb_set(tree, 0x141000, 0x142000, 135);
    rbtree_print(tree); fflush(stdout);
rb_set(tree, 0x140000, 0x141000, 135);
    rbtree_print(tree); fflush(stdout);
rb_set(tree, 0x140000, 0x141000, 7);
    rbtree_print(tree); fflush(stdout);
rb_set(tree, 0x140000, 0x141000, 135);
    rbtree_print(tree); fflush(stdout);
    uint32_t val = rb_get(tree, 0x141994); printf("0x141994 has attribute %hhu\n", val); fflush(stdout);
    rbtree_delete(tree);
}
#endif

#ifdef RBTREE_MT_SAFE_TEST
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>

extern FILE* ftrace;

// Test configuration
#define NUM_THREADS           10
#define NUM_OPS               10000
#define ADDR_RANGE            1000
#define MAX_VALUE             255
#define VERIFICATION_INTERVAL 100
#define MAX_ERRORS_TO_PRINT   10

// Global state
rbtree_t* tree;
uint32_t* shadow_memory; // Shadow memory as correct reference
pthread_mutex_t shadow_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
atomic_ulong total_operations = 0;
atomic_ulong total_errors = 0;

typedef enum {
    OP_SET,
    OP_UNSET,
    OP_VERIFY
} operation_type_t;

typedef struct {
    operation_type_t type;
    uintptr_t start;
    uintptr_t end;
    uint32_t data;
    uint32_t thread_id;
    uint64_t timestamp;
} operation_log_t;

#define LOG_SIZE 1000000
operation_log_t op_log[LOG_SIZE];
atomic_int log_index = 0;

typedef struct {
    int thread_id;
    uint64_t ops;
    uint64_t errors;
    uint64_t verify_failures;
    uint64_t reads;
    uint64_t writes;
} thread_stats_t;

thread_stats_t thread_stats[NUM_THREADS * 2];

// Forward declarations
void init_shadow(void);
void shadow_set(uintptr_t addr, uint32_t value);
uint32_t shadow_get(uintptr_t addr);
int verify_with_shadow(uintptr_t addr, uint32_t tree_val, const char* context, int thread_id);
void log_operation(operation_type_t type, uintptr_t start, uintptr_t end, uint32_t data, int thread_id);
void replay_verify(void);
void range_consistency_check(void);
uint64_t calculate_tree_checksum(void);
void print_statistics(void);
void* writer_thread(void* arg);
void* reader_thread(void* arg);
void* mixed_thread(void* arg);
void* validator_thread(void* arg);

// Initialize shadow memory
void init_shadow(void)
{
    shadow_memory = (uint32_t*)malloc(ADDR_RANGE * sizeof(uint32_t));
    if (!shadow_memory) {
        fprintf(stderr, "Failed to allocate shadow memory\n");
        exit(1);
    }

    for (int i = 0; i < ADDR_RANGE; i++) {
        shadow_memory[i] = i % (MAX_VALUE + 1); // Predictable initial values
    }

    printf("Shadow memory initialized with %d entries\n", ADDR_RANGE);
}

// Thread-safe shadow memory update
void shadow_set(uintptr_t addr, uint32_t value)
{
    if (addr >= ADDR_RANGE) return;
    pthread_mutex_lock(&shadow_mutex);
    shadow_memory[addr] = value;
    pthread_mutex_unlock(&shadow_mutex);
}

uint32_t shadow_get(uintptr_t addr)
{
    if (addr >= ADDR_RANGE) return 0;
    pthread_mutex_lock(&shadow_mutex);
    uint32_t val = shadow_memory[addr];
    pthread_mutex_unlock(&shadow_mutex);
    return val;
}

// Verify against shadow memory
int verify_with_shadow(uintptr_t addr, uint32_t tree_val, const char* context, int thread_id)
{
    if (addr >= ADDR_RANGE) return 1;

    uint32_t shadow_val = shadow_get(addr);

    if (tree_val != shadow_val) {
        pthread_mutex_lock(&print_mutex);
        printf("[Thread %d] %s: Address 0x%lx mismatch! Tree=%u, Shadow=%u\n",
            thread_id, context, (unsigned long)addr, tree_val, shadow_val);
        atomic_fetch_add(&total_errors, 1);
        pthread_mutex_unlock(&print_mutex);
        return 0;
    }
    return 1;
}

// Log operations for replay verification
void log_operation(operation_type_t type, uintptr_t start, uintptr_t end, uint32_t data, int thread_id)
{
    int idx = atomic_fetch_add(&log_index, 1);
    if (idx < LOG_SIZE) {
        op_log[idx].type = type;
        op_log[idx].start = start;
        op_log[idx].end = end;
        op_log[idx].data = data;
        op_log[idx].thread_id = thread_id;
        op_log[idx].timestamp = time(NULL);
    }
}

// Replay all logged operations to verify consistency
void replay_verify(void)
{
    printf("\n=== Starting Replay Verification ===\n");

    // Create a new clean tree
    rbtree_t* replay_tree = rbtree_init("replay_tree");

    int log_count = atomic_load(&log_index);
    if (log_count > LOG_SIZE) log_count = LOG_SIZE;

    printf("Replaying %d operations...\n", log_count);

    // Replay all operations in order
    for (int i = 0; i < log_count; i++) {
        operation_log_t* op = &op_log[i];

        switch (op->type) {
            case OP_SET:
                rb_set(replay_tree, op->start, op->end, op->data);
                break;
            case OP_UNSET:
                rb_unset(replay_tree, op->start, op->end);
                break;
            case OP_VERIFY:
                // Skip verification operations
                break;
        }

        if (i % 10000 == 0 && i > 0) {
            printf("Replayed %d operations...\n", i);
        }
    }

    // Compare trees
    printf("\nComparing trees...\n");
    int errors = 0;
    for (uintptr_t addr = 0; addr < ADDR_RANGE; addr++) {
        uint32_t original = rb_get(tree, addr);
        uint32_t replay = rb_get(replay_tree, addr);

        if (original != replay) {
            if (errors < MAX_ERRORS_TO_PRINT) {
                printf("Replay mismatch: addr=0x%lx, original=%u, replay=%u\n",
                    (unsigned long)addr, original, replay);
            }
            errors++;
        }
    }

    printf("Replay verification complete. Errors found: %d\n", errors);
    rbtree_delete(replay_tree);
}

// Check range consistency (no gaps/overlaps)
void range_consistency_check(void)
{
    printf("\n=== Starting Range Consistency Check ===\n");

    int errors = 0;
    uint32_t last_value = 0;
    uintptr_t last_end = 0;

    // Use a simple array to track coverage
    int* coverage = (int*)calloc(ADDR_RANGE, sizeof(int));

    for (uintptr_t addr = 0; addr < ADDR_RANGE; addr++) {
        uint32_t val = rb_get(tree, addr);
        coverage[addr] = val;
    }

    // Check for gaps (value 0 might be valid, so we need another method)
    // This is a simple check for sudden changes that might indicate problems

    for (uintptr_t addr = 1; addr < ADDR_RANGE; addr++) {
        if (coverage[addr] != coverage[addr - 1]) {
            // Value changed, check if it's at a valid boundary
            uintptr_t end;
            uint32_t tmp;
            rb_get_end(tree, addr - 1, &tmp, &end);

            if (end != addr) {
                printf("Possible boundary error: at 0x%lx value changed from %u to %u, but previous node ends at 0x%lx\n",
                    (unsigned long)addr, coverage[addr - 1], coverage[addr], (unsigned long)end);
                errors++;
            }
        }
    }

    free(coverage);
    printf("Range consistency check complete. Found %d potential issues\n", errors);
}

// Calculate checksum of entire tree
uint64_t calculate_tree_checksum(void)
{
    uint64_t checksum = 0;

    for (uintptr_t addr = 0; addr < ADDR_RANGE; addr++) {
        uint32_t val = rb_get(tree, addr);
        checksum = checksum * 31 + val;
    }

    return checksum;
}

// Print detailed statistics
void print_statistics(void)
{
    printf("\n=== Test Statistics ===\n");

    uint64_t total_ops = 0;
    uint64_t total_err = 0;
    uint64_t total_verify = 0;
    uint64_t total_reads = 0;
    uint64_t total_writes = 0;

    for (int i = 0; i < NUM_THREADS * 2; i++) {
        printf("Thread %d: ops=%lu, errors=%lu, verify_fail=%lu, reads=%lu, writes=%lu\n",
            thread_stats[i].thread_id,
            thread_stats[i].ops,
            thread_stats[i].errors,
            thread_stats[i].verify_failures,
            thread_stats[i].reads,
            thread_stats[i].writes);

        total_ops += thread_stats[i].ops;
        total_err += thread_stats[i].errors;
        total_verify += thread_stats[i].verify_failures;
        total_reads += thread_stats[i].reads;
        total_writes += thread_stats[i].writes;
    }

    printf("\nTotals:\n");
    printf("Total operations: %lu\n", total_ops);
    printf("Total reads: %lu\n", total_reads);
    printf("Total writes: %lu\n", total_writes);
    printf("Total errors: %lu (%.4f%%)\n", total_err,
        total_ops > 0 ? (float)total_err / total_ops * 100 : 0);
    printf("Total verification failures: %lu\n", total_verify);

    uint64_t checksum = calculate_tree_checksum();
    printf("Tree checksum: 0x%lx\n", (unsigned long)checksum);
}

// Writer thread - updates tree and shadow memory
void* writer_thread(void* arg)
{
    thread_stats_t* stats = (thread_stats_t*)arg;
    int thread_id = stats->thread_id;

    for (int i = 0; i < NUM_OPS; i++) {
        uintptr_t addr = rand() % ADDR_RANGE;
        uint32_t new_value = rand() % (MAX_VALUE + 1);

        // Update shadow memory first (as correct reference)
        shadow_set(addr, new_value);

        // Update the tree
        int ret = rb_set(tree, addr, addr + 1, new_value);

        stats->ops++;
        stats->writes++;

        // Log operation for replay
        log_operation(OP_SET, addr, addr + 1, new_value, thread_id);

        // Immediate verification
        uint32_t tree_val = rb_get(tree, addr);
        if (!verify_with_shadow(addr, tree_val, "immediate verify", thread_id)) {
            stats->errors++;
        }

        // Random verification of other addresses
        if (rand() % 10 == 0) {
            uintptr_t check_addr = rand() % ADDR_RANGE;
            uint32_t check_val = rb_get(tree, check_addr);
            if (!verify_with_shadow(check_addr, check_val, "random verify", thread_id)) {
                stats->verify_failures++;
            }
            stats->reads++;
        }

        atomic_fetch_add(&total_operations, 1);
    }

    return NULL;
}

// Reader thread - only reads and verifies
void* reader_thread(void* arg)
{
    thread_stats_t* stats = (thread_stats_t*)arg;
    int thread_id = stats->thread_id;

    for (int i = 0; i < NUM_OPS; i++) {
        uintptr_t addr = rand() % ADDR_RANGE;

        uint32_t tree_val = rb_get(tree, addr);

        stats->ops++;
        stats->reads++;

        if (!verify_with_shadow(addr, tree_val, "read verify", thread_id)) {
            stats->errors++;
        }

        // Read same address multiple times to check consistency
        if (rand() % 20 == 0) {
            uintptr_t repeat_addr = rand() % ADDR_RANGE;
            uint32_t first = rb_get(tree, repeat_addr);
            uint32_t second = rb_get(tree, repeat_addr);

            if (first != second) {
                pthread_mutex_lock(&print_mutex);
                printf("[Thread %d] Inconsistent reads: addr=0x%lx, first=%u, second=%u\n",
                    thread_id, (unsigned long)repeat_addr, first, second);
                stats->verify_failures++;
                pthread_mutex_unlock(&print_mutex);
            }
            stats->reads += 2;
        }

        atomic_fetch_add(&total_operations, 1);
    }

    return NULL;
}

// Mixed thread - does both reads and writes
void* mixed_thread(void* arg)
{
    thread_stats_t* stats = (thread_stats_t*)arg;
    int thread_id = stats->thread_id;

    for (int i = 0; i < NUM_OPS; i++) {
        if (rand() % 2 == 0) {
            // Write operation
            uintptr_t addr = rand() % ADDR_RANGE;
            uint32_t new_value = rand() % (MAX_VALUE + 1);

            shadow_set(addr, new_value);
            rb_set(tree, addr, addr + 1, new_value);

            stats->writes++;
            log_operation(OP_SET, addr, addr + 1, new_value, thread_id);

            // Verify immediately
            uint32_t tree_val = rb_get(tree, addr);
            if (!verify_with_shadow(addr, tree_val, "mixed write verify", thread_id)) {
                stats->errors++;
            }
        } else {
            // Read operation
            uintptr_t addr = rand() % ADDR_RANGE;
            uint32_t tree_val = rb_get(tree, addr);

            if (!verify_with_shadow(addr, tree_val, "mixed read verify", thread_id)) {
                stats->errors++;
            }
            stats->reads++;
        }

        stats->ops++;
        atomic_fetch_add(&total_operations, 1);
    }

    return NULL;
}

// Validator thread - continuously verifies the entire tree
void* validator_thread(void* arg)
{
    thread_stats_t* stats = (thread_stats_t*)arg;
    int thread_id = stats->thread_id;

    while (atomic_load(&total_operations) < (NUM_THREADS * 2 * NUM_OPS)) {
        // Verify a random range
        uintptr_t start = rand() % (ADDR_RANGE - 100);
        uintptr_t end = start + 100;

        for (uintptr_t addr = start; addr < end; addr++) {
            uint32_t tree_val = rb_get(tree, addr);
            if (!verify_with_shadow(addr, tree_val, "validator verify", thread_id)) {
                stats->errors++;
            }
            stats->reads++;
            stats->ops++;
        }

        usleep(1000); // Small delay to avoid overwhelming
    }

    return NULL;
}

int main(int argc, char* argv[])
{
    ftrace = stdout;
    srand(time(NULL));

    printf("========================================\n");
    printf("   Red-Black Tree Thread Safety Test    \n");
    printf("========================================\n");
    printf("Configuration:\n");
    printf("  Threads: %d\n", NUM_THREADS * 2);
    printf("  Operations per thread: %d\n", NUM_OPS);
    printf("  Address range: 0-%d\n", ADDR_RANGE - 1);
    printf("  Max value: %d\n", MAX_VALUE);
    printf("========================================\n\n");

    // Initialize
    printf("Initializing test environment...\n");
    tree = rbtree_init("test_tree");
    init_shadow();

    // Pre-populate with some data
    printf("Pre-populating data...\n");
    for (int i = 0; i < ADDR_RANGE; i += 10) {
        uint32_t val = i % (MAX_VALUE + 1);
        shadow_set(i, val);
        rb_set(tree, i, i + 5, val);
    }

    printf("Initial tree checksum: 0x%lx\n", (unsigned long)calculate_tree_checksum());

    // Create threads
    pthread_t threads[NUM_THREADS * 2];

    printf("\nStarting %d threads...\n", NUM_THREADS * 2);

    // Reset stats
    memset(thread_stats, 0, sizeof(thread_stats));

    for (int i = 0; i < NUM_THREADS * 2; i++) {
        thread_stats[i].thread_id = i;

        // Mix different types of threads
        if (i < NUM_THREADS) {
            // First half are writer threads
            pthread_create(&threads[i], NULL, writer_thread, &thread_stats[i]);
        } else if (i < NUM_THREADS * 3 / 2) {
            // Next quarter are reader threads
            pthread_create(&threads[i], NULL, reader_thread, &thread_stats[i]);
        } else {
            // Last quarter are mixed threads
            pthread_create(&threads[i], NULL, mixed_thread, &thread_stats[i]);
        }
    }

    // Wait for all threads to complete
    printf("Waiting for threads to complete...\n");
    for (int i = 0; i < NUM_THREADS * 2; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n=== Test Results ===\n");

    // Print statistics
    print_statistics();

    // Range consistency check
    range_consistency_check();

    // Final comprehensive verification
    printf("\n=== Final Comprehensive Verification ===\n");
    int final_errors = 0;
    int printed_errors = 0;

    for (uintptr_t addr = 0; addr < ADDR_RANGE; addr++) {
        uint32_t tree_val = rb_get(tree, addr);
        uint32_t shadow_val = shadow_get(addr);

        if (tree_val != shadow_val) {
            if (printed_errors < MAX_ERRORS_TO_PRINT) {
                printf("Final mismatch: addr=0x%lx, tree=%u, shadow=%u\n",
                    (unsigned long)addr, tree_val, shadow_val);
                printed_errors++;
            }
            final_errors++;
        }
    }

    printf("\nFinal verification results:\n");
    printf("  Total addresses: %d\n", ADDR_RANGE);
    printf("  Mismatches found: %d (%.2f%%)\n", final_errors,
        (float)final_errors / ADDR_RANGE * 100);

    if (final_errors == 0) {
        printf("All verifications passed! Tree is consistent!\n");
    } else {
        printf("Inconsistencies detected! Tree may be corrupted!\n");
    }

    // Replay verification
    replay_verify();

    // Print tree structure if small enough
    if (ADDR_RANGE <= 100) {
        printf("\nFinal tree structure:\n");
        rbtree_print(tree);
    }

    // Cleanup
    printf("\nCleaning up...\n");
    rbtree_delete(tree);
    free(shadow_memory);

    printf("\nTest completed. Total errors: %lu\n", atomic_load(&total_errors));

    return (final_errors == 0) ? 0 : 1;
}
#endif
