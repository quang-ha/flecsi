/*~--------------------------------------------------------------------------~*
 *  @@@@@@@@  @@           @@@@@@   @@@@@@@@ @@
 * /@@/////  /@@          @@////@@ @@////// /@@
 * /@@       /@@  @@@@@  @@    // /@@       /@@
 * /@@@@@@@  /@@ @@///@@/@@       /@@@@@@@@@/@@
 * /@@////   /@@/@@@@@@@/@@       ////////@@/@@
 * /@@       /@@/@@//// //@@    @@       /@@/@@
 * /@@       @@@//@@@@@@ //@@@@@@  @@@@@@@@ /@@
 * //       ///  //////   //////  ////////  //
 *
 * Copyright (c) 2016 Los Alamos National Laboratory, LLC
 * All rights reserved
 *~--------------------------------------------------------------------------~*/

#ifndef flecsi_structured_index_space_h
#define flecsi_structured_index_space_h

#include <cassert>
#include <algorithm>
#include <type_traits>
#include <vector>

#include "flecsi/topology/structured_querytable.h"

namespace flecsi {
namespace topology {

//E is entity type
//DM is mesh dimension
template<class E, size_t DM=0>
class structured_index_space{
public:
  using id_t        = typename std::remove_pointer<E>::type::id_t;
  using id_vector_t = typename std::conditional<DM==0, 
                               std::vector<id_t>, 
                               std::array<id_t,DM>>::type;
  using id_array_t  = std::vector<std::vector<id_t>>;

 /******************************************************************************
 *               Constructors/Destructors/Initializations                      *    
 * ****************************************************************************/ 
  void init(bool primary, const id_vector_t &lbnds, const id_vector_t &ubnds, 
            id_array_t &mubnds)
  {
    assert(lbnds.size() == ubnds.size());
    // this check is to ensure that the primary IS doesn't have 
    // multiple boxes
    if (primary) 
      assert (mubnds.size()==1);

    dim_ = lbnds.size();
    offset_ = 0;
    primary_ = primary;
    num_boxes_ = mubnds.size();
    std::vector<size_t> ubnds_new, lbnds_new;

    for (size_t i = 0; i < num_boxes_; i++)
    {
      size_t cnt = 1;
      ubnds_new.clear();
      lbnds_new.clear();
      for (size_t j = 0; j < dim_; j++)
      {
         cnt *= ubnds[j]+mubnds[i][j]-lbnds[j]+1;
         ubnds_new.push_back(ubnds[j]+mubnds[i][j]);
         lbnds_new.push_back(lbnds[j]);
      }

      box_offset_[i] = 0;
      box_size_[i] = cnt;
      box_lowbnds_.push_back(lbnds_new);
      box_upbnds_.push_back(ubnds_new);
    }

    size_ = 0;
    for (size_t i = 0; i < num_boxes_; i++)
     size_ += box_size_[i];

    //debug print
    for (size_t i = 0; i < num_boxes_; i++)
    {
      std::cout<<"Box-id = "<<i<<std::endl;
      std::cout<<" -- Box-offset = "<<box_offset_[i]<<std::endl;
      std::cout<<" -- Box-size   = "<<box_size_[i]<<std::endl;

      std::cout<<" ----Box-lower-bnds = { ";
      for (size_t j = 0 ; j < dim_; j++)
        std::cout<<box_lowbnds_[i][j]<<", ";
      std::cout<<" }"<<std::endl;
    
      std::cout<<" ----Box-upper-bnds = { ";
      for (size_t j = 0 ; j < dim_; j++)
        std::cout<<box_upbnds_[i][j]<<", ";
      std::cout<<" }"<<std::endl;
    }
  }
   
  //default constructor
  structured_index_space(){};

  //default destructor
  ~structured_index_space(){};

 /******************************************************************************
 *                              Basic Iterators                                *    
 * ****************************************************************************/ 
 template<typename S=E>
 auto iterate()
 { 
   return iterator_whole_t<S>(offset_, size_);
 }

 template< typename S = E>
 class iterator_whole_t
 {
    public:
      iterator_whole_t(id_t start, id_t sz):start_{start}, sz_{sz}{};
     ~iterator_whole_t(){};

      class iterator_t{
        public:
          iterator_t (id_t offset):current{offset}
          {
            current_ent = new S;
            current_ent->set_id(current,0);
          };
    
          ~iterator_t()
           {
             delete current_ent;
           };

          iterator_t& operator++()
          {
            ++current;
            current_ent->set_id(current,0);
            return *this;
          }

          bool operator!=(const iterator_t& rhs)
          {
           return (this->current != rhs.current);
          } 
          
          bool operator==(const iterator_t& rhs)
          {
           return (this->current == rhs.current);
          } 

          S& operator*()
          {
           return *current_ent;
          }

       private:
        id_t current;
        S* current_ent;
     };
    
    auto begin()
    {
      return iterator_t(start_);
    };

    auto end()
    {
      return iterator_t(start_+sz_);
    }; 
 
   private:
    id_t start_;
    id_t sz_; 
 };
  

 /******************************************************************************
 *                         Query-Specific Iterators                            *    
 * ****************************************************************************/ 
  template <size_t TD, class S>
  auto traverse(size_t FD, size_t ID, id_vector_t &indices)
  {
    return traversal<TD, S>(this, dim_, FD, ID, indices);
  }

  template<size_t TD1, class S1, class E1=E, size_t DM1 = DM>
  class traversal{
    public:
    
    //Constructor//Destructor
    traversal(
      structured_index_space<E1, DM1> *is,
      id_t MD1, 
      id_t FD1, 
      id_t ID1, 
      id_vector_t &indices):
      is_{is}, 
      MD1_{MD1}, 
      FD1_{FD1}, 
      ID1_{ID1}, 
      indices_{indices}
    {
      TD1_ = TD1;
      auto qt = query::qtable(MD1_);
      id_t nq = qt->entry[FD1_][ID1_][TD1_].size();
      start_ = 0;
      finish_ = nq;
    };

    ~traversal(){};
 
    //Iterator
    template<class S2 = S1, class E2 = E1, size_t DM2 = DM1>
    class iterator_t{
     public:
      iterator_t(
        structured_index_space<E2,DM2> *is, 
        id_t MD2, 
        id_t FD2, 
        id_t ID2, 
        id_t TD2,
        id_vector_t &indices, 
        id_t index,
        id_t end_idx, 
        bool forward):
        is_{is}, 
        MD2_{MD2}, 
        FD2_{FD2}, 
        ID2_{ID2},
        TD2_{TD2}, 
        indices_{indices}, 
        valid_idx_{index}, 
        end_idx_{end_idx},
        forward_{forward}
      {
        bool valid = isvalid(valid_idx_); 
        while (!valid)
        {
          if (forward) 
             valid_idx_ += 1; 
          else
             valid_idx_ -= 1;
          valid = isvalid(valid_idx_);
        }

        bool valid_end = isvalid(end_idx_ - 1);
        while (!valid_end)
        {
           end_idx_ -= 1;
           valid_end = isvalid(end_idx_ - 1);
        }
          
        id_t entid = compute_id(valid_idx_);
        valid_ent_ = new S2; 
        valid_ent_->set_id(entid,0);
      };

      ~iterator_t()
       {
         delete valid_ent_;
       };

      iterator_t& operator++()
      {
        valid_idx_ += 1;
        if(valid_idx_ != end_idx_){
          bool valid = isvalid(valid_idx_); 
          while ((!valid) && (valid_idx_ != end_idx_))
          {
            valid_idx_ += 1;
            //if (valid_idx_ != end_idx_) 
               valid = isvalid(valid_idx_);
            //else
              // break;
          }
         
          if (valid_idx_ != end_idx_)
          {
            id_t entid = compute_id(valid_idx_);
            valid_ent_->set_id(entid,0);
          }
        }
        return *this;
      };

      bool operator!=(const iterator_t& rhs)
      {
         return (this->valid_idx_ != rhs.end_idx_);
      };

      bool operator==(const iterator_t& rhs)
      {
         return (this->valid_idx_ == rhs.valid_idx_);
      };


      S2& operator*()
      {
        //S2* ent;
        //id_t entid = compute_id(this->valid_idx_);
        //ent->set_id(entid,0);
        //return *ent;
        return *valid_ent_;
      };

      bool isvalid(id_t vindex)
      {
        bool valid = true; 
        auto qt = query::qtable(MD2_);
        
        // Get box id 
        id_t bid = qt->entry[FD2_][ID2_][TD2_].adjacencies[vindex].box_id;
 
        // Get number of directions to check
        id_t nchk    = qt->entry[FD2_][ID2_][TD2_].adjacencies[vindex].numchk;
        auto dir     = qt->entry[FD2_][ID2_][TD2_].adjacencies[vindex].dir; 
        auto chk_bnd = qt->entry[FD2_][ID2_][TD2_].adjacencies[vindex].bnd_chk;
 
        for (id_t i = 0; i < nchk; i++)
        {
          if (chk_bnd[i])
            valid = valid && (indices_[dir[i]] <= (is_->max(bid, dir[i])));
          else 
            valid = valid && (indices_[dir[i]] >= (is_->min(bid, dir[i]))+1);  
        }
   
        return valid;
      }

      auto compute_id(id_t vindex)
      { 
        id_vector_t adj;
        auto qt = query::qtable(MD2_);
        id_t bid = qt->entry[FD2_][ID2_][TD2_].adjacencies[vindex].box_id;
        auto offset = qt->entry[FD2_][ID2_][TD2_].adjacencies[vindex].offset;
        for (id_t i = 0; i < MD2_; i++)
          adj[i] = indices_[i]+offset[i];     

        return is_->get_global_offset_from_indices(bid,adj);
      }

     private:
       structured_index_space<E2,DM2> *is_; 
       id_t MD2_, FD2_, ID2_, TD2_;
       id_vector_t indices_;
       id_t valid_idx_;
       id_t end_idx_;
       S2*  valid_ent_;
       bool forward_; 
    };

    auto begin()
    {
      return iterator_t<S1, E1, DM1>(is_, MD1_, FD1_, ID1_, TD1_, indices_,
                                 start_, finish_, true); 
    };

    auto end()
    {
      return iterator_t<S1, E1, DM1>(is_, MD1_, FD1_, ID1_, TD1_, indices_, 
                                 finish_-1, finish_, false);
    };

    private: 
       structured_index_space<E1, DM1> *is_; 
       id_t MD1_, FD1_, ID1_, TD1_;
       id_vector_t indices_;
       id_t start_, finish_; 
  };

 /******************************************************************************
 *          Offset --> Indices & Indices --> Offset Routines                   *    
 * ****************************************************************************/ 

   // Return the global offset id w.r.t the box id B  
  template<size_t B>
  id_t get_global_offset_from_indices(id_vector_t &idv)
  {
    id_t lval = get_local_offset_from_indices<B>(idv);
    
    for (id_t i = 1; i < B; i++)
      lval += box_size_[i-1];
    return lval;
  }
  
  id_t get_global_offset_from_indices(id_t B, id_vector_t &idv)
  {
    id_t lval = get_local_offset_from_indices(B, idv);
    
    for (id_t i = 1; i <= B; i++)
      lval += box_size_[i-1];
    return lval;
  }

  // Return the local offset id w.r.t the box id B
  template<size_t B>
  id_t get_local_offset_from_indices(id_vector_t &idv) 
  {
    //add range check for idv to make sure it lies within the bounds
    size_t value =0;
    size_t factor;

    for (int i = 0; i <dim_; ++i)
    {
      factor = 1;
      for (int j=0; j<dim_-i-1; ++j)
      {
        factor *= box_upbnds_[B][j]-box_lowbnds_[B][j]+1;
      }
      value += idv[dim_-i-1]*factor;
    }

    return value;
  }
  
  id_t get_local_offset_from_indices(size_t B, id_vector_t &idv) 
  {
    //add range check for idv to make sure it lies within the bounds
    size_t value =0;
    size_t factor;

    for (int i = 0; i <dim_; ++i)
    {
      factor = 1;
      for (int j=0; j<dim_-i-1; ++j)
      {
        factor *= box_upbnds_[B][j]-box_lowbnds_[B][j]+1;
      }
      value += idv[dim_-i-1]*factor;
    }

    return value;
  }


  auto get_indices_from_offset(id_t offset)
  {
    //Find the box from which the indices have to be computed
    auto box_id = 0;
    if (num_boxes_ > 0)
       box_id = find_box_id(offset);

    id_vector_t id;
    size_t factor;
    size_t rem = offset, value;

    for (int i=0; i<dim_; ++i)
    {
      factor = 1; 
      for (int j=0; j<dim_-i-1; ++j)
      {
       factor *= box_upbnds_[box_id][j]-box_lowbnds_[box_id][j] + 1; 
      }
      value = rem/factor;
      id[dim_-i-1] = value;
      rem -= value*factor;
    }
 
   return id;
  }

  auto find_box_id(id_t offset)
  {
    //assert(num_boxes>0);
    size_t bid = 0, low = 0, up = box_size_[0];
    for (size_t i = 0; i < num_boxes_; i++)
    {
      if (offset >= low && offset < up)
      {
        bid = i;
        break;
      }

      low = low + up;
      up  = up + box_size_[i+1];
    }
    return bid;
  }

  // Return size along direction D of box B in IS.
  template<size_t B, size_t D>
  auto get_size_in_direction()
  {
    assert(D>=0 && D <dim_);
    return (box_upbnds_[B][D] - box_lowbnds_[B][D]+1);
  }

  // Check if input index is between bounds along direction
  // D of box B. 
  template<size_t B, size_t D>
  bool check_index_limits(size_t index)
  {
    return (index >= box_lowbnds_[B][D] && index <= box_upbnds_[B][D]);
  }

  // Return upper bound along direction D of box B
  template<size_t B, size_t D>
  auto max()
  {
    auto val = box_upbnds_[B][D];
    return val;
  }
  
  auto max(size_t B, size_t D)
  {
    auto val = box_upbnds_[B][D];
    return val;
  }


  // Return lower bound along direction D of box B
  template<size_t B, size_t D>
  auto min()
  {
    auto val = box_lowbnds_[B][D];
    return val;
  }

  auto min(size_t B, size_t D)
  {
    auto val = box_lowbnds_[B][D];
    return val;
  }
  size_t size() const 
  {
    return size_;
  };
 
 private:
   bool primary_;               // primary_ is set to true only for the IS of 
                                // highest-dimensional entity 
   id_t dim_;                   // dimension of mesh this IS stores
   id_t offset_;                // starting offset for the entire IS
   id_t size_;                  // size of the entire IS

   id_t        num_boxes_;      // number of boxes in this IS
   id_vector_t box_size_;       // total number of entities in each box
   id_vector_t box_offset_;     // starting offset of each box
   id_array_t  box_lowbnds_;    // lower bounds of each box
   id_array_t  box_upbnds_;     // upper bounds of each box

};
} // namespace topology
} // namespace flecsi

#endif // flecsi_structured_index_space_h

/*~-------------------------------------------------------------------------~-*
 * Formatting options
 * vim: set tabstop=2 shiftwidth=2 expandtab :
 *~-------------------------------------------------------------------------~-*/
