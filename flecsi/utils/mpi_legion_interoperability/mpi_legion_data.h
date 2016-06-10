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
#ifndef MPI_LEGION_DATA_HPP
#define MPI_LEGION_DATA_HPP

#include <iostream>
#include <string>
#include <cstdio>
#include <condition_variable>

#include "legion.h"
#include "realm.h"

#include "flecsi/utils/mpi_legion_interoperability/legion_arrays.h"
#include "flecsi/execution/mpilegion_execution_policy.h"
namespace flecsi
{
namespace mpilegion
{

template <typename Type,  uint64_t N>
class MPILegionArray{

 public:
  MPILegionArray(){};

 private: 
  bool mpi_allocated=true;
  
 public:

 LogicalArray<Type> legion_object;
 std::array<Type,N> mpi_object;

 void  allocate_legion(context_t<mpilegion_execution_policy_t>  &ctx)
       {
           legion_object.allocate(N, ctx.legion_ctx(),
                                     ctx.runtime());
      }

 void  deallocate_legion(context_t<mpilegion_execution_policy_t>  &ctx)
       {
           legion_object.deallocate( ctx.legion_ctx(),
                                     ctx.runtime());
       }

 void  partition_legion( context_t<mpilegion_execution_policy_t> &ctx)
       {
         legion_object.partition(N, ctx.legion_ctx(), ctx.runtime());
       }

// void allocate_mpi(void)
//      {
//        mpi_allocated=true;
//        //Type *mpi_object=new Type[numberOfElements];
//      }


 Type legion_accessor(const PhysicalRegion &physicalRegion,
                      Context ctx,
                      HighLevelRuntime *runtime)
      {
        PhysicalArray<Type> PS (physicalRegion, ctx, runtime);
        return PS.data();
 //       Type *data = PS.data();
  //      assert (data);
  //      return data;
      }

 Type *  mpi_accessor(void)
       {
        assert (mpi_allocated);
         return mpi_object.data();
       }

  Type mpi_ptr (void)
        {
          return *mpi_object;
        }

  Type legion_ptr (void)
        {
          return *legion_object.legion_accessor;
        }

  uint64_t size(void){ return N;}

  void copy_legion_to_mpi (context_t<mpilegion_execution_policy_t>  &ctx)
  {
    //TOFIX: should we use pointers for acc and mpi_object here?
   LegionRuntime::Accessor::RegionAccessor<LegionRuntime::Accessor::AccessorType::Generic, Type, Type> acc=
     legion_object.get_accessor(READ_ONLY, EXCLUSIVE, ctx.legion_ctx(), ctx.runtime());
   int count =0;
   for(GenericPointInRectIterator<1> pir(legion_object.bounds); pir; pir++){
    mpi_object[count] = acc.read(DomainPoint::from_point<1>(pir.p));
   count++;
   }
   legion_object.unmap_all_regions(ctx.legion_ctx(), ctx.runtime());
  }

  void copy_mpi_to_legion (context_t<mpilegion_execution_policy_t>  &ctx)
  {
   //TOFIX: should we use pointers for acc and mpi_object here?
     LegionRuntime::Accessor::RegionAccessor<LegionRuntime::Accessor::AccessorType::Generic, Type, Type> acc=
        legion_object.get_accessor(WRITE_DISCARD, EXCLUSIVE, ctx.legion_ctx(), ctx.runtime());
     int count=0;
     for(GenericPointInRectIterator<1> pir(legion_object.bounds); pir; pir++){
          acc.write(DomainPoint::from_point<1>(pir.p), mpi_object[count]);
          count++;
     }
    //TOFIX: instead unmap_all_regions we would like to call unmap only on used region 
    legion_object.unmap_all_regions(ctx.legion_ctx(), ctx.runtime());   
  }

 void dump_legion(const std::string &prefix,
                  int64_t nle,
                  context_t<mpilegion_execution_policy_t>  &ctx)
 {
  
   legion_object.dump(prefix, 1, ctx.legion_ctx(), ctx.runtime());
 }

};

}//end namespace mpilegion

}//end namespace flecsi


#endif
/*~-------------------------------------------------------------------------~-*
 * Formatting options
 * vim: set tabstop=2 shiftwidth=2 expandtab :
 *~-------------------------------------------------------------------------~-*/
