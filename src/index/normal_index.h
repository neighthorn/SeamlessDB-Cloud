#pragma once

#include "ix_defs.h"
#include "ix_index_handle.h"
#include "transaction/transaction.h"


/* 管理B+树中的每个节点 */
class NMIxNodeHandle {
    friend class NMIxIndexHandle;
    friend class IxScan;

   private:
    const IxFileHdr *file_hdr;      // 节点所在文件的头部信息
    Page *page;                     // 存储节点的页面
    IxPageHdr *page_hdr;            // page->data的第一部分，指针指向首地址，长度为sizeof(IxPageHdr)
    char *keys;                     // page->data的第二部分，指针指向首地址，长度为file_hdr->keys_size，每个key的长度为file_hdr->col_len
    Rid *rids;                      // page->data的第三部分，指针指向首地址

   public:
    IxNodeHandle() = default;

    IxNodeHandle(const IxFileHdr *file_hdr_, Page *page_) : file_hdr(file_hdr_), page(page_) {
        page_hdr = reinterpret_cast<IxPageHdr *>(page->get_data());
        keys = page->get_data() + sizeof(IxPageHdr);
        rids = reinterpret_cast<Rid *>(keys + file_hdr->keys_size_);
    }

    int get_size() { return page_hdr->num_key; }

    void set_size(int size) { page_hdr->num_key = size; }

    int get_max_size() { return file_hdr->btree_order_ + 1; }

    int get_min_size() { return get_max_size() / 2; }

    int key_at(int i) { return *(int *)get_key(i); }

    /* 得到第i个孩子结点的page_no */
    page_id_t value_at(int i) { return get_rid(i)->page_no; }

    page_id_t get_page_no() { return page->get_page_id().page_no; }

    PageId get_page_id() { return page->get_page_id(); }

    page_id_t get_next_leaf() { return page_hdr->next_leaf; }

    page_id_t get_prev_leaf() { return page_hdr->prev_leaf; }

    page_id_t get_parent_page_no() { return page_hdr->parent; }

    bool is_leaf_page() { return page_hdr->is_leaf; }

    bool is_root_page() { return get_parent_page_no() == INVALID_PAGE_ID; }

    void set_next_leaf(page_id_t page_no) { page_hdr->next_leaf = page_no; }

    void set_prev_leaf(page_id_t page_no) { page_hdr->prev_leaf = page_no; }

    void set_parent_page_no(page_id_t parent) { page_hdr->parent = parent; }

    char *get_key(int key_idx) const { return keys + key_idx * file_hdr->col_tot_len_; }

    Rid *get_rid(int rid_idx) const { return &rids[rid_idx]; }

    void set_key(int key_idx, const char *key) { memcpy(keys + key_idx * file_hdr->col_tot_len_, key, file_hdr->col_tot_len_); }

    void set_rid(int rid_idx, const Rid &rid) { rids[rid_idx] = rid; }

    int lower_bound(const char *target) const;

    int upper_bound(const char *target) const;

    void insert_pairs(int pos, const char *key, const Rid *rid, int n);

    page_id_t internal_lookup(const char *key);

    bool leaf_lookup(const char *key, Rid **value);

    int insert(const char *key, const Rid &value);

    // 用于在结点中的指定位置插入单个键值对
    void insert_pair(int pos, const char *key, const Rid &rid) { insert_pairs(pos, key, &rid, 1); }

    void erase_pair(int pos);

    int remove(const char *key);

    /**
     * @brief used in internal node to remove the last key in root node, and return the last child
     *
     * @return the last child
     */
    page_id_t remove_and_return_only_child() {
        assert(get_size() == 1);
        page_id_t child_page_no = value_at(0);
        erase_pair(0);
        assert(get_size() == 0);
        return child_page_no;
    }

    /**
     * @brief 由parent调用，寻找child，返回child在parent中的rid_idx∈[0,page_hdr->num_key)
     * @param child
     * @return int
     */
    int find_child(IxNodeHandle *child) {
        int rid_idx;
        for (rid_idx = 0; rid_idx < page_hdr->num_key; rid_idx++) {
            if (get_rid(rid_idx)->page_no == child->get_page_no()) {
                break;
            }
        }
        assert(rid_idx < page_hdr->num_key);
        return rid_idx;
    }
};

/* B+树 */
class IxIndexHandle {
    friend class IxScan;
    friend class IxManager;

   private:
    DiskManager *disk_manager_;
    BufferPoolManager *buffer_pool_manager_;
    int fd_;                                    // 存储B+树的文件
    IxFileHdr* file_hdr_;                       // 存了root_page，但其初始化为2（第0页存FILE_HDR_PAGE，第1页存LEAF_HEADER_PAGE）
    std::mutex root_latch_;

   public:
    IxIndexHandle(DiskManager *disk_manager, BufferPoolManager *buffer_pool_manager, int fd);

    // for search
    bool get_value(const char *key, std::vector<Rid> *result, Transaction *transaction);

    std::pair<IxNodeHandle *, bool> find_leaf_page(const char *key, Operation operation, Transaction *transaction,
                                                 bool find_first = false);

    // for insert
    page_id_t insert_entry(const char *key, const Rid &value, Transaction *transaction);

    IxNodeHandle *split(IxNodeHandle *node);

    void insert_into_parent(IxNodeHandle *old_node, const char *key, IxNodeHandle *new_node, Transaction *transaction);

    // for delete
    bool delete_entry(const char *key, Transaction *transaction);

    bool coalesce_or_redistribute(IxNodeHandle *node, Transaction *transaction = nullptr,
                                bool *root_is_latched = nullptr);
    bool adjust_root(IxNodeHandle *old_root_node);

    void redistribute(IxNodeHandle *neighbor_node, IxNodeHandle *node, IxNodeHandle *parent, int index);

    bool coalesce(IxNodeHandle **neighbor_node, IxNodeHandle **node, IxNodeHandle **parent, int index,
                  Transaction *transaction, bool *root_is_latched);

    Iid lower_bound(const char *key);

    Iid upper_bound(const char *key);

    Iid leaf_end() const;

    Iid leaf_begin() const;

   private:
    // 辅助函数
    void update_root_page_no(page_id_t root) { file_hdr_->root_page_ = root; }

    bool is_empty() const { return file_hdr_->root_page_ == IX_NO_PAGE; }

    // for get/create node
    IxNodeHandle *fetch_node(int page_no) const;

    IxNodeHandle *create_node();

    // for maintain data structure
    void maintain_parent(IxNodeHandle *node);

    void erase_leaf(IxNodeHandle *leaf);

    void release_node_handle(IxNodeHandle &node);

    void maintain_child(IxNodeHandle *node, int child_idx);

    // for index test
    Rid get_rid(const Iid &iid) const;
};