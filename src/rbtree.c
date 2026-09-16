enum color {
    RED,
    BLACK
};

struct rb_node {
    char *key;
    void *value;

    enum color color;

    struct rb_node *leftl;
    struct rb_node *right;
    struct rb_node *parent;

};

struct rbtree {
    struct rb_node *root;
    size_t size;
    rb_value_free_fn value_free;
};

