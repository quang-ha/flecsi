/*~-------------------------------------------------------------------------~~*
 * Copyright (c) 2014 Los Alamos National Security, LLC
 * All rights reserved.
 *~-------------------------------------------------------------------------~~*/

#include "flecsi/execution/legion/internal_task.h"
#include "cinchtest.h"

///
/// \file
/// \date Initial file creation: Apr 01, 2017
///

namespace flecsi {
namespace execution {

// Define a Legion task to register.
int internal_task_example_1(const LegionRuntime::HighLevel::Task * task,
  const std::vector<LegionRuntime::HighLevel::PhysicalRegion> & regions,
  LegionRuntime::HighLevel::Context context,
  LegionRuntime::HighLevel::HighLevelRuntime * runtime) 
{
  std::cout <<"inside of the task1" <<std::endl;
} // internal_task_example

// Register the task. The task id is automatically generated.
__flecsi_internal_register_legion_task(internal_task_example_1, loc,
  single);

// Define a Legion task to register.
int internal_task_example_2(const LegionRuntime::HighLevel::Task * task,
  const std::vector<LegionRuntime::HighLevel::PhysicalRegion> & regions,
  LegionRuntime::HighLevel::Context context,
  LegionRuntime::HighLevel::HighLevelRuntime * runtime)
{
  std::cout <<"inside of the task2" <<std::endl;
} // internal_task_example

// Register the task. The task id is automatically generated.
__flecsi_internal_register_legion_task(internal_task_example_2, loc,
  index);

//__flecsi_internal_register_legion_task(internal_task_example_2, loc,
//  single);

void driver(int argc, char ** argv) {

  // These keys will allow you to lookup the task id that was assigned.
  auto key_1 = __flecsi_internal_task_key(internal_task_example_1, loc);
  auto key_2 = __flecsi_internal_task_key(internal_task_example_2, loc);
//  auto key_3 = __flecsi_internal_task_key(internal_task_example_2, loc);

  // Lookup the task ids.
  auto tid_1 = context_t::instance().task_id(key_1);
  auto tid_2 = context_t::instance().task_id(key_2);
//  auto tid_3 = context_t::instance().task_id(key_3);

  clog(info) << "Task ID: " << tid_1 << std::endl;
  clog(info) << "Task ID: " << tid_2 << std::endl;
//  clog(info) << "Task ID: " << tid_3 << std::endl;

  ASSERT_EQ(tid_1, 7);
  ASSERT_EQ(tid_2, 8);
//  ASSERT_EQ(tid_3, 9);

  context_t & context_ = context_t::instance();
  size_t task_key = utils::const_string_t{"driver"}.hash();
  auto runtime = context_.runtime(task_key);
  auto context = context_.context(task_key);

  //executing legion tasks:
  LegionRuntime::HighLevel::TaskLauncher launcher(
    context_t::instance().task_id(key_1),
    LegionRuntime::HighLevel::TaskArgument(0,0));
  auto f=runtime->execute_task(context, launcher);

  Legion::ArgumentMap arg_map;
  Legion::IndexLauncher index_launcher(
    context_t::instance().task_id(key_2),
    Legion::Domain::from_rect<1>(context_t::instance().all_processes()),
    Legion::TaskArgument(0, 0),
    arg_map
  );
 auto fm = runtime->execute_index_space(context, index_launcher);

} // driver

} // namespace execution
} // namespace flecsi

/*~------------------------------------------------------------------------~--*
 * Formatting options for vim.
 * vim: set tabstop=2 shiftwidth=2 expandtab :
 *~------------------------------------------------------------------------~--*/
