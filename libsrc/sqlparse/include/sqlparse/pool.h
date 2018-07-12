#ifndef __POOL_H__
#define __POOL_H__
#include <iostream>
#include <stdio.h>

using namespace std;
namespace pool {
  template<typename T>
    class simple_pool {
      public:
        simple_pool (size_t unit_size = 128, 
            size_t unit_count = 1024) : 
          unit_size_(unit_size),
          unit_count_(unit_count) {
            init();
          }
        void init () {
          try{
            Node* p = new Node;
            p->mem_ = new T[unit_size_];
            head_ = p;
            tail_ = head_;
            size_t count = 1;
            for(size_t i=0; i<unit_count_-1; ++i) {
              p->next_ = new Node;
              p = p->next_;
              p->mem_ = new T[unit_size_];
              count++;
            }
            p->next_ = NULL;
          } catch( std::bad_alloc &ba) {
            std::cerr << "bad_alloc caught: " << ba.what() << '\n';
          }
        }

        ~simple_pool () {
          Node* head = head_;
          Node* p = head;
          size_t count = 0;
          while (p != NULL) {
            p = p->next_;
            if(head != NULL) {
              if(head->mem_ != NULL) {
                delete [] head->mem_;
                head->mem_ = NULL;
              }
              delete head;
              head = NULL;
              count++;
            }
            head = p;
          }
        }

        T* simple_pool_malloc(int length = 0) {
          if (tail_->next_ == NULL) {
            Node* p = new Node;
            p->mem_ = new T[unit_size_]; 
            p->next_ = NULL;
            tail_->next_ = p;
            p = NULL;
          }
          Node* p = tail_;
          tail_ = tail_->next_;
          if (length > unit_size_) {
            if(p->mem_ != NULL) {
              delete [] p->mem_;
              p->mem_ = NULL;
            }
            p->mem_ = new T[length]; 
          }
          return p->mem_;
        }

        size_t simple_pool_used() {
          Node* p = head_;
          size_t cout = 0;
          while (p != tail_) {
            p=p->next_;
            cout++;
          }
          return cout;
        }

        size_t simple_pool_unused() {
          Node* p = tail_;
          size_t cout = 0;
          while (p != NULL) {
            p=p->next_;
            cout++;
          }
          return cout;
        }

      private:
        simple_pool(simple_pool&  st) ;
        void operator=(simple_pool& st);
        typedef struct node {
          T* mem_;
          struct node *next_;
        } Node;
        size_t unit_size_;
        size_t unit_count_;
        Node* tail_;
        Node* head_;
    };
};
#endif
