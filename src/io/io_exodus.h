/*~--------------------------------------------------------------------------~*
 *  @@@@@@@@ @@       @@@@@@@@ @@     @@ @@
 * /@@///// /@@      /@@///// //@@   @@ /@@
 * /@@      /@@      /@@       //@@ @@  /@@
 * /@@@@@@@ /@@      /@@@@@@@   //@@@   /@@
 * /@@////  /@@      /@@////     @@/@@  /@@
 * /@@      /@@      /@@        @@ //@@ /@@
 * /@@      /@@@@@@@@/@@@@@@@@ @@   //@@/@@
 * //       //////// //////// //     // //
 *
 * Copyright (c) 2016 Los Alamos National Laboratory, LLC
 * All rights reserved
 *~--------------------------------------------------------------------------~*/

#ifndef flexi_io_exodus_h
#define flexi_io_exodus_h

#include <exodusII.h>

#include "io_base.h"
#include "../specializations/burton.h"

/*!
 * \file io_exodus.h
 * \authors wohlbier
 * \date Initial file creation: Oct 07, 2015
 */

namespace flexi {

/*!
  \class io_exodus_t io_exodus.h
  \brief io_exodus_t provides a derived type of io_base.h and registrations
         of the file extensions.
 */
struct io_exodus_t : public io_base_t {

  //! Default constructor
  io_exodus_t() {}

  //! implementation of exodus read
  int32_t read(const std::string &filename, burton_mesh_t &m);

  //! implementation of exodus write
  int32_t write(const std::string &filename, const burton_mesh_t &m);

}; // struct io_exodus_t

//! create an io_exodus_t and return a pointer to the base class
io_base_t *create_io_exodus() { return new io_exodus_t; } // create_io_exodus

//! register file extension g with factory
bool exodus_g_registered =
    io_factory_t::instance().registerType("g", create_io_exodus);

//! register file extension exo with factory
bool exodus_exo_registered =
    io_factory_t::instance().registerType("exo", create_io_exodus);

//! implementation of exodus read
int32_t io_exodus_t::read(const std::string &filename, burton_mesh_t &m) {
  std::cout << "Reading mesh from file: " << filename << std::endl;

  // size of floating point variables used in app.
  int CPU_word_size = sizeof(burton_mesh_t::real_t);
  // size of floating point to be stored in file.
  int IO_word_size = 0;
  float version;

  auto exoid = ex_open(
      filename.c_str(), EX_READ, &CPU_word_size, &IO_word_size, &version);
  assert(exoid >= 0);

  // get the initialization parameters
  ex_init_params exopar;
  auto status = ex_get_init_ext(exoid, &exopar);
  assert(status == 0);

  // verify 2d mesh
  assert(m.dimension() == exopar.num_dim);
  auto num_nodes = exopar.num_nodes;
  auto num_elem = exopar.num_elem;
  auto num_elem_blk = exopar.num_elem_blk;
  auto num_node_sets = exopar.num_node_sets;
  auto num_side_sets = exopar.num_side_sets;

  // read nodes
  burton_mesh_t::real_t xcoord[num_nodes];
  burton_mesh_t::real_t ycoord[num_nodes];
  status = ex_get_coord(exoid, xcoord, ycoord, nullptr);
  assert(status == 0);

  // put nodes into mesh
  std::vector<burton_mesh_t::vertex_t *> vs;
  for (size_t i = 0; i < num_nodes; ++i) {
    auto v = m.create_vertex({xcoord[i], ycoord[i]});
    v->setRank(1);
    vs.push_back(v);
  } // for

  // 1 block for now

  // read blocks
  int blockids[num_elem_blk];
  status = ex_get_elem_blk_ids(exoid, blockids);
  assert(status == 0);
  char block_name[256];
  status = ex_get_name(exoid, EX_ELEM_BLOCK, blockids[0], block_name);
  assert(status == 0);

  // get the info about this block
  auto num_attr = 0;
  auto num_nodes_per_elem = 0;
  char elem_type[256];
  status = ex_get_elem_block(
      exoid, blockids[0], elem_type, &num_elem, &num_nodes_per_elem, &num_attr);
  assert(status == 0);

  // verify mesh has quads
  assert(num_nodes_per_elem == 4);
  assert(strcmp(elem_type, "quad") == 0);

  // read element definitions
  int elt_conn[num_elem * num_nodes_per_elem];
  status = ex_get_elem_conn(exoid, blockids[0], elt_conn);
  assert(status == 0);

  // store elements
  for (size_t e = 0; e < num_elem; ++e) {
    //        for ( ex_entity_id iNode=0; iNode<nNodesPerElem; iNode++ ) {
    //          ex_entity_id id = iElem*nNodesPerElem + iNode;
    //          set[i].ElementNodes[ id ] = NodeList[ id ] - 1;
    //        }
    //      auto b = e*num_nodes_per_elem;
    //      auto c = m.create_cell({vs[b+0],vs[b+1],vs[b+2],vs[b+3]});
  }
  m.init();

  return status;
}

//! implementation of exodus write
int32_t io_exodus_t::write(
    const std::string &filename, const burton_mesh_t &m) {

  std::cout << "Writing mesh to file: " << filename << std::endl;

  // size of floating point variables used in app.
  int CPU_word_size = sizeof(burton_mesh_t::real_t);
  // size of floating point to be stored in file.
  int IO_word_size = sizeof(burton_mesh_t::real_t);
  auto exoid =
      ex_create(filename.c_str(), EX_CLOBBER, &CPU_word_size, &IO_word_size);
  assert(exoid >= 0);
  auto d = m.dimension();
  auto num_nodes = m.numVertices();
  auto num_elem = m.numCells();
  auto num_elem_blk = 1;
  auto num_node_sets = 0;
  auto num_side_sets = 0;

  // initialize the file.
  auto status = ex_put_init(exoid, "Exodus II output from flexi.", d, num_nodes,
      num_elem, num_elem_blk, num_node_sets, num_side_sets);
  assert(status == 0);

  // get the coordinates from the mesh.
  burton_mesh_t::real_t xcoord[num_nodes];
  burton_mesh_t::real_t ycoord[num_nodes];
  auto i = 0;
  for (auto v : m.vertices()) {
    xcoord[i] = v->coordinates()[0];
    ycoord[i] = v->coordinates()[1];
    i++;
  } // for
  // write the coordinates to the file
  status = ex_put_coord(exoid, xcoord, ycoord, nullptr);
  assert(status == 0);

  // write the coordinate names
  const char *coord_names[3];
  coord_names[0] = "x";
  coord_names[1] = "y";
  coord_names[2] = "z";
  status = ex_put_coord_names(exoid, (char **)coord_names);
  assert(status == 0);

  // loop over element blocks
  auto blockid = 0;
  auto num_attr = 0;
  auto num_nodes_per_elem = 4;
  status = ex_put_elem_block(
      exoid, blockid, "quad", num_elem, num_nodes_per_elem, num_attr);
  assert(status == 0);

  // element definitions
  int elt_conn[num_elem * num_nodes_per_elem];
  i = 0;
  for (auto c : m.cells()) {
    for (auto v : m.vertices(c)) {
      elt_conn[i] = v->id() + 1;
      i++;
    } // for
  }   // for

  // write connectivity
  status = ex_put_elem_conn(exoid, blockid, elt_conn);
  assert(status == 0);

  // close
  status = ex_close(exoid);

  return status;
} // io_exodus_t::write

} // namespace flexi

#endif // flexi_io_exodus_h

/*~-------------------------------------------------------------------------~-*
 * Formatting options
 * vim: set tabstop=2 shiftwidth=2 expandtab :
 *~-------------------------------------------------------------------------~-*/
