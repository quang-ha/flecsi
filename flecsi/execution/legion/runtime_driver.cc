/*~-------------------------------------------------------------------------~~*
 * Copyright (c) 2014 Los Alamos National Security, LLC
 * All rights reserved.
 *~-------------------------------------------------------------------------~~*/

//----------------------------------------------------------------------------//
//! \file
//! \date Initial file creation: Jul 26, 2016
//----------------------------------------------------------------------------//

#include "flecsi/execution/legion/runtime_driver.h"

#include <legion.h>

#include "flecsi/data/storage.h"
#include "flecsi/execution/context.h"
#include "flecsi/execution/legion/legion_tasks.h"
#include "flecsi/execution/legion/mapper.h"
#include "flecsi/execution/legion/internal_field.h"
#include "flecsi/utils/common.h"

#include <legion_utilities.h>

clog_register_tag(runtime_driver);

namespace flecsi {
namespace execution {

//----------------------------------------------------------------------------//
// Implementation of FleCSI runtime driver task.
//----------------------------------------------------------------------------//

void
runtime_driver(
  const Legion::Task * task,
  const std::vector<Legion::PhysicalRegion> & regions,
  Legion::Context ctx,
  Legion::HighLevelRuntime * runtime
)
{
  {
  clog_tag_guard(runtime_driver);
  clog(info) << "In Legion runtime driver" << std::endl;
  }

  // Get the input arguments from the Legion runtime
  const Legion::InputArgs & args =
    Legion::HighLevelRuntime::get_input_args();

  // Initialize MPI Interoperability
  context_t & context_ = context_t::instance();
  context_.connect_with_mpi(ctx, runtime);
  context_.wait_on_mpi(ctx, runtime);

#if defined FLECSI_ENABLE_SPECIALIZATION_DRIVER
  {
  clog_tag_guard(runtime_driver);
  clog(info) << "Executing specialization driver task" << std::endl;
  }

  // Set the current task context to the driver
  context_.push_state(utils::const_string_t{"specialization_driver"}.hash(),
    ctx, runtime, task, regions);

  // run default or user-defined specialization driver 
  specialization_driver(args.argc, args.argv);

  // Set the current task context to the driver
  context_.pop_state( utils::const_string_t{"specialization_driver"}.hash());
#endif // FLECSI_ENABLE_SPECIALIZATION_DRIVER

  // Register user data
  data::storage_t::instance().register_all();

  // FIXME auto, auto, auto, second, first tells nothing of what this code does
  auto & data_client_registry =
    flecsi::data::storage_t::instance().data_client_registry(); 

  for(auto & c: data_client_registry) {
    for(auto & d: c.second) {
      d.second.second(d.second.first);
    } // for
  } // for

  int num_colors;
  MPI_Comm_size(MPI_COMM_WORLD, &num_colors);
  {
  clog_tag_guard(runtime_driver);
  clog(info) << "MPI num_colors is " << num_colors << std::endl;
  }


  std::set<size_t> map_handles;
  std::map<size_t, Legion::IndexSpace> expanded_ispaces_map;
  std::map<size_t, Legion::FieldSpace> expanded_fspaces_map;
  std::map<size_t, Legion::LogicalRegion> expanded_lregions_map;
  std::map<size_t, Legion::IndexPartition> color_iparts_map;
  std::map<size_t, std::vector<Legion::PhaseBarrier>> phase_barriers_map;

  LegionRuntime::Arrays::Rect<1> color_bounds(0, num_colors - 1);
  Legion::Domain color_domain = Legion::Domain::from_rect<1>(color_bounds);

  auto coloring_info = context_.coloring_info_map();
  const std::unordered_map<size_t, flecsi::coloring::index_coloring_t> coloring_map
    = context_.coloring_map();

  auto ghost_owner_pos_fid = 
    LegionRuntime::HighLevel::FieldID(internal_field::ghost_owner_pos);

  for(auto handle_idx: coloring_info) {
    for(auto color_idx: handle_idx.second) {
      flecsi::coloring::coloring_info_t color_info = handle_idx.second[color_idx.first];
      clog(error) << " Kolor " << color_idx.first << " Handle " << handle_idx.first << " has " <<
          color_info.ghost_owners.size() << " ghost owners" << std::endl;
      for (auto owner : color_info.ghost_owners)
        clog(error) << owner << std::endl;

    } // color_idx
  } // handle idx

  {
  clog_tag_guard(runtime_driver);

  for(auto handle_idx: coloring_info) {
    // Create expanded IndexSpace
    map_handles.insert(handle_idx.first);

    // Determine max size of a color partition
    size_t total_num_entities = 0;
    for(auto color_idx: handle_idx.second) {
      clog(trace) << "index: " << handle_idx.first << " color: " << color_idx.first << " " << color_idx.second << std::endl;
      total_num_entities = std::max(total_num_entities,
          color_idx.second.exclusive + color_idx.second.shared + color_idx.second.ghost);
    } // for color_idx
    clog(trace) << "total_num_entities " << total_num_entities << std::endl;

    // Create expanded index space
    LegionRuntime::Arrays::Rect<2> expanded_bounds = LegionRuntime::Arrays::Rect<2>(
        LegionRuntime::Arrays::Point<2>::ZEROES(),
        LegionRuntime::Arrays::make_point(num_colors,total_num_entities));
    Legion::Domain expanded_dom(Legion::Domain::from_rect<2>(expanded_bounds));
    Legion::IndexSpace expanded_is = runtime->create_index_space(ctx, expanded_dom);
    char buf[80];
    sprintf(buf, "expanded index space %ld", handle_idx.first);
    runtime->attach_name(expanded_is, buf);
    expanded_ispaces_map[handle_idx.first] = expanded_is;

    // Read user + FleCSI registered field spaces
    Legion::FieldSpace expanded_fs = runtime->create_field_space(ctx);

    Legion::FieldAllocator allocator = 
      runtime->create_field_allocator(ctx, expanded_fs);

    // temporary fix until we add registration of internal fields
    allocator.allocate_field(sizeof(LegionRuntime::Arrays::Point<2>), 
      ghost_owner_pos_fid);

    // Get field info for this index space
    auto fitr = context_.field_info_map().find(handle_idx.first);
 
    if(fitr != context_.field_info_map().end())
    {
      auto& field_map = fitr->second;

      // Allocate all fields on this index space
      for(auto& aitr : field_map){
        const context_t::field_info_t& fi = aitr.second;
        allocator.allocate_field(fi.size, aitr.first);
      }
    }

    sprintf(buf, "expanded field space %ld", handle_idx.first);
    runtime->attach_name(expanded_fs, buf);
    expanded_fspaces_map[handle_idx.first] = expanded_fs;

    Legion::LogicalRegion expanded_lr = runtime->create_logical_region(ctx,
        expanded_is, expanded_fs);
    sprintf(buf, "expanded logical region %ld", handle_idx.first);
    runtime->attach_name(expanded_lr, buf);
    expanded_lregions_map[handle_idx.first] = expanded_lr;

    // Partition expanded IndexSpace color-wise & create associated PhaseBarriers
    Legion::DomainColoring color_partitioning;
    for (int color = 0; color < num_colors; color++) {
      flecsi::coloring::coloring_info_t color_info = handle_idx.second[color];
      LegionRuntime::Arrays::Rect<2> subrect(
          LegionRuntime::Arrays::make_point(color, 0),
          LegionRuntime::Arrays::make_point(color,
            color_info.exclusive + color_info.shared + color_info.ghost - 1));
      color_partitioning[color] = Legion::Domain::from_rect<2>(subrect);
      phase_barriers_map[handle_idx.first].push_back(runtime->create_phase_barrier(ctx,
          1 + color_info.shared_users.size()));
      clog(trace) << handle_idx.first << " phase barrier " << color << " has " <<
          color_info.shared_users.size() + 1 << " arrivers" << std::endl;
    }

    Legion::IndexPartition color_ip = runtime->create_index_partition(ctx,
        expanded_is, color_domain, color_partitioning, true /*disjoint*/);
    sprintf(buf, "color partitioing %ld", handle_idx.first);
    runtime->attach_name(color_ip, buf);
    color_iparts_map[handle_idx.first] = color_ip;
  } // for handle_idx
  } // clog_tag_guard

  // Map between pre-compacted and compacted data placement
  const auto compaction_id =
    context_.task_id<__flecsi_internal_task_key(compaction_task)>();
  Legion::IndexLauncher compaction_launcher(compaction_id, color_domain,
      Legion::TaskArgument(nullptr, 0), Legion::ArgumentMap());
  compaction_launcher.tag = MAPPER_FORCE_RANK_MATCH;

  for(auto handle : map_handles) {
    Legion::LogicalPartition color_lp = runtime->get_logical_partition(ctx,
        expanded_lregions_map[handle], color_iparts_map[handle]);
    compaction_launcher.add_region_requirement(
        Legion::RegionRequirement(color_lp, 0/*projection ID*/,
            WRITE_DISCARD, EXCLUSIVE, expanded_lregions_map[handle]))
                .add_field(ghost_owner_pos_fid);
  } // for handle
  runtime->execute_index_space(ctx, compaction_launcher);

  // Must epoch launch
  Legion::MustEpochLauncher must_epoch_launcher;

  std::vector<Legion::Serializer> args_serializers(num_colors);

  const auto spmd_id =
    context_.task_id<__flecsi_internal_task_key(spmd_task)>();
  clog(trace) << "spmd_task is handle " << spmd_id << std::endl;

  // Add colors to must_epoch_launcher
  for(size_t color(0); color<num_colors; ++color) {

    // Serialize PhaseBarriers and set as task arguments
    std::vector<Legion::PhaseBarrier> pbarriers_as_master;
    std::vector<size_t> num_ghost_owners;
    std::vector<std::vector<Legion::PhaseBarrier>> owners_pbarriers;

    for(auto handle : map_handles) {
      pbarriers_as_master.push_back(phase_barriers_map[handle][color]);
      flecsi::coloring::coloring_info_t color_info = coloring_info[handle][color];
      clog(trace) << " Color " << color << " Handle " << handle << " has " <<
          color_info.ghost_owners.size() << " ghost owners" << std::endl;
      num_ghost_owners.push_back(color_info.ghost_owners.size());
      std::vector<Legion::PhaseBarrier> per_color_owners_pbs;
      for (auto owner : color_info.ghost_owners) {
        clog(trace) << owner << std::endl;
        per_color_owners_pbs.push_back(phase_barriers_map[handle][owner]);
      }
      owners_pbarriers.push_back(per_color_owners_pbs);
    } // for handle

    size_t num_handles = map_handles.size();
    args_serializers[color].serialize(&num_handles, sizeof(size_t));
    args_serializers[color].serialize(&pbarriers_as_master[0], num_handles
        * sizeof(Legion::PhaseBarrier));
    args_serializers[color].serialize(&num_ghost_owners[0], num_handles
        * sizeof(size_t));
    for (size_t handle=0; handle < num_handles; handle++)
      args_serializers[color].serialize(&owners_pbarriers[handle][0],
          num_ghost_owners[handle] * sizeof(Legion::PhaseBarrier));

    using field_info_t = context_t::field_info_t;

    size_t num_fields = context_.registered_fields().size();
    args_serializers[color].serialize(&num_fields, sizeof(size_t));
    args_serializers[color].serialize(
      &context_.registered_fields()[0], num_fields * sizeof(field_info_t));

    Legion::TaskLauncher spmd_launcher(spmd_id,
        Legion::TaskArgument(args_serializers[color].get_buffer(), args_serializers[color].get_used_bytes()));
    spmd_launcher.tag = MAPPER_FORCE_RANK_MATCH;

    // Add region requirements

    for(auto handle : map_handles) {
      Legion::LogicalPartition color_lp = runtime->get_logical_partition(ctx,
          expanded_lregions_map[handle], color_iparts_map[handle]);
      Legion::LogicalRegion color_lr = runtime->get_logical_subregion_by_color(ctx,
          color_lp, color);

      Legion::RegionRequirement rr(color_lr, READ_WRITE, SIMULTANEOUS, expanded_lregions_map[handle]);

      rr.add_field(ghost_owner_pos_fid);
      
      auto fitr = context_.field_info_map().find(handle);
      
      if(fitr != context_.field_info_map().end())
      {
        auto& field_map = fitr->second;

        // Allocate all fields on this index space
        for(auto& aitr : field_map){
          const context_t::field_info_t& fi = aitr.second;
          rr.add_field(fi.fid);
        }
      }

      spmd_launcher.add_region_requirement(rr);

      flecsi::coloring::coloring_info_t color_info = coloring_info[handle][color];
      for (auto ghost_owner : color_info.ghost_owners) {
        clog(trace) << " Color " << color << " Handle " << handle << " has owner " <<
            ghost_owner << std::endl;
        Legion::LogicalRegion ghost_owner_lr = runtime->get_logical_subregion_by_color(ctx,
            color_lp, ghost_owner);

        const LegionRuntime::Arrays::coord_t owner_color = ghost_owner;
        const bool is_mutable = false;
        runtime->attach_semantic_information(ghost_owner_lr, OWNER_COLOR_TAG, (void*)&owner_color,
            sizeof(LegionRuntime::Arrays::coord_t), is_mutable);
        spmd_launcher.add_region_requirement(
          Legion::RegionRequirement(ghost_owner_lr, READ_ONLY, SIMULTANEOUS, expanded_lregions_map[handle])
          .add_flags(NO_ACCESS_FLAG)
          .add_field(ghost_owner_pos_fid));

      } // for owner

    } // for handle

    Legion::DomainPoint point(color);
    must_epoch_launcher.add_single_task(point, spmd_launcher);
  } // for color

  // Launch the spmd tasks
  auto future = runtime->execute_must_epoch(ctx, must_epoch_launcher);
  future.wait_all_results();

  // Finish up Legion runtime and fall back out to MPI.

  for (auto itr = color_iparts_map.begin(); itr != color_iparts_map.end(); ++itr)
    runtime->destroy_index_partition(ctx, color_iparts_map[itr->first]);
  color_iparts_map.clear();

  for (auto itr = expanded_ispaces_map.begin(); itr != expanded_ispaces_map.end(); ++itr)
    runtime->destroy_index_space(ctx, expanded_ispaces_map[itr->first]);
  expanded_ispaces_map.clear();

  for (auto itr = expanded_fspaces_map.begin(); itr != expanded_fspaces_map.end(); ++itr)
    runtime->destroy_field_space(ctx, expanded_fspaces_map[itr->first]);
  expanded_fspaces_map.clear();

  for (auto itr = expanded_lregions_map.begin(); itr != expanded_lregions_map.end(); ++itr)
    runtime->destroy_logical_region(ctx, expanded_lregions_map[itr->first]);
  expanded_lregions_map.clear();

  for (auto itr : phase_barriers_map) {
    const size_t handle = itr.first;
    for (size_t color = 0; color < phase_barriers_map[handle].size(); color ++) {
      runtime->destroy_phase_barrier(ctx, phase_barriers_map[handle][color]);
    }
    phase_barriers_map[handle].clear();
  }
  phase_barriers_map.clear();


  context_.unset_call_mpi(ctx, runtime);
  context_.handoff_to_mpi(ctx, runtime);
} // runtime_driver

} // namespace execution 
} // namespace flecsi

/*~------------------------------------------------------------------------~--*
 * Formatting options for vim.
 * vim: set tabstop=2 shiftwidth=2 expandtab :
 *~------------------------------------------------------------------------~--*/
